#include "EventLoopThread.h"

using namespace YTT;
using namespace YTT::net;

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb)
    : loop_(NULL),    // 指向当前线程创建的EventLoop对象
      exiting_(false),     // 退出线程标志位
      fut(pro.get_future()), 
      thread_(std::bind(&EventLoopThread::threadFunc, this)),  // 线程
      callback_(cb)    // 线程初始化的回调函数
{
    if (!loop_)
    {
        fut.get();
    }
}

// 析构，退出事件循环，关闭线程
EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop *EventLoopThread::startloop()
{
    {   
        fut = pro.get_future();
        fut.get();
        return loop_;
    }
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;     // 线程中创建一个EventLoop对象
    if (callback_)
    {
        callback_(&loop);   // 执行初始化回调函数
    }
    loop_ = &loop;
    pro.set_value(true);
}