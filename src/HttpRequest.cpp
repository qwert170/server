#include "HttpRequest.h"

using namespace YTT;

const char *HttpRequestLine::httpstringmethod[DONT_UNDERSTAND] = {
    "GET", "POST" 
};

void HttpRequestLine::debug()
{
    LOG(DEBUG) << "RequestLine: " << endl;
    LOG(DEBUG) << "version_: " << version << " url_: " << url
                    << " method: " << getstringmethod() << endl;
}

void HttpRequest::requestLine()
{
    int len;
    for (pt = 0; pt != len; pt++)
    {
        if (buf[pt] != '\r')
            continue;
        if (buf[pt + 1] == '\n')
            break;
    }
    if (strncmp(buf + pt, CR_LF, 2) != 0) //不完整
        return;
    m_end = pt + 2;
    pt = m_beg;
    len = m_end - m_beg;
    request->line.setnow(HttpRequestLine::lineState::method_);

    if (len >= strlen("GET") && strncmp(buf + pt, "GET", strlen("GET")) == 0)
    {
        request->line.setmethod(GET);
        pt += strlen("GET");
    }
    else if (len >= strlen("POST") && strncmp(buf + pt, "POST", strlen("POST")) == 0)
    {
        request->line.setmethod(POST);
        pt += strlen("POST");
    }
    else
    {
        line = LineFail_;
    }
    for (; pt != m_end; pt++)
    {
        if (pt == m_end)
            break;
        else if (buf[pt++] != ' ')
            break;
    }
    request->line.setnow(HttpRequestLine::lineState::url_);
    int u_beg = pt;
    while (pt != m_end && buf[pt++] != ' ')
    {
    }
    request->line.seturl(std::string(buf + u_beg, pt - u_beg - 1));
    LOG(DEBUG) << buf[u_beg] << pt - u_beg << endl;
    request->line.setnow(HttpRequestLine::lineState::version_);
    if (!strncmp(buf + pt, "HTTP/1.1", 8) ||
        !strncmp(buf + pt, "HTTP/1.0", 8))
    {
        request->line.setversion(std::string(buf + pt, 8));
        pt = m_end;
        line = LineSuc_;
        return;
    }
    line = LineFail_;
}