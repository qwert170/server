#ifndef TIMESTAMP_H_
#define TIMESTAMP_H_

#include "noncopyable.h"
#include <sys/time.h>
#include <unistd.h>
#include <boost/operators.hpp>

namespace YTT
{
    class Timestamp : public boost::less_than_comparable1<Timestamp>
    {
    public:
        Timestamp() : ms(0) {}
        explicit Timestamp(int64_t time_) : ms(time_) {}
        ~Timestamp() = default;
        static Timestamp now();
        std::string toString() const;
        std::string toFormattedString(bool showMicroseconds) const;
        int64_t getSeconds() const { return ms; }
        time_t seconds() const { return static_cast<time_t>(ms/s2time); }
        void swap(Timestamp &a, Timestamp &b)
        {
            std::swap(a, b);
        }
        static const int s2time = 1000 * 1000;
    private:
        int64_t ms;
    };
inline bool operator<(Timestamp a, Timestamp b)
{
    return a.getSeconds() < b.getSeconds();
}

inline bool operator==(Timestamp a, Timestamp b)
{
    return a.getSeconds() == b.getSeconds();
}

inline double alltime(Timestamp front, Timestamp end)
{
    return static_cast<double>((front.getSeconds() - end.getSeconds()) / Timestamp::s2time);
}

inline Timestamp time_add(Timestamp enter, double s)
{
    time_t ns = static_cast<time_t>(s * Timestamp::s2time);
    return Timestamp(enter.getSeconds() + ns);
}
}

#endif