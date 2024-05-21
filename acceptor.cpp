#include "acceptor.h"

Acceptor::Acceptor(EventLoop* loop,const std::string &ip,const uint16_t port)
               :loop_(loop),servsock_(CreateNonblocking()),acceptchannel_(loop_,servsock_.Fd())
{
    InetAddress servaddr(ip,port);             // 服务端的地址和协议。
    servsock_.SetReuseaddr(true);
    servsock_.SetTcpnodelay(true);
    servsock_.SetReuseport(true);
    servsock_.SetKeepalive(true);
    servsock_.Bind(servaddr);
    servsock_.Listen();

    acceptchannel_.SetReadCallback(std::bind(&Acceptor::NewConnection,this));
    //在此处向mainloop注册读事件
    acceptchannel_.EnableReading();       // 让epoll_wait()监视servchannel的读事件。 
}

Acceptor::~Acceptor()
{
}

// 处理新客户端连接请求。
void Acceptor::NewConnection()    
{
    InetAddress clientaddr;             // 客户端的地址和协议。
    
    std::unique_ptr<Socket> clientsock(new Socket(servsock_.Accept(clientaddr)));
    clientsock->SetIPPort(clientaddr.ip(),clientaddr.port());

    newconnectioncb_(std::move(clientsock));        // 回调TcpServer::newconnection()。
                                                    //在把clientsock传到TcpServer类中创建Connection
                                                    //下面的fn由TcpServer确定
} 

//tcpserver.cpp line:11
void Acceptor::SetNewConnectioncb(std::function<void(std::unique_ptr<Socket>)> fn)       // 设置处理新客户端连接请求的回调函数。
{
    newconnectioncb_=fn;
}
