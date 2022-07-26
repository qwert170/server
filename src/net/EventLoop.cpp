#include "EventLoop.h"
using namespace YTT;
using namespace YTT::net;
namespace
{
    __thread net::EventLoop *t_loopInThisThread = 0;
    int createEventfd()
    {
        int evfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (evfd < 0)
        {
            LOG(FATAL) << "createEventfd error" << endl;
        }
        return evfd;
    }
}
const int kPollTimeMs = 10000;
EventLoop::EventLoop() 
                : loop_(false),   // 是否已经开始了IO事件循环
                  quit_(false),   // 是否退出
                  eventHandling_(false),    // 是否正在处理IO事件
                  callingPendingFunctors_(false),    // 是否正在处理计算任务
                  threadid(gettid()),    // 当前线程的tid
                  wakefd(createEventfd()),     // eventfd描述符，用于唤醒阻塞的poll
                  wakechannel_(new Channel(this, wakefd)),   // eventfd封装的Channel
                  poller_(new Poller(this))    // Poller对象(监听socketfd封装的Channels)
{
    if (t_loopInThisThread)
    {
        LOG(FATAL) << "Another EventLoop " << t_loopInThisThread
                        << " exists in this thread " << threadid << endl;
    }
    else
    {
        t_loopInThisThread = this;
    }
    // 设置唤醒channel的可读事件回调，并注册到Poller中
    wakechannel_->setreadback(std::bind(&EventLoop::handleRead, this));
    wakechannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakechannel_->disableAll();
    wakechannel_->remove();
    close(wakefd);
}

void EventLoop::loop()
{
    quit_.store(false);
    while (quit_.load())
    {
        // 定期检查是否有就绪的IO事件（超时事件为kPollTimeMs = 10s）
        activelist.clear();
        polltime = poller_->poll(kPollTimeMs, &activelist);
        // 依次处理有就绪的IO事件的回调
        eventHandling_ = true;   // IO事件处理标志
        for (Channel *chan : activelist)
        {
            curractivechannel = chan;
            curractivechannel->handleEvent(polltime);    // 执行某个事件的用户回调函数
        }
        eventHandling_ = false;
        curractivechannel = NULL;
        // 处理一些计算任务(依次按队列中的顺序执行)
        doPendingFunctors();
    }
}

void EventLoop::assertInLoopThread() {
  if (!isInLoopThread()) {
    LOG(FATAL) << "not in specified EventLoop"
               << (isMainLoop() ? "(mainLoop)" : "(subLoop)") <<endl;
  }
}

void EventLoop::quit()
{
    quit_ = true;
    if (isInLoopThread())
        wakeup();
}

void EventLoop::runInLoop(Function cb)
{
    if (isInLoopThread())
    {
        cb();    // 如果是在IO线程调用，则直接执行
    }
    else
    {   // 其他线程调用，就先放入到任务队列，之后在loop中执行
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Function fun)
{
    {   // 加锁，放入待执行的任务队列中；大括号减小临界区长度，降低锁竞争
        std::lock_guard<std::mutex> lock(mutex_);
        pendinglist.push_back(fun);
    }
    // 根据情况选择是否唤醒
    if (!isInLoopThread() || callingPendingFunctors_)
        wakeup();
}

void EventLoop::wakeup()
{
    // 通过往eventfd写标志通知，让阻塞poll立马返回并执行回调函数
    uint64_t one;
    ssize_t t = write(wakefd, &one, sizeof one);
    if (t != sizeof(one))
    {
        LOG(ERROR) << "EventLoop::wakeup error" << endl;
    }
}

void EventLoop::updateChannel(Channel *channel)
{
    assertInLoopThread();
    poller_->updatechannel(channel); // 将channel的事件和回调注册到Poller中
}

void EventLoop::removeChannel(Channel *channel)
{
    assertInLoopThread();
    if (eventHandling_)
    {
        assert(curractivechannel == channel);
    }
    poller_->removechannel(channel);    // 将channel的事件和回调从Poller中移除
}

bool EventLoop::hasChannel(Channel *channel)
{
    assertInLoopThread();
    poller_->haschannel(channel);
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakefd, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG(ERROR) << "EventLoop::handleRead() reads " << n << " bytes instead of 8" << endl;
    }
}

void EventLoop::doPendingFunctors()
{
    Functlist funtors;
    callingPendingFunctors_ = true;  // 执行计算任务中
    {
        // 加锁，把回调任务队列swap到临时变量中；降低临界区长度
        std::lock_guard<std::mutex> lock(mutex_);
        pendinglist.swap(funtors);
    }
    // 依次执行队列中的用户回调任务
    for(const Function&fun:funtors)
    {
        fun();
    }
    callingPendingFunctors_ = false;
}