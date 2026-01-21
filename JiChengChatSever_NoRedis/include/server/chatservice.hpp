//用于解耦业务代码和网络代码,这里实现业务代码
#ifndef CHATSERVICE_H
#define CHATSERVICE_H
#include<unordered_map>
#include <functional>
#include <muduo/net/TcpConnection.h>
#include <mutex>
#include "UserModel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendModel.hpp"
#include "groupmodel.hpp"
using namespace std;
using namespace muduo::net;
using namespace muduo;

#include "json.hpp"
using namespace placeholders;
using json = nlohmann::json;
//使用单例模式来实现业务类，业务回调
//一个消息id映射一个事件处理
using MsgHandler = std::function<void(const TcpConnectionPtr &conn,
                                       json &js,
                                        Timestamp)>;


class ChatService
{
public:
    //获取单例对象的接口函数
    static ChatService* instance();
    //处理登录业务
    void login(const TcpConnectionPtr& conn,json &js,Timestamp time);
    //处理注册业务
    void reg(const TcpConnectionPtr& conn,json &js,Timestamp time);
    
    //一对一聊天业务
    void oneChat(const TcpConnectionPtr& conn,json &js,Timestamp time);

    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);

    //服务器异常，业务重置方法
    void restart();

    //添加好友业务
    void addFriend(const TcpConnectionPtr& conn,json &js,Timestamp time);

    //创建群组业务
    void createGroup(const TcpConnectionPtr& conn,json &js,Timestamp time);

    //加入群组业务
    void addGroup(const TcpConnectionPtr& conn,json &js,Timestamp time);

    //群组聊天业务
    void groupChat(const TcpConnectionPtr& conn,json &js,Timestamp time);

    //处理注销业务
    void loginout(const TcpConnectionPtr& conn,json &js,Timestamp time);

    //客户端异常关闭情况下的处理
    void clientCloseException(const TcpConnectionPtr& conn);
private:
    //私有化构造函数
    ChatService();
    //存储序号和相应的处理接口
    unordered_map<int,MsgHandler> _msgHandlerMap;

    //存储在线用户的通信连接  用户id以及对应的连接
    unordered_map<int,TcpConnectionPtr> _userConnMap; //这个指定会多个线程都会调用到，涉及到线程安全问题，所以需要加锁来保证互斥

    //定义互斥锁，保证线程安全
    mutex _connMutex;

    //数据操作类对象
    UserModel _userModel;

    //离线消息操作类
    OfflineMsgModel _offlineMsgModel;

    //好友操作类
    friendModel _friendmodel;

    //群组操作类
    GroupModel _groupModel;    

};






#endif