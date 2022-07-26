#include "Socket.h"
#include "Logging.h"
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <cassert>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h> // snprintf
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/uio.h> // readv
#include <unistd.h>

using namespace YTT;
using namespace YTT::net;

void Socket::bindAddress(const InetAddress &localaddr)
{
    ::bindOrDie(sockfd_, localaddr.getSockAddr());
}

void Socket::listen() 
{ 
    listenOrDie(sockfd_); 
}

int Socket::accept(InetAddress *peeraddr)
{
    struct sockaddr_in addr;
    explicit_bzero(&addr, sizeof addr);
    int connfd = net::accept(sockfd_, &addr);
    if (connfd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    ::shutdownWrite(sockfd_);
}

void Socket::shutdownRead()
{
    ::shutdownRead(sockfd_);
}

void Socket::setTcpNoDelay(bool on) // 禁用Nagle接口
{
    int opt = on ? 1 : 0;
    setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &opt, static_cast<socklen_t>(sizeof(opt)));
}

void Socket::setReuseAddr(bool on)
{
    int opt = on ? 1 : 0;
    setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &opt, static_cast<socklen_t>(sizeof(opt)));
}

void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval,
                           static_cast<socklen_t>(sizeof optval));
}

/* 发送心跳包 */
/* 使用SO_KEEPALIVE套接字选项调用
的getsockopt函数允许应用程序检索
keepalive选项的当前状态。
*/
void Socket::setKeepAlive(bool on=true)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval,
                 static_cast<socklen_t>(sizeof optval));
}

int createNonblockingOrDie(sa_family_t family)
{
    int fd = socket(family, SOCK_CLOEXEC | SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if (fd < 0)
        LOG(ERROR) << "sock::createNonblockingOrDie" << endl;
    return fd;
}

int connect(int sockfd, const struct sockaddr *addr)
{
    return connect(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in)));
}

void bindOrDie(int sockfd, const struct sockaddr *addr)
{
    int ret = bind(sockfd, addr, static_cast<socklen_t>(sizeof(struct sockaddr_in)));
    if (ret < 0)
        LOG(ERROR) << "sock::bindOrDie" << endl;
}

void listenOrDie(int sockfd)
{
    int ret = listen(sockfd, SOMAXCONN);
    if (ret < 0)
        LOG(ERROR) << "sock::listenOrDie" << endl;
}

int accept(int sockfd, struct sockaddr_in *addr)
{
    socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
    int connfd = ::accept4(sockfd, static_cast<sockaddr *>(static_cast<void *>(addr)), &addrlen,
                           SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd < 0)
    {
        int saveerr = errno;
        switch (saveerr)
        {
        case EAGAIN:
        case ECONNABORTED:
        case EINTR:
        case EPROTO:
        case EPERM:
        case EMFILE:
            errno = saveerr;
            break;
        case EBADF:
        case EFAULT:
        case EINVAL:
        case ENFILE:
        case ENOBUFS:
        case ENOMEM:
        case ENOTSOCK:
        case EOPNOTSUPP:

            LOG(FATAL) << "unexpected error of ::accept " << saveerr;
            break;
        default:
            LOG(FATAL) << "unknown error of ::accept " << saveerr;
            break;
        }
    }
    return connfd;
}
void net::close(int sockfd)
{
    int ret = ::close(sockfd);
    if (ret < 0)
        LOG(ERROR) << "sock::close" << endl;
}

void shutdownRead(int sockfd)
{
    if (::shutdown(sockfd, SHUT_RD) < 0)
    {
        LOG(ERROR) << "sock::shutdownRead" << endl;
    }
}

void shutdownWrite(int sockfd)
{
    if (::shutdown(sockfd, SHUT_WR) < 0)
    {
        LOG(ERROR) << "sock::shutdownWrite" << endl;
    }
}

void net::toIpPort(char *buf, size_t size, const struct sockaddr *addr)
{
    toIp(buf, size, addr);
    size_t end = ::strlen(buf);
    const struct sockaddr_in *addr4 = static_cast<sockaddr_in *>(static_cast<void *>(const_cast<sockaddr *>(addr)));
    uint16_t port = ntohs(addr4->sin_port);
    assert(size > end);
    snprintf(buf + end, size - end, ":%u", port);
}

void net::toPortString(char *buf, size_t size, const struct sockaddr *addr)
{
    const struct sockaddr_in *addr4 = static_cast<sockaddr_in *>(static_cast<void *>(const_cast<sockaddr *>(addr)));
    size_t end = ::strlen(buf);
    uint16_t port = ntohs(addr4->sin_port);
    assert(size > end);
    snprintf(buf + end, size - end, "%u", port);
}

void net::toIp(char *buf, size_t size, const struct sockaddr *addr)
{
    if (addr->sa_family == AF_INET)
    {
        assert(size >= INET_ADDRSTRLEN);
        const struct sockaddr_in *addr4 = static_cast<sockaddr_in *>(static_cast<void *>(const_cast<sockaddr *>(addr)));
        inet_ntop(AF_INET, &addr4->sin_addr, buf, static_cast<socklen_t>(size));
    }
}

void net::fromIpPort(const char *ip, uint16_t port, struct sockaddr_in *addr)
{
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0)
    {
    }
}

int net::getSocketError(int sockfd)
{
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);

    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        return errno;
    }
    else
    {
        return optval;
    }
}

struct sockaddr_in net::getLocalAddr(int sockfd)
{
    struct sockaddr_in localaddr;
    explicit_bzero(&localaddr, sizeof (localaddr));
    socklen_t addrlen = static_cast<socklen_t>(sizeof(localaddr));
    struct sockaddr *socksaddr = static_cast<sockaddr *>(static_cast<void *>(&localaddr));
    if (::getsockname(sockfd, socksaddr, &addrlen) < 0)
    {
        LOG(ERROR) << "sock::getLocalAddr";
    }
    return localaddr;
}

struct sockaddr_in net::getPeerAddr(int sockfd)
{
    struct sockaddr_in peeraddr;
    explicit_bzero(&peeraddr, sizeof peeraddr);
    socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
    struct sockaddr *socksaddr = static_cast<sockaddr *>(static_cast<void *>(&peeraddr));
    if (::getpeername(sockfd, socksaddr, &addrlen) < 0)
    {
        LOG(ERROR) << "sock::getPeerAddr";
    }
    return peeraddr;
}