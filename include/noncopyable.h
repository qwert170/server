#ifndef NONCOPYABLE_H_
#define NONCOPYABLE_H_

namespace YTT
{
    class noncopyable
    {
    public:
        noncopyable(const noncopyable &) = delete;
        noncopyable operator=(const noncopyable&) = delete;
        noncopyable() = default;
        ~noncopyable() = default;
    };
}

#endif