#include "tcpserver.h"

TcpServer::TcpServer(const std::string &ip,const uint16_t port,int threadnum)
                 :threadnum_(threadnum),mainloop_(new EventLoop(true)), 
                  acceptor_(mainloop_.get(),ip,port),threadpool_(threadnum_,"IO"),log_(Logfile::GetObject())
{
    // 设置epoll_wait()超时的回调函数。
    mainloop_->SetEpollTimeoutCallback(std::bind(&TcpServer::EpollTimeout,this,std::placeholders::_1));   

    // 设置处理新客户端连接请求的回调函数。
    acceptor_.SetNewConnectioncb(std::bind(&TcpServer::NewConnection,this,std::placeholders::_1));

    // 创建从事件循环。
    for (int ii=0;ii<threadnum_;ii++)
    {
        subloops_.emplace_back(new EventLoop(false,5,10));              // 创建从事件循环，存入subloops_容器中。
        subloops_[ii]->SetEpollTimeoutCallback(std::bind(&TcpServer::EpollTimeout,this,std::placeholders::_1));   // 设置timeout超时的回调函数。
        subloops_[ii]->SetTimerCallback(std::bind(&TcpServer::RemoveConn,this,std::placeholders::_1));   // 设置清理空闲TCP连接的回调函数。
        threadpool_.AddTask(std::bind(&EventLoop::Run,subloops_[ii].get()));    // 在线程池中运行从事件循环。
    }
}

TcpServer::~TcpServer()
{
}

// 运行事件循环。
void TcpServer::Start()          
{
    mainloop_->Run();
    
}

 // 停止IO线程和事件循环。
 void TcpServer::Stop()          
 {
    // 停止主事件循环。
    mainloop_->Stop();
    printf("主事件循环已停止。\n");

    // 停止从事件循环。
    for (int ii=0;ii<threadnum_;ii++)
    {
        subloops_[ii]->Stop();
    }
    printf("从事件循环已停止。\n");

    // 停止IO线程。
    threadpool_.Stop();
    printf("IO线程池停止。\n");
 }


// 处理新客户端连接请求。
void TcpServer::NewConnection(std::unique_ptr<Socket> clientsock)
{
    // 把新建的conn分配给从事件循环。
    spConnection conn(new Connection(subloops_[clientsock->Fd()%threadnum_].get(),std::move(clientsock)));   
    conn->SetCloseCallback(std::bind(&TcpServer::CloseConnection,this,std::placeholders::_1));
    conn->SetErrorCallback(std::bind(&TcpServer::ErrorConnection,this,std::placeholders::_1));
    conn->SetOnMessageCallback(std::bind(&TcpServer::OnMessage,this,std::placeholders::_1,std::placeholders::_2));
    conn->SetSendCompleteCallback(std::bind(&TcpServer::SendComplete,this,std::placeholders::_1));
    // conn->enablereading();

    // printf ("new connection(fd=%d,ip=%s,port=%d) ok.\n",conn->fd(),conn->ip().c_str(),conn->port());

    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conns_[conn->Fd()]=conn;            // 把conn存放到TcpSever的map容器中。
    }
    subloops_[conn->Fd()%threadnum_]->NewConnection(conn);       // 把conn存放到EventLoop的map容器中。

    //将std::function<void()>对象用作if语句的条件
    //在这种情况下，如果std::function<void()>对象包含可调用对象，则条件为true；
    //如果对象为空（即未包装任何可调用对象），则条件为false。
    if (newconnectioncb_) newconnectioncb_(conn);             // 回调上层业务类的HandleNewConnection()。
}

 // 关闭客户端的连接，在Connection类中回调此函数。 
 void TcpServer::CloseConnection(spConnection conn)
 {
    if (closeconnectioncb_) closeconnectioncb_(conn);       // 回调上层业务类的HandleClose()。

    // printf("client(fd=%d) disconnected.\n",conn->fd());
    {
         std::lock_guard<std::mutex> gd(mmutex_);
        conns_.erase(conn->Fd());        // 从map中删除conn。
    }
 }

// 客户端的连接错误，在Connection类中回调此函数。
void TcpServer::ErrorConnection(spConnection conn)
{
    if (errorconnectioncb_) errorconnectioncb_(conn);     // 回调上层业务类的HandleError()。

    // printf("client(fd=%d) error.\n",conn->fd());
    {
         std::lock_guard<std::mutex> gd(mmutex_);
        conns_.erase(conn->Fd());      // 从map中删除conn。
    }
}

// 处理客户端的请求报文，在Connection类中回调此函数。
void TcpServer::OnMessage(spConnection conn,std::string& message)
{
    if (onmessagecb_) onmessagecb_(conn,message);     // 回调上层业务类的HandleMessage()。
}

// 数据发送完成后，在Connection类中回调此函数。
void TcpServer::SendComplete(spConnection conn)     
{
    // printf("send complete.\n");

    if (sendcompletecb_) sendcompletecb_(conn);     // 回调上层业务类的HandleSendComplete()。
}

// epoll_wait()超时，在EventLoop类中回调此函数。
void TcpServer::EpollTimeout(EventLoop *loop)         
{
    // printf("epoll_wait() timeout.\n");

    if (timeoutcb_)  timeoutcb_(loop);           // 回调上层业务类的HandleTimeOut()。
}

void TcpServer::SetNewConnectioncb(std::function<void(spConnection)> fn)
{
    newconnectioncb_=fn;
}

void TcpServer::SetCloseConnectioncb(std::function<void(spConnection)> fn)
{
    closeconnectioncb_=fn;
}

void TcpServer::SetErrorConnectioncb(std::function<void(spConnection)> fn)
{
    errorconnectioncb_=fn;
}

void TcpServer::SetOnMessagecb(std::function<void(spConnection,std::string &message)> fn)
{
    onmessagecb_=fn;
}

void TcpServer::SetSendCompletecb(std::function<void(spConnection)> fn)
{
    sendcompletecb_=fn;
}

void TcpServer::SetTimeoutcb(std::function<void(EventLoop*)> fn)
{
    timeoutcb_=fn;
}

// 删除conns_中的Connection对象，在EventLoop::handletimer()中将回调此函数。
void TcpServer::RemoveConn(int fd)                 
{
    {
        std::lock_guard<std::mutex> gd(mmutex_);
        conns_.erase(fd);          // 从map中删除conn。
    }

    if (removeconnectioncb_) removeconnectioncb_(fd);
}

void TcpServer::SetRemoveConnectioncb(std::function<void(int)> fn)
{
    removeconnectioncb_=fn;
}

// void TcpServer::WriteLog(const char* log,bool with_info)
