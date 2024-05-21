#pragma once 
#include <functional>
#include "epoll.h"
#include <memory>
#include <unistd.h>
#include <queue>
#include <mutex>
#include <map>
#include <atomic>
#include <sys/eventfd.h>
#include <sys/syscall.h>
#include <sys/timerfd.h>      // 定时器需要包含这个头文件。

class Channel;
class Epoll;
class Connection;
using spConnection=std::shared_ptr<Connection>;

// 事件循环类。
// 在Tcpserver类中初始化，在其他线程中运行run函数
class EventLoop
{
private:
    int  timetvl_;                                                              // 闹钟时间间隔(interval)，单位：秒。。
    int  timeout_;                                                              // Connection对象超时的时间，单位：秒。
    std::unique_ptr<Epoll> ep_;                                                 // 每个事件循环只有一个Epoll。
    pid_t threadid_;                                                            // 事件循环所在线程的id。
    std::queue<std::function<void()>> taskqueue_;                               // 事件循环线程被eventfd唤醒后执行的任务队列。
    std::mutex mutex_;                                                          // 任务队列同步的互斥锁。
    int wakeupfd_;                                                              // 用于唤醒事件循环线程的eventfd。
    std::unique_ptr<Channel> wakechannel_;                                      // eventfd的Channel。
    int timerfd_;                                                               // 定时器的fd。
    std::unique_ptr<Channel> timerchannel_;                                     // 定时器的Channel。
    bool mainloop_;                                                             // true-是主事件循环，false-是从事件循环。
    std::mutex mmutex_;                                                         // 保护conns_的互斥锁。
    std::map<int,spConnection> conns_;                                          // 存放运行在该事件循环上全部的Connection对象。
    std::function<void(EventLoop*)> epolltimeoutcallback_;                      // epoll_wait()超时的回调函数，在Tcpserver构造函数中被设置为TcpServer::EpollTimeout()
    std::function<void(int)>  timercallback_;                                   // 删除TcpServer中超时的Connection对象，在Tcpserver构造函数中将被设置为TcpServer::removeconn()
    std::atomic_bool stop_;                                                     // 初始值为false，如果设置为true，表示停止事件循环。

public:
    EventLoop(bool mainloop,int timetvl=30,int timeout=80);                     // 在构造函数中创建Epoll对象ep_。
    ~EventLoop();               

    void Run();                                                                 // 运行事件循环。
    void Stop();                                                                // 停止事件循环。

    void UpdateChannel(Channel *ch);                                            // 把channel添加/更新到红黑树上，channel中有fd，也有需要监视的事件。
    void RemoveChannel(Channel *ch);                                            // 从黑树上删除channel。
    void SetEpollTimeoutCallback(std::function<void(EventLoop*)> fn);           // 设置epoll_wait()超时的回调函数。

    bool IsInloopThread();                                                      // 判断当前线程是否为事件循环线程。

    void QueueInLoop(std::function<void()> fn);                                 // 把任务添加到队列中。
    void Wakeup();                                                              // 用eventfd唤醒事件循环线程。
    void HandleWakeup();                                                        // 事件循环线程被eventfd唤醒后执行的函数。

    void HandleTimer();                                                         // 闹钟响时执行的函数。

    void NewConnection(spConnection conn);                                      // 把Connection对象保存在conns_中。
    void SetTimerCallback(std::function<void(int)> fn);                         // 将被设置为TcpServer::removeconn()
};