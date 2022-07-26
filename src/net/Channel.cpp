#include "Channel.h"
#include "Poller.h"
#include <sstream>

using namespace YTT;
using namespace YTT::net;

static const int kNoneEvent = EPOLLET;
static const int kReadEvent = EPOLLIN | EPOLLRDHUP | EPOLLPRI;
static const int kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop_, int fd_) : loop(loop_), recvevents_(kNoneEvent), fd(fd_), status(kNew), tied_(false) {}

Channel::~Channel() {}

// 把当前的channel加入到poll队列当中，或者更新fd的监听事件
void Channel::update()
{

    loop->updateChannel(this);
}

// 先关闭fd上注册的事件，并从其所属的EventLoop的Poller中移除当前channel
void Channel::remove()
{

    loop->removeChannel(this);    // 从EvenetLoop的Poller中删除当前channel
}

void Channel::tie(const std::shared_ptr<void> &pt)
{
    tie_ = pt;
    tied_ = true;
}

// 事件分发。由EventLoop::loop()调用，根据revents_的值分别调用不同的用户回调函数。
void Channel::handleEvent(Timestamp recvtime)   //Timestamp用于读事件的回调函数的参数
{
    std::shared_ptr<void> pt;
    if (tied_)   // 提升tie_为shared_ptr，如果提升成功，说明指向一个存在的对象
    {
        pt = tie_.lock();
        if (pt)
        {
            handleEVentwithguard(recvtime);
        }
    }
    else
    {
        handleEVentwithguard(recvtime);
    }
}

// 查看epoll/或者poll返回的具体是什么事件，并根据事件的类型进行相应的处理
void Channel::handleEVentwithguard(Timestamp recvtime)
{
    LOG(TRACE) << reventsToString();
    // 当事件为挂起并没有可读事件时
    if ((recvevents_ & EPOLLHUP) && !(recvevents_ & EPOLLIN))
    {
        if (closeCallback_)
            closeCallback_();
    }

    if (recvevents_ & POLLNVAL)   // 描述字不是一个打开的文件描述符
    {
        LOG(WARN) << "fd = " << fd << " Channel::handle_event() POLLNVAL";
    }

    if (recvevents_ & (EPOLLERR | POLLNVAL))   // 发生错误或者描述符不可打开
    {
        if (errorCallback_)
            errorCallback_();
    }
    if (recvevents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP))  // 关于读的事件 
    {
        if (readCallback_)
            readCallback_(recvtime);
    }
    if (recvevents_ & EPOLLOUT)   // 关于写的事件
    {
        if (writeCallback_)
            writeCallback_();
    }
}
std::string Channel::reventsToString()
{
    std::ostringstream os;
    os << fd << ":";
    if (events_ & EPOLLIN)
        os << "IN ";
    if (events_ & EPOLLPRI)
        os << "PRI ";
    if (events_ & EPOLLOUT)
        os << "OUT ";
    if (events_ & EPOLLHUP)
        os << "HUP ";
    if (events_ & EPOLLRDHUP)
        os << "RDHUP ";
    if (events_ & EPOLLERR)
        os << "ERR ";
    if (events_ & POLLNVAL)
        os << "NVAL ";
    return os.str();
}