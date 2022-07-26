#ifndef POLLER_H_
#define POLLER_H_

#include <sys/epoll.h>
#include <sys/poll.h>
#include <unordered_map>
#include <vector>
#include "Timestamp.h"
#include "Logging.h"
#include "Channel.h"
#include "noncopyable.h"
#include "EventLoop.h"

namespace YTT
{
    namespace net
    {
        class Channel;
        class EventLoop;
        class Poller : noncopyable
        {
        public:
            static const int initnum = 36;
            using channelist = std::vector<Channel *>;
            typedef std::unordered_map<int, Channel *> ChannelMap; // fd->channel 映射
            typedef std::vector<struct epoll_event> Eventlist;
            Poller(EventLoop *loop_);
            ~Poller();
            void updatechannel(Channel *channel_);
            void removechannel(Channel *channel_);
            bool haschannel(Channel *channel);
            Timestamp poll(int timeoutMs, channelist *activeChannels);
            void assertinloop();

        private:
            static const char *operationToString(int op);

            void fillActiveChannels(int numEvents,
                                    channelist *activeChannels) const;
            void update(int operation, Channel *channel_);
            ChannelMap channels;
            int epfd;
            EventLoop *loop;
            Eventlist events_;
        };
    }
}

#endif