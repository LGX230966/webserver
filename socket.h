#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "inetaddress.h"

// 创建一个非阻塞的socket。
int CreateNonblocking();

// socket类。
class Socket
{
private:
    const int fd_;                 // Socket持有的fd，在构造函数中传进来。
    std::string ip_;               // 如果是listenfd，存放服务端监听的ip，如果是客户端连接的fd，存放对端的ip。
    uint16_t port_;              // 如果是listenfd，存放服务端监听的port，如果是客户端连接的fd，存放外部端口。

public:
    Socket(int fd);             // 构造函数，传入一个已准备好的fd。
    ~Socket();                   // 在析构函数中，将关闭fd_。

    int Fd() const;                              // 返回fd_成员。
    std::string Ip() const;                   // 返回ip_成员。
    uint16_t Port() const;                  // 返回port_成员。
    void SetIPPort(const std::string &ip,uint16_t port);   // 设置ip_和port_成员。
    
    void SetReuseaddr(bool on);       // 设置SO_REUSEADDR选项，true-打开，false-关闭。
    void SetReuseport(bool on);       // 设置SO_REUSEPORT选项。
    void SetTcpnodelay(bool on);     // 设置TCP_NODELAY选项。
    void SetKeepalive(bool on);       // 设置SO_KEEPALIVE选项。
    void Bind(const InetAddress& servaddr);        // 服务端的socket将调用此函数。
    void Listen(int nn=128);                                    // 服务端的socket将调用此函数。
    int Accept(InetAddress& clientaddr);            // 服务端的socket将调用此函数。
};