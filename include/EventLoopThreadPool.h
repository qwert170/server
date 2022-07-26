#ifndef EVENTLOOPTHREADPOOL_H_
#define EVENTLOOPTHREADPOOL_H_

#include "EventLoopThread.h"
#include "noncopyable.h"
#include <memory>
#include <vector>
#include <functional>

namespace YTT
{
    namespace net
    {
        class EventloopThreadpool : noncopyable
        {
        public:
            using ThreadInitback = EventLoopThread::ThreadInitCallback;
            EventloopThreadpool(EventLoop* baseLoop);
            ~EventloopThreadpool() = default;
            // 启动后有效，使用round-robin策略获取线程池中的EventLoop对象
            EventLoop* getNextLoop();
            std::vector<EventLoop*> getAllLoops();
            // 设置线程池数量
            void setThreadNum(int numThreads) { numThreads_ = numThreads; }
            // 启动线程池
            void start(const ThreadInitback &cb = ThreadInitback());
            bool started() const { return started_; }
        private:
            int numThreads_;     // 线程数
            int next_;        // 新连接到来，所选择的EventLoop对象下标
            EventLoop* baseLoop_;    // Acceptor所属EventLoop
            std::atomic<bool> started_;     // 是否已经启动
            std::vector<std::unique_ptr<EventLoopThread>> threads_;    // IO线程列表
            std::vector<EventLoop*> loops_;    // EventLoop列表
        };
    }
}

#endif