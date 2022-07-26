#include "TcpServer.h"
#include "EventLoop.h"

using namespace YTT;
using namespace YTT::net;

TcpServer::TcpServer(EventLoop *loop, const InetAddress address, const std::string &name_, int numthread) 
                    : mainloop(loop), 
                      ipport(address.toIpPort()),    // 服务端监听的地址
                      name(name_),
                      acceptor_(new Acceptor(loop, address, true)),   // 处理新连接的Acceptor
                      threadpool_(new EventloopThreadpool(loop)),     // 线程池
                      connectionCallback_(defaultConnectionCallback),  // 提供给用户的 连接、断开 的回调
                      messageCallback_(defaultMessageCallback),        // 提供给用户的 新消息到来 的回调
                      threadnum(numthread)
{
    // 设置Acceptor处理新连接的回调函数
    acceptor_->setConnectback(std::bind(&TcpServer::newConnection, this, _1, _2));
    threadpool_->setThreadNum(numthread);
}

TcpServer::~TcpServer()
{
    mainloop->assertInLoopThread();
    nextConnId_ = 1;
    LOG(INFO) << "yutbin server " << name << "quit" << endl;
    fprintf(stdout, "yutbin server %s quit", name.data());
    for (auto &it : connects)
    {
        TcpConnectionPtr conn(it.second);
        it.second.reset();      // 引用计数减1，不代表已经释放
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

void TcpServer::setThreadnums(int num)
{
    assert(num >= 0);
    threadpool_->setThreadNum(num);
}

void TcpServer::start()
{
    bool expect = false;
    if (stared_.compare_exchange_strong(expect, true))
    {
        threadpool_->start(threadInitCallback_); //启动i/o线程池
        mainloop->runInLoop(std::bind(&Acceptor::listen, acceptor_));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) // accept 回调
{
    mainloop->assertInLoopThread();
    // 使用round-robin策略从线程池中获取一个loop
    EventLoop *ioloop = threadpool_->getNextLoop();
    char buf[64];
    snprintf(buf, sizeof(buf), "-%s#%d", ipport.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name + buf;   // 当前连接的名称
    LOG(INFO) << "TcpServer::newConnection [" << name
                   << "] - new connection [" << connName
                   << "] from " << peerAddr.toIpPort() << endl;
    InetAddress localAddr(getLocalAddr(sockfd)); //获取本地地址
    // 创建一个TcpConnection对象，表示每一个已连接
    Tcpconptr conn(new TcpConnection(ioloop,    // 当前连接所属的loop
                                     sockfd,     // 已连接的socket
                                     connName,   // 连接名称
                                     localAddr,   // 本端地址
                                     peerAddr));  // 对端地址
    // 当前连接加入到connection map中
    connects[connName] = conn;
    // 设置TcpConnection上的事件回调
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, _1));
    // 回调执行TcpConnection::connectEstablished()，确认当前已连接状态，
    // 在Poller中注册当前已连接socket上的IO事件
    ioloop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    mainloop->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    mainloop->assertInLoopThread();
    size_t n = connects.erase(conn->name());  // 通过name移除coon
    assert(n == 1);
    // 在conn所在的loop中执行TcpConnection::connectDestroyed
    EventLoop *ioloop = conn->getLoop();
    ioloop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}