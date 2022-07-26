#ifndef EVENTLOOP_H_
#define EVENTLOOP_H_

#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include <mutex>
#include <sys/eventfd.h>
#include "Channel.h"
#include "Timestamp.h"
#include "Poller.h"
#include "Logging.h"

#include "noncopyable.h"
#include "Socket.h"
#include "TcpConnection.h"

namespace YTT
{
    namespace net
    {
        class Channel;
        class TcpConnection;
        class EventLoop : noncopyable, public std::enable_shared_from_this<EventLoop>
        {
            public:
            using Function = std::function<void()>;
            EventLoop();
            ~EventLoop();
            void init();
            void loop(); /* 循环 epoll_wait */
            void quit(); /* 退出 */
            void runInLoop(Function fun);
            void queueInLoop(Function fun);

            void wakeup();
            void updateChannel(Channel *channel);
            void removeChannel(Channel *channel);
            bool hasChannel(Channel *channel);
            void assertInLoopThread();
            bool isInLoopThread() const { return threadid == gettid(); }

        private:
            void handleRead();        //用于 读取Wakeupfd 为了唤醒所发送的内容
            void doPendingFunctors(); /* 执行小任务函数 */

            bool isMainLoop();
            using Channelist = std::vector<Channel *>;
            using Functlist = std::vector<Function>;
            bool loop_;
            std::atomic<bool> quit_;
            bool eventHandling_;          //是否事件循环
            bool callingPendingFunctors_; //是否处理后续任务
            const pid_t threadid;
            int wakefd;
            std::unique_ptr<Channel> wakechannel_;
            std::unique_ptr<Poller> poller_;
            Timestamp polltime;
            mutable std::mutex mutex_;
            Channel *curractivechannel;
            Channelist activelist;
            Functlist pendinglist;
        };
    }
}

#endif