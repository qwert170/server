#include "Poller.h"
#include<cassert>
using namespace YTT;
using namespace YTT::net;

Poller::Poller(EventLoop *loop_) : epfd(epoll_create1(EPOLL_CLOEXEC)), loop(loop_), events_(36)
{
    if (epfd < 0)
    {
        LOG(ERROR) << "poller:epoll_create1" << endl;
    }
}

// Poller::poll()函数是Poller的核心功能，调用系统函数::poll()获得当前活动的IO事件，
// 然后填充调用方传入的activeChannels，并返回return的时刻。
Timestamp Poller::poll(int timeoutMs, channelist *activeChannels)
{
    int num = epoll_wait(epfd,    // 当前epoll占用的fd
                         &*events_.begin(),     // list数组第一个元素地址
                         static_cast<int>(events_.size()),   // list数组元素个数
                         timeoutMs);    // 超时时间
    int saveerr = errno;
    Timestamp now(Timestamp::now());
    if (num > 0)
    {
        LOG(TRACE) << num << " events happened" << endl;
        // 把有IO事件的channel加入到activeChannels中, EventLoop统一处理
        fillActiveChannels(num,activeChannels);
        if(num == events_.size())
        {
            events_.resize(num*2);
        }
    }
    else if (num == 0)
    {
        LOG(TRACE) << "nothing happened" << endl;
    }
    else if (num < 0)
    {
        LOG(ERROR) << endl;
    }
    return now;
}

void Poller::updatechannel(Channel *channel_)
{
    Poller::assertinloop();
    int status = channel_->getstatus();
    int fd = channel_->getfd();
    if (status == kNew || status == kDeleted)  //没在或者曾经在epoll队列中，添加到epoll队列中
    {
        if (status == kNew)   //没在epoll队列中
        {
            assert(channels.find(fd) == channels.end());
            channels[fd] = channel_;
        }
        else   // index == kDeleted  // 曾经在epoll队列中
        {
            assert(channels.find(fd) != channels.end());
        }
        channel_->setstatus(KAdded);    //修改index为已在队列中kAdded（1）
        update(EPOLL_CTL_ADD, channel_);   // 注册事件
    }
    else  //如果就在epoll队列中的，若没有关注事件了就暂时删除，如果有关注事件，就修改
    {
        assert(channels[fd] == channel_);
        if (channel_->isNoneEvent())
        {
            channel_->setstatus(kDeleted);    // channel标记kDeleted（2）暂时删除 
            update(EPOLL_CTL_DEL, channel_);  // 删除事件
        }
        else
        {
            update(EPOLL_CTL_MOD, channel_);    // 更新事件
        }
    }
    return;
}

// 从std::vector<struct epoll_event> events_中移除Channel，并删除对应的事件
void Poller::removechannel(Channel *channel_)
{
    Poller::assertinloop();
    int fd = channel_->getfd();
    int status = channel_->getstatus();
    assert(channels[fd] == channel_);
    int n = channels.erase(fd);
    assert(n == 1);
    if (status == KAdded)
    {
        update(EPOLL_CTL_DEL, channel_); // 从epoll队列中删除这个channel
    }
}

//注册、删除事件核心
void Poller::update(int operation, Channel *channel_)
{
    struct epoll_event event;
    explicit_bzero(&event, sizeof(event));
    event.events = channel_->getevent();
    event.data.ptr = channel_;   // 注意，event.data.ptr被赋值指向当前指针channel
    int fd = channel_->getfd();
    if (epoll_ctl(epfd, operation, fd, &event) < 0)
    {
        LOG(ERROR) << "Epoller::update epoll_ctl the socket fd is " << fd
                                                                          << "and the event is "
                                                                          << (event.events & EPOLLIN ? "EPOLLIN" : " ")
                                                                          << (event.events & EPOLLOUT ? "EPOLLOUT" : " ") << endl;
    }
}

bool  Poller::haschannel(Channel *channel)
{   
    ChannelMap::iterator it = channels.find(channel->getfd());
    return it!=channels.end();
}

void Poller::assertinloop()
{
    loop->assertInLoopThread();
}

// 遍历events_，找出当前有IO事件就绪的fd，把它对应的channel填入到activeChannels中
// 当前活动事件events保存在channle中，供EvenetLoop中调用Channel::handleEvent()使用
/* 注意：遍历events_[i]时，通过成员data.ptr得到对应的channel，
   是因为在注册调用update()时events_的成员data.ptr被赋值为当前channel */
void Poller::fillActiveChannels(int numEvents,
                        channelist *activeChannels) const
{
    for(int i = 0; i < numEvents; i++)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);  // 获取当前就绪IO事件对应的channel
        int fd = channel->getfd();
        channel->setevent(events_[i].events);   // 设置channel当前就绪的IO事件
        activeChannels->push_back(channel);   // 就绪IO事件channe放入activeChannels
    }
}