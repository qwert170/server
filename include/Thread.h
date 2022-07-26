#ifndef THREAD_H_
#define THREAD_H_

// 标记当前当前线程的名称

#include "noncopyable.h"
#include <atomic>
#include <cassert>
#include <functional>
#include <future>
#include <thread>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/types.h>

namespace YTT
{
    class Thread : noncopyable
    {
        typedef std::function<void()> threadfunc;
    public:
        explicit Thread(threadfunc &&th, const std::string &&s);
        ~Thread();
        int joins(); // 阻塞地等待回调函数执行结束
        // 内部使用pthread_create()创建线程，使用CountDownLatch来等到回调函数进入初始化成功；
        void starts();
        pthread_t *thr;
        bool start;    // 判断线程是否正在执行中
        bool join;
    private:
        pid_t tid;    // 线程tid
        threadfunc func;   // 线程回调函数
        std::string name;  // 线程名称
        std::promise<void> pro;
        static std::atomic<int> num_thread;  // 当前已经创建线程的数量
    };
    pid_t gettid();  // 返回当前线程tid
}

#endif