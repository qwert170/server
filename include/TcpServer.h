#ifndef TCPSERVER_H_
#define TCPSERVER_H_

#include "noncopyable.h"
#include "EventLoopThread.h"
#include "TcpConnection.h"
#include "Acceptor.h"
#include "EventLoopThreadPool.h"
#include <map>
#include <atomic>
#include <memory>

namespace YTT
{
    namespace net
    {
        class EventLoop;
        class Acceptor;
        class EventloopThreadpool;
        class TcpServer : noncopyable
        {
        public:
            using ThreadInitback = std::function<void(EventLoop *)>;
            using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
            // 传入TcpServer所属的loop，本地ip，服务器名
            TcpServer(EventLoop *loop, const InetAddress address, const std::string &name_, int numthread = 4); //默认4 i/o线程
            ~TcpServer();
            //启动线程池管理器，将Acceptor::listen()加入调度队列（只启动一次）
            void start();
            const std::string &getipport() const { return ipport; }
            std::shared_ptr<EventLoop> getloop() const { return mainloop; }
            /// 设定线程池中线程的数量（sub reactor的个数），需在start()前调用
            // 0： 单线程(accept 和 IO 在一个线程)
            // 1： acceptor在一个线程，所有IO在另一个线程
            // N： acceptor在一个线程，所有IO通过round-robin分配到N个线程中
            void setThreadnums(int num);
            // 连接建立、断开，消息到达，消息完全写入tcp内核发送缓冲区 的回调函数
            // 非线程安全，但是都在IO线程中调用
            void setConnectionCallback(ConnectionCallback &&cb)
            {
                connectionCallback_ = cb;
            }
            void setMessageCallback(MessageCallback &&cb)
            {
                messageCallback_ = cb;
            }
            void setWriteCompleteCallback(WriteCompleteCallback &&cb)
            {
                writeCompleteCallback_ = std::move(cb);
            }
            void setThreadInitCallback(const ThreadInitback &&cb)
            {
                threadInitCallback_ = std::move(cb);
            }
            // 移除连接
            void removeConnection(const TcpConnectionPtr &conn);
            // 将连接从loop中移除
            void removeConnectionInLoop(const TcpConnectionPtr &conn);
            // 传给Acceptor，Acceptor会在有新的连接到来时调用->handleRead()
            void newConnection(int sockfd, const InetAddress &peerAddr);

        private:
            // string为TcpConnection名，用于找到某个连接。
            using ConnectionMap = std::map<std::string, TcpConnectionPtr>;
            std::shared_ptr<EventLoop> mainloop;
            const std::string ipport;
            int nextConnId_;
            const std::string name;
            // 仅由TcpServer持有
            std::unique_ptr<Acceptor> acceptor_;
            std::shared_ptr<EventloopThreadpool> threadpool_;
            // 用户的回调函数
            ConnectionCallback connectionCallback_;   // 连接状态（连接、断开）的回调
            MessageCallback messageCallback_;      // 新消息到来的回调
            WriteCompleteCallback writeCompleteCallback_;    // 发送数据完毕(全部发给内核)的回调
            ThreadInitback threadInitCallback_;         // 线程池创建成功的回调
            ConnectionMap connects;     // 当前已连接列表
            std::atomic<bool> stared_;
            int threadnum;
        };
    }
}

#endif