#ifndef ACCEPTOR_H_
#define ACCEPTOR_H_

#include "noncopyable.h"
#include "InetAddress.h"
#include "Channel.h"
#include "Socket.h"
#include <atomic>

namespace YTT
{
    namespace net
    {
        class Socket;
        class Channel;
        using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &address)>;
        class Acceptor : noncopyable
        {
        public:
            Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
            ~Acceptor();
            void setConnectback(const NewConnectionCallback &cb)
            {
                NewConnection_ = cb;     // 设置新连接建立的回调函数
            }
            void listen();   // 开启监听
            bool listening() const { return listenning_; }    // 返回当前监听状态

        private:
            int fd_;       // 空闲的描述符
            bool listenning_;
            EventLoop *loop;    // 基本是main reactor
            Socket acceptSocket_;     // 用于接收新连接的scoket封装
            Channel acceptChannel_;     // 封装acceptSocket_的channel，监听其上的事件
            void handleRead();
            NewConnectionCallback NewConnection_;
        };
    }
}

#endif