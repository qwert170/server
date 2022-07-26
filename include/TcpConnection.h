#ifndef TCPCONNECTION_H_
#define TCPCONNECTION_H_

#include "noncopyable.h"
#include "Callback.h"
#include "Timestamp.h"
#include "Channel.h"
#include "InetAddress.h"
#include "Buffer.h"

namespace YTT
{
    namespace net
    {
        // class EventLoop;
        // class Channel;
        // class Socket;
        class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
        {
        public:
            TcpConnection(EventLoop *loop, int sockfd, const std::string &nameArg,
                          const InetAddress &localAddr, const InetAddress &peerAddr);
            ~TcpConnection();

            // 获取当前TcpConnetction的一些属性、状态
            EventLoop *getLoop() const { return loop_; }
            const InetAddress &localAddress() const { return localAddr_; }
            const InetAddress &peerAddress() const { return peerAddr_; }
            bool connected() const { return state_ == kConnected; }
            bool disconnected() const { return state_ == kDisconnected; }
            /// 提供上层调用的发送接口，直接发送或保存在Buffer中
            void send(const void *message, int len);
            void send(std::string &message);
            void sendFile(int fd, int fileSize);
            void send(Buffer *message);
            // 关闭连接，设置TCP选项
            void shutdown();
            void shutdownInLoop();
            void forceClose();
            void setTcpNoDelay(bool on);
            // 开启/关闭当前连接socket上的可读事件监听（通过channel传递到Poller）
            void startRead();
            void stopRead();
            std::string name() const { return name_; }
            bool isReading() const
            {
                return reading_;
            }; 
            // 回调函数设置
            void setConnectionCallback(const ConnectionCallback &cb)
            {
                connectionCallback_ = cb;
            }

            void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }

            void setWriteCompleteCallback(const WriteCompleteCallback &cb)
            {
                writeCompleteCallback_ = cb;
            }
            // Advanced interface 输入输出缓冲区，应用层不用关系底层处理流程
            Buffer *inputBuffer() { return &inputBuffer_; }

            Buffer *outputBuffer() { return &outputBuffer_; }

            /// Internal use only.
            void setCloseCallback(CloseCallback cb) { closeCallback_ = cb; }

            void debugBuffer()
            {
                LOG(DEBUG) << "outputBuffer_:" << endl;
                outputBuffer_.debug();
                LOG(DEBUG) << "inputBuffer_:" << endl;
                inputBuffer_.debug();
            }
            // called when TcpServer accepts a new connection
            void connectEstablished(); // should be called only once
            // called when TcpServer has removed me from its map
            void connectDestroyed(); // should be called only once

            enum State
            {
                kDisconnected,
                kConnecting,
                kConnected,
                kDisconnecting
            };

        private:
            void handleRead(Timestamp receiveTime);
            void handleWrite();
            void handleClose();
            void handleError();
            void sendInLoop(std::string &&message);
            void sendInLoop(const void *message, size_t len);
            void sendInLoop(std::string &message);
            void shutdownWriteInLoop(); /* 关闭写 */
            void forceCloseInLoop();
            void setState(State s) { state_ = s; }
            const char *stateToString() const;
            // 开始接收可读事件
            void startReadInLoop();
            void stopReadInLoop();

            EventLoop *loop_;
            const std::string name_;
            State state_;
            bool reading_;
            // 已连接的socketfd的封装Socket、Channel对象
            std::unique_ptr<Socket> socket_;
            std::unique_ptr<Channel> channel_;
            const InetAddress localAddr_;
            const InetAddress peerAddr_;
            ConnectionCallback connectionCallback_;
            MessageCallback messageCallback_;
            WriteCompleteCallback writeCompleteCallback_;
            CloseCallback closeCallback_;
            // 底层的输入、输出缓冲区的处理
            size_t highWaterMark_;
            Buffer inputBuffer_;
            Buffer outputBuffer_;
            // 智能指针管理
            using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
            void defaultMessageCallback(const TcpConnectionPtr &conn, Buffer *buf);
            void defaultConnectionCallback(const TcpConnectionPtr &conn);
        };

    }
}

#endif