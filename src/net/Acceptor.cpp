#include "Acceptor.h"
#include <fcntl.h>

using namespace YTT;
using namespace YTT::net;

Acceptor::Acceptor(EventLoop *loop_, const InetAddress &listenAddr, bool reuseport) 
                : loop(loop_), acceptSocket_(createNonblockingOrDie(listenAddr.family())),
                  acceptChannel_(loop, acceptSocket_.fd()), fd_(open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    assert(fd_ >= 0);
    // 设置服务端socket选项，并绑定到指定ip和port
    acceptSocket_.setReuseAddr(true);     // addr重用
    acceptSocket_.setReusePort(true);     // 端口重用
    acceptSocket_.bindAddress(listenAddr);      // bing
    acceptChannel_.setreadback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    // 不关注socket上的IO事件，从EventLoop的Poller注销
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    // 关闭socket
    close(fd_);
}

void Acceptor::listen()
{
    loop->assertInLoopThread();
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead() // channel 回调
{
    loop->assertInLoopThread();
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);   // 这里是真正接收连接 
    if (connfd >= 0)    //新的连接成功
    {
        std::string hostport = peerAddr.toIpPort();
        LOG(DEBUG) << "open connfd: " << connfd << "accept from " << hostport << endl;
        if (NewConnection_)
            // 建立新的连接，调用TcpServer的回调，返回已连接的socketfd和peer端地址
            NewConnection_(connfd, peerAddr);
        else
            // 若上层应用TcpServer未注册新连接回调函数，则直接关闭当前连接
            close(connfd);
    }
    else // 连接异常，处理服务端fd耗尽
    {
        if (errno == EMFILE)    // 无可用fd，如不处理，否则Poller水平触发模式下会一直触发
        {
            close(fd_);    // 关闭空闲的fd，空出一个可用的fd
            fd_ = net::accept(acceptSocket_.fd(), NULL);   // 把前面调用acceptor没有接受的描述符接受进来到idleFd_ 
            close(fd_);    // 把这个idleFd_ 关闭，就是关闭了当前此次连接
            fd_ = open("/dev/null", O_RDONLY | O_CLOEXEC);    // 重新开启这个空闲描述符
        }
        else
        {
            LOG(ERROR) << "Acceptor::handleRead accept error" << endl;
        }
    }
}