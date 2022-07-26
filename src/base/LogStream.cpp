#include "LogStream.h"
using namespace YTT;

LogStream &endl(LogStream &stream)
{
    if (stream.getBuf().avail() >= 1)
    {
        stream.getBuf().append('\n');
    }
    return stream;
}

// 整形数据流式操作方法的实际处理函数
template <typename T>
void LogStream::formatInteger(T v)
{
    auto s = std::to_string(v);
    auto len = s.size();
    if (len <= buf_.avail()) buf_.append(s, len);
}

LogStream &LogStream::operator<<(short v)
{
    return *this << static_cast<int>(v);
}

LogStream &LogStream::operator<<(unsigned short v)
{
    return *this << static_cast<unsigned int>(v);
}

LogStream &LogStream::operator<<(int v)
{
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned int v)
{
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(long v)
{
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned long v)
{
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(long long v)
{
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(unsigned long long v)
{
    formatInteger(v);
    return *this;
}

LogStream &LogStream::operator<<(const void *v)
{
    if (32 <= buf_.avail())
    {
        int len = snprintf(buf_.stringend(), 32, "%p", v);
        buf_.addlen(len);
    }
    return *this;
}

LogStream &LogStream::operator<<(float v)
{
    return *this << static_cast<double>(v);
    return *this;
}

LogStream &LogStream::operator<<(double v)
{
    if (32 <= buf_.avail())
    {
        int len = snprintf(buf_.stringend(), 32, "%p", v);
        buf_.addlen(len);
    }
    return *this;
}