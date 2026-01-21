#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>
#include <boost/bind.hpp>
#include <muduo/net/EventLoop.h>
#include <functional>
#include <muduo/net/TcpClient.h>
#include <iostream>
using namespace std;
using namespace muduo;
using namespace muduo::net;

/*
moduo库的使用步骤
1 组合TcpServer对象
2 创建 EventLoop事件循环对象的指针
3 明确TcpServer构造函数需要什么参数，输出ChstServer的构造函数
4 在当前服务器类中的构造函数中，注册处理连接的回调函数和处理读写事件的回调函数
5 设置线程数量

*/
class ChatServer
{
  public:
  ChatServer(EventLoop* loop,
    const InetAddress& listenAddr,
    const string& nameArg):_server(loop,listenAddr,nameArg),_loop(loop)
    {

      //给服务器注册用户连接的创建和断开回调
      _server.setConnectionCallback(std::bind(&ChatServer::onConnect,this,placeholders::_1));
      //给服务器注册用户读写事件回调
      _server.setMessageCallback(bind(&ChatServer::onMessage,this,placeholders::_1,placeholders::_2,placeholders::_3 ));

      _server.setThreadNum(2);


    }
    void start()
    {
      _server.start();
    }
  private:
    TcpServer _server;
    EventLoop *_loop;
//专门处理用户的连接和创建和断开
    void onConnect(const TcpConnectionPtr&conn)
    {
      if (conn->connected())
      {
       std::cout<< conn->peerAddress().toIpPort()<<" -> "<<
      conn->localAddress().toIpPort() << "state:online"<<endl;

      }
      else
      {
        cout<< conn->peerAddress().toIpPort()<<" -> "<<
      conn->localAddress().toIpPort() << "state:offline"<<endl;
      conn->shutdown();
      }

    }
    //专门处理用户的读写事件  连接 缓冲区 接收到数据的时间信息
    void onMessage(const TcpConnectionPtr& conn,
                            Buffer* buffer,
                            Timestamp time)
    {
      string buf = buffer->retrieveAllAsString();
      cout<<"recv data:"<<buf<<"time:"<<time.toString()<<endl;
      conn->send(buf);


    }

};

int main()
{
  EventLoop loop;
  InetAddress addr("127.0.0.1",6000);
  ChatServer server(&loop,addr,"chatserver");
  server.start();
  loop.loop();
  return 0;
}

