#ifndef CONNECTOR_H_
#define CONNECTOR_H_

#include "noncopyable.h"
#include "Logging.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include <memory>
#include <functional>

namespace YTT
{
    namespace net
    {
        class Channel;
        class EventLoop;
        class Connector : noncopyable, public std::enable_shared_from_this<Connector>
        {
        public:
            using NewConnectionCallback = std::function<void(int sockfd)>;
            Connector(EventLoop *loop, const InetAddress &serverAddr);
            ~Connector();
            void setNewConnectionCallback(const NewConnectionCallback &cb)
            {
                newConnectionCallback_ = cb;
            }
            void Start();   // can be called in any thread
            void restart(); // must be called in loop thread
            void stop();    // can be called in any thread
            const InetAddress &serverAddress() const { return serverAddr_; }

        private:
            enum States
            {
                kDisconnected,
                kConnecting,
                kConnected
            };
            static const int kMaxRetryDelayMs = 30 * 1000;
            static const int kInitRetryDelayMs = 500;

            void setState(States s) { state_ = s; }
            void startInLoop();
            void stopInLoop();
            void connect();
            void connecting(int sockfd);
            void handleWrite();
            void handleError();
            void retry(int sockfd);
            int removeAndResetChannel();
            void resetChannel();
            EventLoop *loop_;
            InetAddress serverAddr_;
            bool connect_;
            States state_;
            std::unique_ptr<Channel> channel_;
            NewConnectionCallback newConnectionCallback_;
        };
    }
}

#endif