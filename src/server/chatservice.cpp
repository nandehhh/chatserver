#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <vector>
using namespace std;

ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}

//注册消息以及对应的Handler回调操作
ChatService::ChatService()
{
    //用户业务
    _msgHandlerMap.insert({LOGIN_MSG,bind(&ChatService::login,this,_1,_2,_3)});
    _msgHandlerMap.insert({REG_MSG,bind(&ChatService::reg,this,_1,_2,_3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG,bind(&ChatService::oneChat,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG,bind(&ChatService::addFriend,this,_1,_2,_3)});
    _msgHandlerMap.insert({LOGINOUT_MSG,bind(&ChatService::loginout,this,_1,_2,_3)});
    //群组业务
    _msgHandlerMap.insert({CREATE_GROUP_MSG,bind(&ChatService::createGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG,bind(&ChatService::addGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG,bind(&ChatService::groupChat,this,_1,_2,_3)});

    //连接redis服务器
    if (_redis.connect())
    {
        //设置上报消息的回调
        _redis.init_notify_handler(bind(&ChatService::handleRedisSubscribeMessage,this,_1,_2));
    }
    
}


void ChatService::restart()
{
    //将所有用户的online状态置成offline
    _userModel.reset();

}

//处理登录业务
void ChatService::login(const TcpConnectionPtr& conn,json &js,Timestamp time)
{
    //LOG_INFO<<"do login service!!!";
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User usr = _userModel.query(id);
    if(usr.getId() == id && usr.getPassword() == pwd)
    {
        if(usr.getState() == "online")
        {
            //用户存在，密码正确，但是已经登录了
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is using,input another!";
            conn->send(response.dump());

        }
        else
        {
            //登录成功，记录用户连接信息
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id,conn});
            }

            //id用户登录成功后，向redis订阅channel(id)
            _redis.subscribe(id);//我对reis上id这个通道感兴趣，因为别人给我发的消息都是发的这个id
            
            //登录成功  更新用户状态信息
            usr.setState("online");
            _userModel.updateState(usr);



            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = usr.getId();
            response["name"] = usr.getName();


            //查询该用户是否有离线消息，如果有加到json中
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                //json库能直接将vector数据存放到offlinemsg中
                response["offlinemsg"] = vec;
                //读取离线消息后，将该用户的离线消息清除
                _offlineMsgModel.remove(id);
            }
            
            //查询好友信息，并返回
            vector<User> vec_user = _friendmodel.query(id);
            if (!vec_user.empty())
            {
                vector<string> vec2;
                for (User &user : vec_user)
                {
                    json js;
                    js["id"]= user.getId();
                    js["name"]=user.getName();
                    js["state"]=user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

             // 查询用户的群组信息
            vector<Group> groupuserVec = _groupModel.queryGroup(id);
            if (!groupuserVec.empty())
            {
                // group:[{groupid:[xxx, xxx, xxx, xxx]}]
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getrole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }

                response["groups"] = groupV;
            }

            conn->send(response.dump());
        }
        
        

    }
    else
    {
        //登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalid!";
        conn->send(response.dump());

    }
}
//处理注册业务
void ChatService::reg(const TcpConnectionPtr& conn,json &js,Timestamp time)
{
    //LOG_INFO<<"do reg service!!!";
    string name = js["name"];
    string pwd = js["password"];

    User usr;
    usr.setName(name);
    usr.setPassword(pwd);
    bool state = _userModel.insert(usr);
    if (state)
    {
        //注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = usr.getId();
        conn->send(response.dump());
    }
    else
    {
        //注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
    
}

//获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    //记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        //返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr &conn,
                                       json &js,
                                        Timestamp){
        LOG_ERROR <<"msgid: "<<msgid<<" can not find handler!";
                                        };
    }
    else
        return _msgHandlerMap[msgid];
    
}

void ChatService::clientCloseException(const TcpConnectionPtr& conn)
{
    //需要做两件事
    //第一个是从在线用户的map中删除该用户
    //第二个是将用户信息的online置为offline

    User usr;
    //有的情况是用户正在使用呢，异常退出后，需要注意线程安全问题
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it=_userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            //注意earse的迭代器失效问题，但是这里搜到后直接返回就行，不用往下遍历了。
            if (it->second == conn)
            {
                //删除用户的连接信息
              usr.setId(it->first);
              it = _userConnMap.erase(it);
              break;
            }
        }
    }

    //用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(usr.getId());

    if (usr.getId() != -1)
    {
        //更新用户的状态信息
        usr.setState("offline");
        _userModel.updateState(usr);
    }
    
    


}

//一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr& conn,json &js,Timestamp time)
{

    //接收者id
    int toid = js["toid"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if(it != _userConnMap.end())
        {
            //说明，接收者在线，转发消息 服务器主动推送消息给接收端
            it->second->send(js.dump());
            return;

        }
    }

    //第一种是用户都在一个服务器，这样就看_userConnMap在没在就行，也就是上面这种
    //第二种是在线，但是没在一个服务器上，这种就直接发送到redis的通道上
    User user = _userModel.query(toid);

    if (user.getState() == "online")
    {
        _redis.publish(toid,js.dump());//
        return;
    }
    
     //接收者不在线，存储离线消息
     _offlineMsgModel.insert(toid,js.dump());


}

void ChatService::addFriend(const TcpConnectionPtr& conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    //向表中填好友信息
    _friendmodel.insert(userid,friendid);
    
}

void ChatService::createGroup(const TcpConnectionPtr& conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    //存储新创建的群组信息
    Group group(-1,name,desc);
    if (_groupModel.createGroup(group))
    {
        //存储群组创建人的信息
        _groupModel.addGroup(userid,group.getId(),"creator");
    }
}

//加入群组
void ChatService::addGroup(const TcpConnectionPtr& conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid,groupid,"normal");
}

//群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr& conn,json &js,Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    //群组中所有的用户id
    vector<int> useridVec = _groupModel.queryGroupUsers(userid,groupid);

    //从群组中拿取一个用户id，判断是不是在线，在线直接转发，不在线存储离线消息
    lock_guard<mutex> lock(_connMutex);
    for (int id:useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            //转发群消息
            it->second->send(js.dump());
        }
        else
        {
            //查询toid是否在线，存在不在同一个服务器的情况
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id,js.dump());
            }
            else
            {
                //存储离线消息
                _offlineMsgModel.insert(id,js.dump());
            }
        }
        

    }
    
}

 void ChatService::loginout(const TcpConnectionPtr& conn,json &js,Timestamp time)
 {
    int userid = js["id"];
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    //用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(userid);

    //更新用户的状态信息
    User usr(userid,"","","offline");
    _userModel.updateState(usr);



 }


 //从redis中获取订阅的消息
 void ChatService::handleRedisSubscribeMessage(int userid,string msg)
 {
    json js = json::parse(msg.c_str());

    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())//说明在线呢
    {
        it->second->send(js.dump());
        return;
    }

    //不在线，那么存储离线消息
    _offlineMsgModel.insert(userid,msg);
    


 }