#ifndef TCPCLIENT_H_
#define TCPCLIENT_H_

#include <mutex>
#include "Connector.h"

namespace YTT
{
    namespace net
    {
        class Connector;
        using ConnectorPtr=std::shared_ptr<Connector>;
        class TcpClient : noncopyable
        {
        public:
            using TcpConnectionPtr=std::shared_ptr<TcpConnection>; 
            TcpClient(EventLoop *loop,
                      const InetAddress &serverAddr,
                      const std::string &nameArg);
            ~TcpClient();
            void connect();
            void disconnect();
            void stop();
            TcpConnectionPtr connection()
            {
                std::lock_guard<std::mutex> lock(mutex_);
                return connection_;
            }
            EventLoop *getLoop() const { return loop_; }
            bool retry() const { return retry_; }
            void enableRetry() { retry_ = true; }

            const std::string &name() const
            {
                return name_;
            }

            /// Set connection callback.
            /// Not thread safe.
            void setConnectionCallback(ConnectionCallback cb)
            {
                connectionCallback_ = std::move(cb);
            }

            /// Set message callback.
            /// Not thread safe.
            void setMessageCallback(MessageCallback cb)
            {
                messageCallback_ = std::move(cb);
            }

            /// Set write complete callback.
            /// Not thread safe.
            void setWriteCompleteCallback(WriteCompleteCallback cb)
            {
                writeCompleteCallback_ = std::move(cb);
            }

        private:
            /// Not thread safe, but in loop
            void newConnection(int sockfd);
            /// Not thread safe, but in loop
            void removeConnection(const TcpConnectionPtr &conn);

            EventLoop *loop_;     // 所属的EvenetLoops
            ConnectorPtr connector_;      // 使用Connector智能指针，避免头文件引入
            const std::string name_;      // 连接的名字
            ConnectionCallback connectionCallback_;     // 建立连接的回调函数
            MessageCallback messageCallback_;     // 消息到来的回调函数
            WriteCompleteCallback writeCompleteCallback_;     // 数据发送完毕回调函数
            bool retry_;   // 连接断开后是否重连
            bool connect_; // atomic
            int nextConnId_;    // name_+nextConnId_ 用于标识一个连接
            mutable std::mutex mutex_;
            TcpConnectionPtr connection_;
        };
    }
}

#endif