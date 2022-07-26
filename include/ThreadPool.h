#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <deque>
#include <vector>
#include "noncopyable.h"

namespace YTT
{
    class ThreadPool : noncopyable
    {
    public:
        typedef std::function<void()> Task;
        explicit ThreadPool(const std::string &nameArg = std::string("ThreadPool"));
        ~ThreadPool();
        // 设置任务数量上限
        void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }
        // 线程初始化后的操作
        void setThreadInitCallback(const Task &cb)
        {
            threadInitCallback_ = cb;
        }
        // 设置线程池中线程数量，并启动线程池
        void start(int numThreads);
        void stop();  // 关闭线程池
        const std::string &name() const
        {
            return name_;
        }
        size_t queueSize() const;
        void run(Task f);
    private:
        bool isFull() const;
        // 线程池中每个线程运行函数（不断从队列获取任务并执行）
        void runInThread();
        Task take();   // 从队列中取出一个任务

        mutable std::mutex mutex_;
        std::condition_variable notFull_;
        std::condition_variable notEmpty_;
        std::string name_;
        Task threadInitCallback_;
        // 线程池的实现，用vector存储每个线程的智能指针
        std::vector<std::unique_ptr<std::thread>> threads_;
        std::deque<Task> queue_;    // 使用std::deques双端队列实现的任务队列
        size_t maxQueueSize_;
        bool running_;
    };
}

#endif