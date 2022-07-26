#ifndef LOGFILE_H_
#define LOGFILE_H_

#include "noncopyable.h"
#include "FileUtil.h"
#include "Timestamp.h"
#include <memory>
#include <mutex>
#include <string>
#include <ctime>

namespace YTT
{
    class FileAdd;
    class LogFile : noncopyable
    {
    public:
        LogFile(const std::string &name_, off_t rollsize_, bool threadsafe_ = true, int flushtime_ = 60, int checkevery_ = 1024) : basename(name_), rollsize(rollsize_), threadsafe(threadsafe_), flushtime(flushtime_), checkevery(checkevery_), count(0), mutex_(threadsafe ? new std::mutex : nullptr),
                                                                                                                      lastday(0), lastRoll(0), lastFlush(0) { rollfile(); }
         ~LogFile() = default;
        void append(const char *log, int len);
        void flush();
        bool rollfile();
        std::string addtime(const std::string filename, time_t *t);
    private:
        void unlock_append(const char *log, int len);
        const std::string basename;
        const off_t rollsize;   // 超过滚动大小将触发滚动
        const bool threadsafe;
        const int flushtime;    // 超过刷新间隔将触发刷新到磁盘
        const int checkevery;   // append的次数count_ 超过 checkEveryN_ 将检验时间是否应触发滚动
        int count;              // append的次数
        std::unique_ptr<std::mutex> mutex_;
        time_t lastday;
        time_t lastRoll;        // 最后一次滚动的时间
        time_t lastFlush;       // 最后一次刷新的时间
        std::unique_ptr<FileAdd> file;
        const static int daysecond = 60 * 60 * 24;
    };
}

#endif