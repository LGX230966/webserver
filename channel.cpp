#include "channel.h"

Channel::Channel(EventLoop* loop,int fd):loop_(loop),fd_(fd)      // 构造函数。
{

}

Channel::~Channel()                           // 析构函数。 
{
    // 在析构函数中，不要销毁loop_，也不能关闭fd_，因为这两个东西不属于Channel类，Channel类只是需要它们，使用它们而已。
}

int Channel::Fd()                                            // 返回fd_成员。
{
    return fd_;
}

void Channel::UseET()                                    // 采用边缘触发。
{
    events_=events_|EPOLLET;
}

void Channel::EnableReading()                     // 让epoll_wait()监视fd_的读事件。
{
    events_|=EPOLLIN;
    loop_->UpdateChannel(this);
}

void Channel::DisableReading()                    // 取消读事件。
{
    events_&=~EPOLLIN;
    loop_->UpdateChannel(this);
}

void Channel::EnableWriting()                      // 注册写事件。
{
    events_|=EPOLLOUT;
    loop_->UpdateChannel(this);
}

void Channel::DisableWriting()                     // 取消写事件。
{
    events_&=~EPOLLOUT;
    loop_->UpdateChannel(this);
}

void Channel::DisableAll()                             // 取消全部的事件。
{
    events_=0;
    loop_->UpdateChannel(this);
}

void Channel::Remove()                                // 从事件循环中删除Channel。
{
    DisableAll();                                // 先取消全部的事件。
    loop_->RemoveChannel(this);    // 从红黑树上删除fd。
}

void Channel::SetInepoll(bool inepoll)                           // 设置inepoll_成员的值。
{
    inepoll_=inepoll;
}

void Channel::SetRevents(uint32_t ev)         // 设置revents_成员的值为参数ev。
{
    revents_=ev;
}

bool Channel::Inpoll()                                  // 返回inepoll_成员。
{
    return inepoll_;
}

uint32_t Channel::Events()                           // 返回events_成员。
{
    return events_;
}

uint32_t Channel::Revents()                          // 返回revents_成员。
{
    return revents_;
} 

// 事件处理函数，epoll_wait()返回的时候，执行它。
void Channel::HandleEvent()
{
    if (revents_ & EPOLLRDHUP)                     // 对方已关闭，有些系统检测不到，可以使用EPOLLIN，recv()返回0。
    {
        closecallback_();      // 回调Connection::closecallback()。
    }                               
    else if (revents_ & (EPOLLIN|EPOLLPRI))   // 接收缓冲区中有数据可以读。
    {
        readcallback_();   // 如果是acceptchannel，将回调Acceptor::newconnection()，如果是clientchannel，将回调Connection::onmessage()。
    }  
    else if (revents_ & EPOLLOUT)                  // 有数据需要写。
    {
        writecallback_();      // 回调Connection::writecallback()。     
    }
    else                                                           // 其它事件，都视为错误。
    {
        errorcallback_();       // 回调Connection::errorcallback()。
    }
}

 // 设置fd_读事件的回调函数。
 void Channel::SetReadCallback(std::function<void()> fn)    
 {
    readcallback_=fn;
 }

 // 设置关闭fd_的回调函数。
 void Channel::SetCloseCallback(std::function<void()> fn)    
 {
    closecallback_=fn;
 }

 // 设置fd_发生了错误的回调函数。
 void Channel::SetErrorCallback(std::function<void()> fn)    
 {
    errorcallback_=fn;
 }

 // 设置写事件的回调函数。
 void Channel::SetWriteCallback(std::function<void()> fn)   
 {
    writecallback_=fn;
 }
