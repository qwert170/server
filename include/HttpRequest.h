#ifndef HTTPREQUEST_H_
#define HTTPREQUEST_H_

#include "Buffer.h"
#include <string>
#include <memory>
#include <unordered_map>

namespace YTT
{
    enum Method
    {
        GET = 0,
        POST,
        DONT_UNDERSTAND,
    };
    class HttpRequestLine //请求行
    {
    public:
        enum lineState
        {
            method_,
            url_,
            version_
        };
        HttpRequestLine(std::string version_ = "HTTP/1.1") : version(version_){};
        void setmethod(Method mod_) { mod = mod_; }
        void seturl(std::string url_) { url = std::move(url_); }
        void setversion(std::string version_) { version = std::move(version_); }
        const std::string  geturl() { return url; }
        std::string getversion() { return version; }
        Method getmethod() { return mod; }
        const char *getstringmethod() { return httpstringmethod[mod]; }
        void debug();
        void setnow(lineState t) { statnow = t; }
    private:
        lineState statnow;
        Method mod;          //请求方法
        mutable std::string url;     // url
        std::string version; //版本号
        static const char *httpstringmethod[Method::DONT_UNDERSTAND];
    };
    class HttpRequestHead //请求头部
    {
    public:
        HttpRequestHead() = default;
        void add(std::string key, std::string value) { head.insert(key, value); }
        void remove(std::string key) { head.erase(key); }
        std::string getvalue(std::string key) { return head[key]; }
        std::pair<std::string,std::string> get(std::string kv){return *head.find(kv);}
        void debug()
        {
            LOG(DEBUG) << "Httphead" << endl;
            for_each(head.begin(), head.end(), [](const std::pair<std::string, std::string> &pt)
                     { LOG(DEBUG) << "key" << pt.first << "value" << pt.second << endl; });
        }
    private:
        std::unordered_map<std::string, std::string> head;
    };
    class HttpBody
    {
    public:
        HttpBody() = default;
        void setmessage(std::string message_) { message = std::move(message_); }
        std::string message;
    };
    class HttpRequestate //状态机
    {
    public:
        HttpRequestate() : state(0) {}
        int state;
        class HttpRequestLine line;
        HttpRequestHead head;
        HttpBody message;
    };
    class HttpRequest : noncopyable
    {
    public:
        enum State
        {
            Success,
            Fail,
            Incomplete,
        };
        enum Linestate
        {
            LineSuc_,
            LineFail_,
            LineIncom_,
        };
        HttpRequest(const char *buf_, int len_) : buf(buf_), len(len_), pt(0), m_end(0), m_beg(0), end(Incomplete), line(LineIncom_), request(new HttpRequestate()) {}
        void requestLine();
        void requestHead();
        void requestMessage(int len_);
        HttpRequestate* getrequest() {return request.get();}
        State requestall();
        int httpersize;
    private:
        const char *CR_LF = "\r\n";
        const char *buf;
        int len;
        int pt; //当前索引
        int m_end;
        int m_beg;
        State end;
        Linestate line;
        std::shared_ptr<HttpRequestate> request;
    };
}

#endif