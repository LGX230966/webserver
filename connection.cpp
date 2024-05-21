#include "connection.h"

Connection::Connection(EventLoop* loop,std::unique_ptr<Socket> clientsock)
                   :loop_(loop),clientsock_(std::move(clientsock)),disconnect_(false),clientchannel_(new Channel(loop_,clientsock_->Fd())) 
{
    // 为新客户端连接准备读事件，并添加到epoll中。
    clientchannel_->SetReadCallback(std::bind(&Connection::OnMessage,this));
    clientchannel_->SetCloseCallback(std::bind(&Connection::CloseCallback,this));
    clientchannel_->SetErrorCallback(std::bind(&Connection::ErrorCallback,this));
    clientchannel_->SetWriteCallback(std::bind(&Connection::WriteCallback,this));
    clientchannel_->UseET();                 // 客户端连上来的fd采用边缘触发。
    clientchannel_->EnableReading();   // 让epoll_wait()监视clientchannel的读事件
}

Connection::~Connection()
{
    // printf("conn已析构。\n");
}

int Connection::Fd() const                              // 返回客户端的fd。
{
    return clientsock_->Fd();
}

std::string Connection::Ip() const                   // 返回客户端的ip。
{
    return clientsock_->Ip();
}

uint16_t Connection::Port() const                  // 返回客户端的port。
{
    return clientsock_->Port();
}

void Connection::CloseCallback()                    // TCP连接关闭（断开）的回调函数，供Channel回调。
{
    disconnect_=true;
    clientchannel_->Remove();                         // 从事件循环中删除Channel。
    closecallback_(shared_from_this());
}

void Connection::ErrorCallback()                    // TCP连接错误的回调函数，供Channel回调。
{
    disconnect_=true;
    clientchannel_->Remove();                  // 从事件循环中删除Channel。
    errorcallback_(shared_from_this());     // 回调TcpServer::errorconnection()。
}

// 设置关闭fd_的回调函数。
void Connection::SetCloseCallback(std::function<void(spConnection)> fn)    
{
    closecallback_=fn;     // 回调TcpServer::closeconnection()。
}

// 设置fd_发生了错误的回调函数。
void Connection::SetErrorCallback(std::function<void(spConnection)> fn)    
{
    errorcallback_=fn;     // 回调TcpServer::errorconnection()。
}

// 设置处理报文的回调函数。
void Connection::SetOnMessageCallback(std::function<void(spConnection,std::string&)> fn)    
{
    onmessagecallback_=fn;       // 回调TcpServer::onmessage()。
}

// 发送数据完成后的回调函数。
void Connection::SetSendCompleteCallback(std::function<void(spConnection)> fn)    
{
    sendcompletecallback_=fn;
}

// 处理对端发送过来的消息。
void Connection::OnMessage()
{
    char buffer[1024];
    while (true)             // 由于使用非阻塞IO，一次读取buffer大小数据，直到全部的数据读取完毕。
    {    
        bzero(&buffer, sizeof(buffer));
        ssize_t nread = read(Fd(), buffer, sizeof(buffer));
        if (nread > 0)      // 成功的读取到了数据。
        {
            inputbuffer_.Append(buffer,nread);      // 把读取的数据追加到接收缓冲区中。
        } 
        else if (nread == -1 && errno == EINTR) // 读取数据的时候被信号中断，继续读取。
        {  
            continue;
        } 
        else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) // 全部的数据已读取完毕。
        {
            std::string message;
            while (true)             // 从接收缓冲区中拆分出客户端的请求消息。
            {
                if (inputbuffer_.PickMessage(message)==false) break;

                // printf("message (fd=%d):%s\n",fd(),message.c_str());
                lastatime_=Timestamp::Now();             // 更新Connection的时间戳。

                onmessagecallback_(shared_from_this(),message);       // 回调TcpServer::onmessage()处理客户端的请求消息。
            }
            break;
        } 
        else if (nread == 0)  // 客户端连接已断开。
        {  
            CloseCallback();                                  // 回调TcpServer::closecallback()。
            break;
        }
    }
}

// 发送数据，不管在任何线程中，都是调用此函数发送数据。
void Connection::Send(const char *data,size_t size)        
{
    if (disconnect_==true) {  printf("客户端连接已断开了，send()直接返回。\n"); return;}

    if (loop_->IsInloopThread())   // 判断当前线程是否为事件循环线程（IO线程）。
    {
        // 如果当前线程是IO线程，直接调用sendinloop()发送数据。
        // printf("send() 在事件循环的线程中。\n");
        SendInloop(data,size);
    }
    else
    {
        // 如果当前线程不是IO线程，调用EventLoop::queueinloop()，把sendinloop()交给事件循环线程去执行。
        // printf("send() 不在事件循环的线程中。\n");
        loop_->QueueInLoop(std::bind(static_cast<void(Connection::*)(const char *,size_t)>(&Connection::SendInloop),this,data,size));
    }
}

void Connection::Send(const char *data,size_t size,std::string* strptr)        
{
    if (disconnect_==true) {  printf("客户端连接已断开了，send()直接返回。\n"); return;}

    if (loop_->IsInloopThread())   // 判断当前线程是否为事件循环线程（IO线程）。
    {
        // 如果当前线程是IO线程，直接调用sendinloop()发送数据。
        // printf("send() 在事件循环的线程中。\n");
        SendInloop(data,size,strptr);
    }
    else
    {
        // 如果当前线程不是IO线程，调用EventLoop::queueinloop()，把sendinloop()交给事件循环线程去执行。
        // printf("send() 不在事件循环的线程中。\n");
        loop_->QueueInLoop(std::bind(static_cast<void(Connection::*)(const char *,size_t,std::string*)>(&Connection::SendInloop),this,data,size,strptr));
    }

}
// 发送数据，如果当前线程是IO线程，直接调用此函数，如果是工作线程，将把此函数传给IO线程去执行。
void Connection::SendInloop(const char *data,size_t size)
{
    outputbuffer_.AppendWithSep(data,size);    // 把需要发送的数据保存到Connection的发送缓冲区中。
    clientchannel_->EnableWriting();    // 注册写事件。
}
void Connection::SendInloop(const char *data,size_t size,std::string* strptr)
{
    outputbuffer_.AppendWithSep(data,size,strptr);    // 把需要发送的数据保存到Connection的发送缓冲区中。
    clientchannel_->EnableWriting();    // 注册写事件。
}

// 处理写事件的回调函数，供Channel回调。
void Connection::WriteCallback()                   
{
    int writen=::send(Fd(),outputbuffer_.Data(),outputbuffer_.Size(),0);    // 尝试把outputbuffer_中的数据全部发送出去。
    if (writen>0) outputbuffer_.Erase(0,writen);                                        // 从outputbuffer_中删除已成功发送的字节数。

    // 如果发送缓冲区中没有数据了，表示数据已发送完成，不再关注写事件。
    if (outputbuffer_.Size()==0) 
    {
        clientchannel_->DisableWriting();        
        sendcompletecallback_(shared_from_this());
    }
}

 // 判断TCP连接是否超时（空闲太久）。
 bool Connection::Timeout(time_t now,int val)           
 {
    return now-lastatime_.ToInt()>val;    
 }

void Connection::StateMachineAppend(std::string& str)
{
    state_machine_.Append(str);
}

bool Connection::Parse()
{
    return state_machine_.Parse();
}

HTTPPacketInfo Connection::GetInfo()
{
    return state_machine_.GetInfo();
}