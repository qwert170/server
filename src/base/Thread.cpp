#include "Thread.h"

namespace YTT
{
    Thread::Thread(threadfunc &&th, const std::string &&s) : thr(nullptr), tid(0), start(false), join(false), name(s), func(std::move(th))
    {
        num_thread++;
        starts();
    }
    Thread::~Thread() {}
    class ThreadData
    {
    public:
        typedef std::function<void()> ThreadFunc;
        ThreadData(ThreadFunc &&func, std::string &&name, std::promise<void> &pro, pid_t &tid)
              : func_(func), name_(name), pro_(pro), tid_(tid) {}
        ~ThreadData() {}
        ThreadFunc func_;
        std::string name_;
        pid_t tid_;
        std::promise<void> &pro_;
    };
    void *run(void *args)
    {
        ThreadData *t(static_cast<ThreadData *> (args));
        t->pro_.set_value();
        try
        {
            t->func_();  //执行用户传入的回调函数
        }
        catch (const std::exception e)
        {
            fprintf(stderr, "exception caught in Thread %s\n", t->name_.c_str());
            fprintf(stderr, "reason: %s\n", e.what());
        }
        return NULL;
    }

    pid_t gettid()
    {
        return syscall(SYS_gettid);
    }

    void Thread::starts()
    {
        ThreadData *data = new ThreadData(std::move(func), std::move(name), pro, tid);
        std::future<void> s = pro.get_future();
        
        if (pthread_create(thr, nullptr, run, static_cast<void *>(data))) {
        } else {
            s.get();
            start = true;
        }
    }
    
    int Thread::joins()
    {
        assert(!join);
        join = true;
        return pthread_join(tid, nullptr);
    }
}