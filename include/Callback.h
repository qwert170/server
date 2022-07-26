#ifndef CALLBACK_H
#define CALLBACK_H

#include "Timestamp.h"
#include <functional>
#include <memory>

namespace YTT
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    namespace net
    {
        class Buffer;
        class TcpConnection;
        using Tcpconptr = std::shared_ptr<TcpConnection>;
        using Timercallback = std::function<void()>;
        using ConnectionCallback = std::function<void(const Tcpconptr &)>;
        using CloseCallback = std::function<void(const Tcpconptr &)>;
        using WriteCompleteCallback = std::function<void(const Tcpconptr &)>;
        using MessageCallback = std::function<void(const Tcpconptr &, Buffer *, Timestamp)>;
        void defaultConnectionCallback(const Tcpconptr &conn);
        void defaultMessageCallback(const Tcpconptr &conn, Buffer *buffer, Timestamp receiveTime);
    }
}
#endif