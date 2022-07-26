#include "TcpConnection.h"
#include <sys/sendfile.h>

using namespace YTT;
using namespace YTT::net;
TcpConnection::TcpConnection(EventLoop *loop, int sockfd, const std::string &nameArg,
                             const InetAddress &localAddr, const InetAddress &peerAddr) 
                            : loop_(loop), name_(nameArg), state_(kConnecting), 
                              socket_(new Socket(sockfd)),    // 已连接socketfd封装的Socket对象
                              channel_(new Channel(loop, sockfd)),    // 构造的channel
                              localAddr_(localAddr), peerAddr_(peerAddr), 
                              highWaterMark_(64 * 1024 * 1024)   // 缓冲区数据最大64M
{
    // 向Channel对象注册可读事件
      channel_->setreadback(std::bind(&TcpConnection::handleRead, this, _1));
      channel_->serwriteback(std::bind(&TcpConnection::handleWrite, this));
      channel_->setcloseback(std::bind(&TcpConnection::handleClose, this));
      channel_->seterrorback(std::bind(&TcpConnection::handleError, this));
      LOG(DEBUG) << "TcpConnection::ctor[" << name_ << "] at " << this
                     << " fd=" << sockfd;
      // 设置保活机制
      socket_->setKeepAlive(true);  
   }

void TcpConnection::defaultConnectionCallback(const TcpConnectionPtr &conn)
{
    LOG(TRACE) << conn->localAddress().toIpPort() << " -> "
                   << conn->peerAddress().toIpPort() << " is "
                   << (conn->connected() ? "UP" : "DOWN");
}

void TcpConnection::defaultMessageCallback(const TcpConnectionPtr &conn, Buffer *buf)
{
    buf->retrieveAll();
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    loop_->assertInLoopThread();
    int saveerror = 0;
    // 读数据到inputBuffer_中
    size_t n = inputBuffer_.readfd(channel_->getfd(), &saveerror);
    if (n > 0)
    {
      // 用户提供的处理信息的回调函数（由用户自己提供的函数，比如OnMessage）
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    } else if (n == 0) {
        handleClose();   // 读到了0，表明客户端已经关闭了
    } else {
        errno = saveerror;
        LOG(ERROR) << "TcpConnection::handleRead";
        handleError();
    }
}

void TcpConnection::handleWrite()
{
   loop_->assertInLoopThread();
   if (channel_->isWriting())   // 当前channel的socketfd可写（可写事件)
   {
      ssize_t n = write(channel_->getfd(), outputBuffer_.peek(),
                        outputBuffer_.readbyte());
      if (n > 0)
      {
         outputBuffer_.retrieve(n);
         if ( !outputBuffer_.readbyte() )  // 发送完毕
         {  
            channel_->disableWriting();   // 不再关注fd的可写事件，避免busy loop
         }
         if ( writeCompleteCallback_ )
         {
            // 通知用户，发送完毕。（但不保证对端是否完整接收处理）
            loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
         }
         // 如果当前状态是正在关闭连接，主动发起关闭
         if (state_ == kDisconnecting)
         {
            shutdownInLoop();
         }
      }
      else
      {
         LOG(ERROR) << "TcpConnection::handleWrite";
      }
   }
   else  // 已经不可写，不再发送
   {
      LOG(TRACE) << "Connection fd = " << channel_->getfd()
                      << " is down, no more writing";
   }
}

// 当对端调用shutdown()关闭连接时，本端会收到一个FIN，
// channel的读事件被触发，但inputBuffer_.readFd() 会返回0，然后调用
// handleClose()，处理关闭事件，最后调用TcpServer::removeConnection()。
void TcpConnection::handleClose()
{
   loop_->assertInLoopThread();
   assert(state_ == kConnected || state_ == kDisconnecting);
   setState(kDisconnected);     // 设置为已断开状态
   channel_->disableAll();   // channel上不再关注任何事情
}

void TcpConnection::handleError()
{
   int err = getSocketError(channel_->getfd());
   LOG(ERROR) << "TcpConnection::handleError [" << name_
                   << "] - SO_ERROR = " << err << " " << strerror_mr(err);
}

void TcpConnection::send(const void *message, int len)
{
   if (state_ == kConnected)
   {
      EventLoop *subloop = getLoop();
      if (subloop->isInLoopThread())
      {
         sendInLoop(message, len);
      }
      else
      {
         subloop->runInLoop(
             [this, message, len]()
             { this->sendInLoop(message, len); });
      }
   }
}

void TcpConnection::send(std::string &message)
{
   if (state_ == kConnected)
   {
      if (loop_->isInLoopThread())
      {
         this->sendInLoop(message);
      }
      else
      {
         //  loop_->runInLoop(std::bind(&TcpConnection::sendInLoop,this,message));
         loop_->runInLoop([this, &message]
                          { this->sendInLoop(message);});   
      }
   }
}

void TcpConnection::sendInLoop(std::string &message)
{
   sendInLoop(static_cast<const void *>(message.c_str()),
              message.size());
}

void TcpConnection::sendFile(int fd, int fileSize)
{
   loop_->assertInLoopThread();
   sendfile(fd,channel_->getfd(),NULL,fileSize);
}

void TcpConnection::sendInLoop(const void *message, size_t len)
{
   loop_->assertInLoopThread();
   ssize_t nwrite = 0;
   size_t remaining = len;
   bool err = false;
   if (state_ == kDisconnected)
   {
      LOG(WARN) << "disconnected, give up writing";
      return;
   }
   // 如果当前channel没有写事件发生，并且发送buffer无待发送数据，那么直接发送
   if (!channel_->isWriting() && outputBuffer_.readbyte() == 0)
   {
      nwrite = ::write(channel_->getfd(), message, len);
      if (nwrite > 0)   // 发送正常
      {
         // 如果全部发送完毕，就调用发送完成回调函数
         remaining = len - nwrite;
         if (remaining == 0 && writeCompleteCallback_)
         {
            loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
         }
      }
   }
   else   // nwrote < 0   //一旦发生错误，handleRead()读到0字节，关闭连接
   {
      nwrite = 0;
      if (errno != EWOULDBLOCK)
      {
         if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
         {
            err = true;    // 仅记录错误原因
         }
      }
   }
   // 没有经过直接发送remaining == len，或者经过直接发送但还有剩余数据remaining < len
   assert(remaining <= len);
   if (!err && remaining > 0)
   {
      // 把数据添加到输出缓冲区中
      outputBuffer_.append(static_cast<const char *>(message) + nwrite, remaining);
      // 监听channel的可写事件（因为还有数据未发完），
      // 当可写事件被触发，就可以继续发送了，调用的是TcpConnection::handleWrite()
      if (!channel_->isWriting())
      {
         channel_->enableWriting();
      }
   }
}

void TcpConnection::shutdown()
{
   // FIXME: use compare and swap
   if (state_ == kConnected)
   {
      setState(kDisconnecting);
      // FIXME: shared_from_this()?
      loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
   }
}

void TcpConnection::shutdownInLoop()
{
   loop_->assertInLoopThread();
   if (!channel_->isWriting())
   {
      // we are not writing
      socket_->shutdownWrite();
   }
}

void TcpConnection::forceClose()
{
   if (state_ == kConnected || state_ == kDisconnecting)
   {
      setState(kDisconnecting);
      loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
   }
}

void TcpConnection::forceCloseInLoop()
{
   loop_->assertInLoopThread();
   if (state_ == kConnected || state_ == kDisconnecting)
   {

      handleClose();
   }
}

void TcpConnection::setTcpNoDelay(bool on)
{
   socket_->setTcpNoDelay(on);
}

void TcpConnection::startRead()
{
   loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::startReadInLoop()
{
   loop_->assertInLoopThread();
   if (!reading_ || channel_->isReading())
   {
      channel_->enableReading();
      reading_ = true;
   }
}

void TcpConnection::stopRead()
{
   loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
}

void TcpConnection::stopReadInLoop()
{
   loop_->assertInLoopThread();
   if (reading_ || channel_->isReading())
   {
      channel_->disableReading();
      reading_ = false;
   }
}

void TcpConnection::connectEstablished()
{
   loop_->assertInLoopThread();
   assert(state_ == kConnecting);    // 正处于连接建立过程
   setState(kConnected);
   channel_->tie(shared_from_this());
   // 每个连接对应一个channel，打开描述符的可读属性
   channel_->enableReading();
   // 连接成功，回调客户注册的函数（由用户提供的函数，比如OnConnection)
   connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
   loop_->assertInLoopThread();
   if (state_ == kConnected)
   {
      // 这里的代码块和 handleClose() 中重复
      setState(kDisconnected);
      channel_->disableAll();
      connectionCallback_(shared_from_this());
   }
   channel_->remove();
}