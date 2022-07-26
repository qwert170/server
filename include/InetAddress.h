#ifndef INETADDRESS_H
#define INETADDRESS_H
#include "Socket.h"
#include <string>
#include <netinet/in.h>
namespace YTT
{
    namespace net
    {
        class Socket;
        class InetAddress
        {
        public:
            explicit InetAddress();
            explicit InetAddress(const char *ip, uint16_t port);
            explicit InetAddress(const struct sockaddr_in &addr_) : addr(addr_) {}
            std::string toIp() const;    //返回地址Ip(字符串形式)
            std::string toIpPort() const;     //返回地址和端口号(字符串形式)
            std::string toPortString() const;
            uint16_t toPort() const;
            sa_family_t family() const { return addr.sin_family; }
            const struct sockaddr *getSockAddr() const { return static_cast<sockaddr *>(static_cast<void *>(const_cast<sockaddr_in *>(&addr))); }
            void setSockAddr(struct sockaddr_in addr_) { addr = addr_; }

        private:
            struct sockaddr_in addr;
        };
    }
}
#endif