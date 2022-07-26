#ifndef ASYNCLOGGING_H_
#define ASYNCLOGGING_H_

#include "Logging.h"
#include "LogFile.h"
#include "noncopyable.h"
#include <vector>
#include <atomic>
#include <future>

namespace YTT
{
    class AsyncLogging : noncopyable
    {
    public:
        AsyncLogging(const std::string &basename,
                    off_t rollSize,
                    size_t flushInterval = 3);

        ~AsyncLogging();
        void append(const char *logline, int len);
        void start();
        void stop();

    private:
        void threadFunc();
        using buffer = LogBuffer<ksmallbuffer>;
        using bufPtr = std::unique_ptr<LogBuffer<ksmallbuffer>>;
        using bufVector = std::vector<bufPtr>;    // 待写入文件的已填满缓冲，供后端写入

    private:
        std::thread td_;
        mutable std::mutex m_;
        std::condition_variable cv_;
        std::promise<void> p_;
        std::atomic<bool> running_;
        size_t roll_size_;
        const size_t flushInterval_;
        size_t writeInterval_;
        bufPtr cur_;
        bufPtr prv_;
        bufVector tranbufvector;
        std::string logFileName_;
    };
}

#endif