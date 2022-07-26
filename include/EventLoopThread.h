#ifndef ECENTLOOPTHREAD_H_
#define ECENTLOOPTHREAD_H_

#include <thread>
#include <future>
#include "EventLoop.h"

namespace YTT
{
    namespace net
    {
        class EventLoop;
        class EventLoopThread
        {
        public:
            using ThreadInitCallback = std::function<void(EventLoop *)>;
            EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback());
            ~EventLoopThread();
            EventLoop *startloop();
        private:
            EventLoop *loop_;
            std::thread thread_;
            std::promise<bool> pro;
            std::future<bool> fut;
            std::atomic<bool> exiting_;
            void threadFunc();
            ThreadInitCallback callback_;
        };
    }
}

#endif