#ifndef HTTPRESPONSE_H_
#define HTTPRESPONSE_H_

using namespace YTT::net;

#include "HttpRequest.h"
#include "Callback.h"
#include "TcpConnection.h"
#include <map>
#include <memory>
#include <sstream>

namespace YTT
{
    class HttpResponse
    {
    public:
        enum repState_
        {
            _200, // ok
            _400, // BadRequest
            _404, // NotFound
            DontKnow
        };
        static const char *statestring[DontKnow];
        HttpResponse(const Tcpconptr &conn) : fd_(-1), size_(0), conn_(conn) {}
        void setStatusCode(repState_ state) { state_ = state; }
        void setVersion(std::string v) { version_ = v; }
        void setBody(std::string &&body)
        {
            body_ = std::move(body);
            addContentSize(body_.size());
        }
        void setSourceDir(std::string dir = "./source/") { sourceDir_ = dir; }
        void setSourceFile(std::string file) { sourceFile_ = file; }
        void addContentType(std::string type) { addHeader("Content-Type", type); }
        void addContentSize(int size)
        {
            size_ = size;
            addHeader("Content-Length", std::to_string(size));
        }
        void addKeepAlive(bool on)
        {
            if (on)
                addHeader("Connection", "keep-alive");
            else
                addHeader("Connection", "Close");
        }
        void addHeader(const std::string &key, const std::string &value)
        {
            headMap_[key] = value;
        }
        void linkFileFd(int fd)
        {
            if (fd < 0)
            {
                // LOG(ERROR)
            }
            fd_ = fd;
        }
        void respond()
        {
            /* version */
            /* stateCode */
            std::stringstream stream;
            stream << version_ << " " << statestring[state_] << "\r\n";
            for (auto &&t : headMap_)
            {
                stream << t.first << " " << t.second << "\r\n";
            }
            stream << "\r\n";
            std::string str(stream.str());
            conn_->send(str);
            if (fd_ > 0 && size_ > 0)
            {
                conn_->sendFile(fd_, size_);
            }
            else if (size_ == body_.size())
            {
                conn_->send(body_.c_str(), size_);
            }
        }
    private:
        int size_;
        repState_ state_; /*  */
        std::map<std::string, std::string> headMap_;
        std::string version_;
        std::string sourceDir_;  /* ./source */
        std::string sourceFile_; /* 我们需要发送的文件 */
        std::shared_ptr<HttpRequest> request_;
        Tcpconptr conn_;
        int fd_;           /* 需要发送的文件fd */
        std::string body_; /* 如果发送的不是文件，而只是一个body */
    };
}

#endif