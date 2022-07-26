#include "TcpClient.h"

using namespace YTT;
using namespace YTT::net;

TcpClient::TcpClient(EventLoop *loop,
                     const InetAddress &serverAddr,
                     const std::string &nameArg)
    : loop_(loop),
      connector_(new Connector(loop, serverAddr)),    // 创建一个Connector
      name_(nameArg),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback),
      retry_(false),    // 默认不重连
      connect_(true),    // 开始连接
      nextConnId_(1)    // 当前连接的序号
{
    // 设置建立连接的回调函数
    connector_->setNewConnectionCallback(std::bind(&TcpClient::newConnection, this, _1)); //给connector设置连接成功时的回调
}

void TcpClient::newConnection(int sockfd)
{
    loop_->assertInLoopThread();     // 
    InetAddress peerAddr(getPeerAddr(sockfd));    // 获取对端的地址
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
    ++nextConnId_; //对连接进行计数
    std::string connName = name_ + buf;     // 连接的名字

    InetAddress localAddr(getLocalAddr(sockfd));    // 获取本端的地址

    // 构造一个TcpConnection对象，并设置相应的回调函数
    TcpConnectionPtr conn(new TcpConnection(loop_,
                                            sockfd,
                                            connName,
                                            localAddr,
                                            peerAddr));

    conn->setConnectionCallback(connectionCallback_);       
    conn->setMessageCallback(messageCallback_);             
    conn->setWriteCompleteCallback(writeCompleteCallback_); 
    conn->setCloseCallback(
        std::bind(&TcpClient::removeConnection,this, _1)); 
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_=conn;   // 保存到成员变量
    }
    conn->connectEstablished();   // 注册到Poller，监听IO事件
}

void TcpClient::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_.reset();
    }
    loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    if (retry_ && connect_)
    {
        LOG(INFO) << "TcpClient::connect[" << name_ << "] - Reconnecting to "
                       << connector_->serverAddress().toIpPort();
        connector_->restart();
    }
}