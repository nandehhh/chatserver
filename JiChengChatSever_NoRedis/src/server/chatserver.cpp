#include "chatserver.hpp"
#include <functional>
#include "json.hpp"
#include "chatservice.hpp"
using namespace std;
using namespace placeholders;
using json = nlohmann::json;
ChatServer::ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg):_server(loop,listenAddr,nameArg),_loop(loop)
{
    //注册链接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConection,this,_1));

    //注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));

    //设置线程数量
    _server.setThreadNum(4);


}

void ChatServer::start()
{

    _server.start();
}


void ChatServer::onConection(const TcpConnectionPtr& conn)
{
    //客户断开链接
    if (!conn->connected())
    {
        //客户端异常关闭情况的处理
        ChatService::instance()->clientCloseException(conn);


        conn->shutdown();

    }

}


void ChatServer::onMessage(const TcpConnectionPtr&conn,
                            Buffer* buffer,
                            Timestamp time)
{
    //转成string类型
    string buf = buffer->retrieveAllAsString();
    //string类型转成json类型
    //通过解析js[msg_id]来调用不同的函数handler
    json js = json::parse(buf);


    //完全解耦网络模块代码和业务模块代码
    //使用回调的方式，来进行解耦
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    //回调消息绑定好的事件处理器，来执行相应的业务处理
    msgHandler(conn,js,time);
}