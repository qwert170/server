#include "Logging.h"
#include "Thread.h"
#include "LogStream.h"
#include <errno.h>

using namespace YTT;

static Logger::LogLevel loglev=lev::TRACE;
__thread char errbuf[512];
thread_local std::string times;
__thread time_t lastime;   // 当前线程上一次日志记录时的秒数
__thread pid_t tid;

const char *strerror_mr(int err)
{
    return strerror_r(err, errbuf, sizeof(errbuf));
}

const char *LogLevelStr[Logger::NUM_LOG_LEVELS] = {
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL",
};

void dafaultoutput(const char *msg, int len)
{
    fwrite(msg, 1, len, stdout);   // 默认写出到stdout
}

void dafaultflush()
{
    fflush(stdout);    // 默认flush到stdout
}

Logger::OutputFunc g_output = dafaultoutput;
Logger::FlushFunc g_flush = dafaultflush;
Logger::Logger(SourceFile file_, int line_, const char *func_, int logerrno, LogLevel level_) 
    : time(Timestamp::now()),    //登记当前的时间
      basename(file_),           //文件名称
      line(line_),               //行
      func(func_),
                                                                                                errnos(logerrno)
{
    setLogLevel(level_);
    formatTime();         //格式化时间
    Showerr(errno,func);
    streams << (tid ? tid : (tid = gettid())) << " " << LogLevelStr[level] << " ";
    streams << strerror_mr(errnos) << YTT::endl;
}

Logger::~Logger()
{
    streams << " - " << basename.data << ':' << line << YTT::endl;
    const LogStream::Buffer &buf(streams.getBuf());   // 获取缓冲区
    g_output(buf.begin(), buf.sizes());   // 默认输出到stdout
    // 当日志级别为FATAL时，flush设备缓冲区并终止程序
    if (level == LogLevel::FATAL)
    {
        g_flush();
        abort();
    }
}

void Logger::formatTime()
{
    time_t misecond = time.seconds();   //获得微秒格式的时间
    if (misecond != lastime)
    {
        lastime = misecond;
        times = std::move(time.toFormattedString(true));
    }
    streams << times << " ";
}

void Logger::setLogLevel(LogLevel level_)
{
    loglev = level_;
}

void Logger::setOutput(OutputFunc t)
{
    g_output = t;
}

void Logger::setFlush(FlushFunc t)
{
    g_flush = t;
}