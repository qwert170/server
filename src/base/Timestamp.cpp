#include "Timestamp.h"
#include <chrono>

using namespace YTT;

// 这个函数的作用是将microSecondsSinceEpoch_转化为秒数和微秒数的组合，
// 然后使用snprintf转成一个符合格式的字符串。
std::string Timestamp::toString() const
{
    char buf[32] = {0};
    int64_t a = ms % s2time;
    int64_t b = ms / s2time;
    snprintf(buf, sizeof(buf), "%ld:%ld", a, b);
    return buf;
}

// 将microSecondsSinceEpoch_转化为年月日分秒的字符串形式
std::string Timestamp::toFormattedString(bool showMicroseconds) const
{
    char buf[64] = {0};
    struct tm time_;
    time_t a = static_cast<time_t>(ms % s2time);
    time_t b = ms / s2time;
    gmtime_r(&a, &time_);
    if (showMicroseconds) {
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
                 time_.tm_year + 1900, time_.tm_mon + 1, time_.tm_mday,
                 time_.tm_hour, time_.tm_min, time_.tm_sec,
                 time);
    } else {
        snprintf(buf, sizeof(buf), "%4d%02d%02d ",
                 time_.tm_year + 1900, time_.tm_mon + 1, time_.tm_mday);
    }
    return buf;
}

// 当前时间戳
Timestamp Timestamp::now()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);  //获得当前时间，第二个参数是时区，不需要返回时区填指针
    return Timestamp(tv.tv_sec*Timestamp::s2time + tv.tv_usec);
}