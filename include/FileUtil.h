#ifndef FILEUTIL_H_
#define FILEUTIL_H_

#include <string>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>

namespace YTT
{
    class FileAdd //处理文件写入的资源类
    {
    public:
        FileAdd(const std::string filname);
        ~FileAdd() { fclose(file); }
        size_t append(const char *logline, size_t len);
        void flush();
        off_t writebyte() { return writed; }
        size_t write(const char *logline, size_t len);

    private:
        off_t writed;
        FILE *file;
        char buf[64 * 1024];
    };
}

#endif