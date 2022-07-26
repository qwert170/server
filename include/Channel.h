#ifndef CHANNEL_H_
#define CHANNEL_H_

#include "EventLoop.h"
#include "Callback.h"
#include "noncopyable.h"
#include <memory>
#include <vector>
#include <functional>

namespace YTT
{
    namespace net
    {
        class EventLoop;
        enum
        {
            kNew,
            KAdded,
            kDeleted,
        };
        class Channel : noncopyable
        {
            typedef std::function<void()> Eventback;      // 事件回调函数定义
            using readEventback = std::function<void(Timestamp)>;  // 读事件回调函数定义
            using channelist = std::vector<Channel *>;

        public:
            Channel(EventLoop *loop_, int fd_);
            ~Channel();

            void handleEvent(Timestamp recv_time);   // 处理事件

            //设置可读、可写、关闭、出错的事件回调函数
            void setreadback(readEventback cb)
            {
                readCallback_ = std::move(cb);
            }
            void serwriteback(Eventback cb)
            {
                writeCallback_ = std::move(cb);
            }
            void setcloseback(Eventback cb)
            {
                closeCallback_ = std::move(cb);
            }
            void seterrorback(Eventback cb)
            {
                errorCallback_ = std::move(cb);
            }
            void enableReading()    // 注册可读事件
            {
                events_ |= kReadEvent;
                update();
            }
            void disableReading()   // 注销可读事件
            {
                events_ &= ~kReadEvent;
                update();
            }
            void enableWriting()    // 注册可写事件
            {
                events_ |= kWriteEvent;
                update();
            }
            void disableWriting()   // 注销可写事件
            {
                events_ &= ~kWriteEvent;
                update();
            }
            void disableAll()       // 注销所有事件
            {
                events_ = kNoneEvent;
                update();
            }
            bool isWriting() const { return events_ & kWriteEvent; }   // 是否注册可写事件
            bool isReading() const { return events_ & kReadEvent; }    // 是否注册可读事件
            bool noEvent() const { return events_ == kNoneEvent; }     // 判断是否注册了事件
            int getfd() { return fd; }
            int getstatus() { return status; }
            void setstatus(int status) {}
            bool isNoneEvent();
            int getevent() { return events_; }    //返回注册的事件
            void setevent(int opt) { recvevents_ = opt; }   
            void tie(const std::shared_ptr<void> &pt);
            void handleEVentwithguard(Timestamp recvtime);
            std::string reventsToString();
            void remove();

        private:
            void update();    // 注册事件后更新到EventLoop

            static const int kNoneEvent;
            static const int kReadEvent;
            static const int kWriteEvent;

            EventLoop *loop;    //channel所属的EventLoop
            int events_;        //注册的事件
            int recvevents_;    // poller返回接收到的就绪的事件
            const int fd;       //channel负责的文件描述符，但不负责关闭该文件描述符
            int status;
            std::weak_ptr<void> tie_;
            bool tied_;

            readEventback readCallback_;    // 读事件回调 
            Eventback writeCallback_;       // 写事件回调 
            Eventback closeCallback_;       // 关闭事件回调 
            Eventback errorCallback_;       // 出错事件回调 
        };
    }
}

#endif