#ifndef LOGGING_H_
#define LOGGING_H_

#include "LogStream.h"
#include "Timestamp.h"
#include <functional>
const char *strerror_mr(int err);
class TimeZone;

namespace YTT
{       
    LogStream &endl(LogStream &stream);
    class Logger
    {
    public:
        Logger();
        enum LogLevel
        {
            TRACE,
            DEBUG,
            INFO,
            WARN,
            ERROR,
            FATAL,
            NUM_LOG_LEVELS,
        };

        // 编译器计算源文件名
        class SourceFile
        {
        public:
            template <int N>
            SourceFile(const char (&a)[N]) : data(a), size(N - 1)
            {
                // char *strrchr(const char *str, int c) 在参数 str 所指向的字符串中
                // 搜索最后一次出现字符 c（一个无符号字符）的位置。
                // 该函数返回 str 中最后一次出现字符 c 的位置。如果未找到该值，则函数返回一个空指针。
                const char *t = strrchr(data, '/');
                if (t)
                {
                    data = t + 1;
                    size -= static_cast<int>(data - a);
                }
            }

            SourceFile(const char *filename) : data(filename)
            {
                const char *t = strrchr(data, '/');
                if (t)
                {
                    data = t + 1;
                    size = static_cast<int>(strlen(data));
                }
            }
            const char *data;
            int size;
        };

        // 构造函数，实际是用于初始化 Impl 类
        Logger(SourceFile file_, int line_, const char *func_, int logerrno, LogLevel level_ = INFO);
        // 析构函数， flush输出
        ~Logger();
        using FlushFunc = std::function<void()>;
        using OutputFunc = std::function<void(const char *msg, int len)>;
        static LogLevel getlevel() { return level; }
        LogStream &getstream() { return streams; }
        static void setLogLevel(LogLevel level_);
        static void setOutput(OutputFunc);  // 默认 fwrite 到 stdout
        static void setFlush(FlushFunc);    // 默认 fflush 到 stdout
        static void setTimeZone(const TimeZone &tz);   // 默认 GMT
        void formatTime();   // 格式化时间
        Timestamp time;      // 日志时间戳
        LogStream streams;   // 日志缓存流
        static LogLevel level;   // 日志级别
        size_t line;         // 当前记录日式宏的 源代码行数
        SourceFile basename; // 当前记录日式宏的 源代码名称
        const char *func;
        int errnos;
    };

        // 全局的日志级别，静态成员函数定义，静态成员函数实现
    extern Logger::LogLevel loglev;
    using lev = Logger::LogLevel;
#define LOG(level)       \
    if (YTT::Logger::level >= YTT::loglev) \
        YTT::Logger(__FILE__, __LINE__, __func__, (YTT::Logger::level == YTT::Logger::ERROR || YTT::Logger::level == YTT::Logger::FATAL) ? errno : 0, YTT::Logger::level)  \
        .streams

#define Showerr(err,str)  \
    if (err)   \
        fprintf(stderr, "[ERROR]: %s %s", str, strerror(errno));
}

#endif