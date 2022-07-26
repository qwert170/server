#ifndef SOCKET_H_
#define SOCKET_H_

#include "noncopyable.h"
#include "InetAddress.h"
#include <sys/socket.h>
#include <netinet/in.h>

namespace YTT
{
    namespace net
    {
        class InetAddress;
        int createNonblockingOrDie(sa_family_t family);
        int connect(int sockfd, const struct sockaddr *addr);
        void bindOrDie(int sockfd, const struct sockaddr *addr);
        void listenOrDie(int sockfd);
        int accept(int sockfd, struct sockaddr_in *addr);
        void close(int sockfd);
        void shutdownWrite(int sockfd);
        void shutdownRead(int sockfd);
        void toIpPort(char *buf, size_t size, const struct sockaddr *addr);
        void toIp(char *buf, size_t size, const struct sockaddr *addr);
        void toPortString(char *buf, size_t size, const struct sockaddr *addr);
        void fromIpPort(const char *ip, uint16_t port, struct sockaddr_in *addr);

        int getSocketError(int sockfd);

        struct sockaddr_in getLocalAddr(int sockfd);
        struct sockaddr_in getPeerAddr(int sockfd);
        class Socket : noncopyable
        {
        public:
            explicit Socket(int sockfd) : sockfd_(sockfd) {}
            ~Socket() { close(sockfd_); }
            int fd() const { return sockfd_; }
            // 绑定地址
            void bindAddress(const InetAddress &localaddr);
            // 启动监听
            void listen();
            int accept(InetAddress *peeraddr);
            // 半关闭写
            void shutdownWrite();
            void shutdownRead();
            // 禁用 Nagle?
            void setTcpNoDelay(bool on);
            // 端口复用
            void setReuseAddr(bool on);
            void setReusePort(bool on);
            void setKeepAlive(bool on);

        private:
            const int sockfd_;
        };
    }
}

#endif  