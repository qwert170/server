#include "AsyncLogging.h"
#include <iostream>
#include <assert.h>
using namespace YTT;

AsyncLogging::AsyncLogging(const std::string &basename, off_t rollsize, size_t flushs) 
                            : td_(std::bind(&AsyncLogging::threadFunc, this), "loggging"), 
                            roll_size_(rollsize), flushInterval_(flushs), cur_(new buffer), prv_(new buffer)
{
    cur_->reset();
    prv_.reset();
    tranbufvector.reserve(16);
}

AsyncLogging::~AsyncLogging()
{
    running_ = false;
    cv_.notify_one();
    td_.join();
}

void AsyncLogging::append(const char *logline, int len) //发送端
{
    if (!running_)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(m_);
    if (cur_->avail() > len)     // 当前缓冲区足够
    {
        cur_->append(logline, len);
    }
    else    // 当前缓冲已满，放入待写入后端的缓冲队列 
    {
        tranbufvector.push_back(std::move(cur_));
        if (prv_)    // 有备用缓冲，直接移用
        {
            prv_ = std::move(prv_);
        }
        else        // 无备用缓冲，重新分配一个
        { 
            cur_.reset(new buffer);
        }
        cur_->append(logline, len);
        cv_.notify_all();     // 当前缓冲区已满，通知消费线程写入日志
    }
}

void AsyncLogging::start()
{
    running_.store(true);
    std::future<void> fut = p_.get_future();
    fut.get();  //阻塞到进入循环
}

void AsyncLogging::threadFunc() //接收端
{
    // 内部缓冲区、缓冲队列
    bufPtr buf1(new buffer);
    bufPtr buf2(new buffer);
    // 日志后端（非线程安全的）
    LogFile file(logFileName_, roll_size_, flushInterval_);   // 非线程安全的后端
    bufVector RecvVector;
    Logger::setFlush(std::bind(&LogFile::flush, &file));
    Logger::setOutput(std::bind(&LogFile::append, &file, std::placeholders::_1, std::placeholders::_2));
    p_.set_value();
    while (running_.load())
    {
        // 一、等待唤醒，临界区操作。准备待写入日志的缓冲队列
        {
            std::unique_lock<std::mutex> mut(m_);
            while (tranbufvector.empty())
            {
                cv_.wait_for(mut, std::chrono::seconds(flushInterval_));  // 带超时的唤醒
            }
            tranbufvector.push_back(std::move(cur_));    // 当前缓冲放入待写入队列
            cur_ = std::move(buf1);    // 移用新缓冲1给当前缓冲
            if (!prv_)
            {
                prv_ = std::move(buf2);
            }
            // 交换，减少临界区操作时间，非临界区能安全操作待写入文件的日志队列
            RecvVector.swap(tranbufvector);
        }

        //  (1) 日志待写入文件buffersToWrite队列超限（短时间堆积，多为异常情况）
        if (RecvVector.size() > 25)
        {
            char buferr[256];
            snprintf(buferr, sizeof buferr,
                     "Dropped log messages at %s, %lu larger buffers\n",
                     Timestamp::now().toFormattedString(true).c_str(),
                     RecvVector.size() - 2);
            fputs(buferr, stderr);
            RecvVector.resize(2);
        }

        // (2) buffersToWrited队列中的日志消息交给后端写入
        for (auto &buf : RecvVector)
        {
            file.append(buf->begin(), buf->sizes());
        }

        // (3) 将buffersToWrited队列中的buffer重新填充newBuffer1、newBuffer2
        if (RecvVector.size() > 2)
        {
            RecvVector.resize(2);
            if (!buf1)
            {
                buf1 = std::move(RecvVector.back());
                RecvVector.pop_back();
                buf1->reset();
            }
            if (!buf2)
            {
                buf2 = std::move(RecvVector.back());
                RecvVector.pop_back();
                buf1->reset();
            }
        }

        // 二、 非临界区处理
    //  (1) 日志待写入文件buffersToWrite队列超限（短时间堆积，多为异常情况）
    //  (2) buffersToWrited队列中的日志消息交给后端写入
    //  (3) 将buffersToWrited队列中的buffer重新填充newBuffer1、newBuffer2
        RecvVector.clear();
        file.flush();
    }
    // 日志关闭
    file.flush();
}