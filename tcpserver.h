#pragma once
#include "eventloop.h"
#include "socket.h"
#include "channel.h"
#include "acceptor.h"
#include "connection.h"
#include "threadpool.h"
#include <map>
#include <mutex>
#include <memory>
#include "logfile.h"

// TCP网络服务类。
class TcpServer
{
private:
    std::unique_ptr<EventLoop> mainloop_;                                   // 主事件循环，处理tcp连接，Eventloop中的Epoll只注册了Acceptor的读事件
    std::vector<std::unique_ptr<EventLoop>> subloops_;                      // 存放从事件循环的容器。 处理每个连接的io
    Acceptor acceptor_;                                                     // 一个TcpServer只有一个Acceptor对象，由于被动连接。
    int threadnum_;                                                         // 线程池的大小，即从事件循环(Eventloop)的个数。
    ThreadPool threadpool_;                                                 // 线程池(每个线程执行一个Eventloop)。
    std::mutex mmutex_;                                                     // 保护conns_的互斥锁。
    std::map<int,spConnection>  conns_;                                     // 一个TcpServer有多个Connection对象，每个Connection对应一个tcp连接，存放在map容器中。
    std::function<void(spConnection)> newconnectioncb_;                     // 回调上层业务类HandleNewConnection()。
    std::function<void(spConnection)> closeconnectioncb_;                   // 回调上层业务类HandleClose()。
    std::function<void(spConnection)> errorconnectioncb_;                   // 回调上层业务类HandleError()。
    std::function<void(spConnection,std::string &message)> onmessagecb_;    // 回调上层业务类HandleMessage()。
    std::function<void(spConnection)> sendcompletecb_;                      // 回调上层业务类HandleSendComplete()。
    std::function<void(EventLoop*)>  timeoutcb_;                            // 回调上层业务类HandleTimeOut()。
    std::function<void(int)>  removeconnectioncb_;                          // 回调上层业务类的HandleRemove()。
    Logfile* log_;                                                          // 日志
public:
    TcpServer(const std::string &ip,const uint16_t port,int threadnum=3);
    ~TcpServer();

    void Start();          // 运行事件循环。 
    void Stop();          // 停止IO线程和事件循环。

    void NewConnection(std::unique_ptr<Socket> clientsock);    // 处理新客户端连接请求，在Acceptor类中回调此函数。
    void CloseConnection(spConnection conn);  // 关闭客户端的连接，在Connection类中回调此函数。 
    void ErrorConnection(spConnection conn);  // 客户端的连接错误，在Connection类中回调此函数。
    void OnMessage(spConnection conn,std::string& message);     // 处理客户端的请求报文，在Connection类中回调此函数。
    void SendComplete(spConnection conn);     // 数据发送完成后，在Connection类中回调此函数。
    void EpollTimeout(EventLoop *loop);           // epoll_wait()超时，在EventLoop类中回调此函数。

    void SetNewConnectioncb(std::function<void(spConnection)> fn);
    void SetCloseConnectioncb(std::function<void(spConnection)> fn);
    void SetErrorConnectioncb(std::function<void(spConnection)> fn);
    void SetOnMessagecb(std::function<void(spConnection,std::string &message)> fn);
    void SetSendCompletecb(std::function<void(spConnection)> fn);
    void SetTimeoutcb(std::function<void(EventLoop*)> fn);

    void RemoveConn(int fd);                 // 删除conns_中的Connection对象，在EventLoop::handletimer()中将回调此函数。
    void SetRemoveConnectioncb(std::function<void(int)> fn);
    template<typename... Args>
    void WriteLog(bool with_info,const char* log,Args... args)
    {
        if(with_info == true)
        {
            log_->Write(log,args...);
        }
        else
        {
            (*log_)<<log;
        }
    }
};