#pragma once
#include <functional>
#include "socket.h"
#include "inetaddress.h"
#include "channel.h"
#include "eventloop.h"
#include "buffer.h"
#include "timestamp.h"
#include <memory>
#include <atomic>
#include <sys/syscall.h>
#include "httpstatemachine.h"

class EventLoop;
class Channel;
class Connection;
using spConnection=std::shared_ptr<Connection>;

class Connection:public std::enable_shared_from_this<Connection>
{
private:
    EventLoop* loop_;                                           // Connection对应的事件循环，在构造函数中传入。 
    std::unique_ptr<Socket> clientsock_;             // 与客户端通讯的Socket。 
    std::unique_ptr<Channel> clientchannel_;     // Connection对应的channel，在构造函数中创建。
    Buffer inputbuffer_;             // 接收缓冲区。
    Buffer outputbuffer_;          // 发送缓冲区。
    std::atomic_bool disconnect_;      // 客户端连接是否已断开，如果已断开，则设置为true。
    HttpStateMachine state_machine_;

    std::function<void(spConnection)> closecallback_;                   // 关闭fd_的回调函数，将回调TcpServer::closeconnection()。
    std::function<void(spConnection)> errorcallback_;                   // fd_发生了错误的回调函数，将回调TcpServer::errorconnection()。
    std::function<void(spConnection,std::string&)> onmessagecallback_;   // 处理报文的回调函数，将回调TcpServer::onmessage()。
    std::function<void(spConnection)> sendcompletecallback_;               // 发送数据完成后的回调函数，将回调TcpServer::sendcomplete()。
    Timestamp lastatime_;             // 时间戳，创建Connection对象时为当前时间，每接收到一个报文，把时间戳更新为当前时间。

public:
    Connection(EventLoop* loop,std::unique_ptr<Socket> clientsock);
    ~Connection();

    int Fd() const;                              // 返回客户端的fd。
    std::string Ip() const;                   // 返回客户端的ip。
    uint16_t Port() const;                  // 返回客户端的port。 

    void OnMessage();                      // 处理对端发送过来的消息。
    void CloseCallback();                   // TCP连接关闭（断开）的回调函数，供Channel回调。
    void ErrorCallback();                   // TCP连接错误的回调函数，供Channel回调。
    void WriteCallback();                   // 处理写事件的回调函数，供Channel回调。

    void SetCloseCallback(std::function<void(spConnection)> fn);    // 设置关闭fd_的回调函数。
    void SetErrorCallback(std::function<void(spConnection)> fn);    // 设置fd_发生了错误的回调函数。
    void SetOnMessageCallback(std::function<void(spConnection,std::string&)> fn);    // 设置处理报文的回调函数。
    void SetSendCompleteCallback(std::function<void(spConnection)> fn);    // 发送数据完成后的回调函数。

    // 发送数据，不管在任何线程中，都是调用此函数发送数据。
    void Send(const char *data,size_t size);
    void Send(const char *data,size_t size,std::string* strptr);        
    // 发送数据，如果当前线程是IO线程，直接调用此函数，如果是工作线程，将把此函数传给IO线程去执行。
    void SendInloop(const char *data,size_t size);
    //发送回应报文的时候需要用这个函数？
    void SendInloop(const char *data,size_t size,std::string* strptr);        

    bool Timeout(time_t now,int val);           // 判断TCP连接是否超时（空闲太久）。

    void StateMachineAppend(std::string& str);//向状态机中加入信息

    bool Parse();

    HTTPPacketInfo GetInfo();

};
