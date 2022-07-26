#ifndef HTTPSERVER_H_
#define HTTPSERVER_H_

#include "noncopyable.h"
#include "TcpServer.h"
#include "ThreadPool.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
using namespace YTT;
using namespace YTT::net;

namespace YTT
{
    using Task = std::function<void()>;
    class HttpServer : noncopyable
    {
    public:
        HttpServer(int poolthreads, InetAddress &addr) 
           : pool(new ThreadPool()), 
           tcpserver_(new YTT::net::TcpServer(mainloop, addr, "httpserver", 4)), mainloop(new EventLoop())
        {
            pool->start(poolthreads);
            srand(time(nullptr));
        }
        void Messageback(const Tcpconptr &conn, Buffer *buf);
        void Connectionback(const Tcpconptr &conn);
        void Successback(const Tcpconptr &conn, HttpRequestate *request, Buffer *buf);
        int saveurl(std::string &url, struct stat &ft, HttpResponse &respond);
        void setthreadpoolback(Task tk)
        {
            pool->run(tk); //加入线程池
        }
        void start()
        {
            tcpserver_->setMessageCallback(std::bind(&HttpServer::Messageback, this, _1, _2));
            tcpserver_->setConnectionCallback(std::bind(&HttpServer::Connectionback, this, _1));
            tcpserver_->start();
        }
    private:
        std::unique_ptr<YTT::ThreadPool> pool;
        std::unique_ptr<EventLoop> mainloop;
        std::unique_ptr<net::TcpServer> tcpserver_;
    };
}

#endif