#include "EventLoopThreadPool.h"

using namespace YTT;
using namespace YTT::net;

EventloopThreadpool::EventloopThreadpool(EventLoop *baseLoop) : baseLoop_(baseLoop),
                                                                started_(false), numThreads_(0), next_(0) {}
                                                                
EventLoop *EventloopThreadpool::getNextLoop()
{
    baseLoop_->assertInLoopThread();
    EventLoop *loop = NULL;
    if (!loops_.empty())  // 假设不为空，依照round-robin的调度方式选择一个EventLoop
    {
        loop = loops_[next_];
        ++next_;
        next_ %= loops_.size();
        return loop;
    }
}

void EventloopThreadpool::start(const ThreadInitback &cb)
{
    assert(!started_);
    baseLoop_->assertInLoopThread();
    started_ = true;
    for (int i = 0; i < numThreads_; i++)
    {
        // 启动EventLoopThread线程。在进入事件循环之前。会调用cb
        EventLoopThread* t = new EventLoopThread(cb);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startloop());
    }
    if (numThreads_ == 0 && cb)
        cb(baseLoop_);   // 仅仅有一个EventLoop。在这个EventLoop进入事件循环之前，调用cb
}

std::vector<EventLoop *> EventloopThreadpool::getAllLoops()
{
    baseLoop_->assertInLoopThread();
    if (loops_.empty())
    {
        return std::vector<EventLoop *>(1, baseLoop_);
    }
    else
    {
        return loops_;
    }
}