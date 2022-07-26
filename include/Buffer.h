#ifndef BUFFER_H_
#define BUFFER_H_

#include "Logging.h"
#include <algorithm>
#include <vector>
#include <assert.h>
#include <string.h>

namespace YTT
{
    namespace net
    {
        class Buffer : noncopyable
        {
            public:
            static const size_t khead = 8;
            static const size_t kinitsize = 1024;
            explicit Buffer(size_t initsize = kinitsize) : buffer_(khead + initsize), readindex_(khead), writeindex_(khead) {}
            void swap(Buffer &rh)
            {
                buffer_.swap(rh.buffer_);
                std::swap(readindex_, rh.readindex_);
                std::swap(writeindex_, rh.writeindex_);
            }
            size_t readbyte() const { return writeindex_ - readindex_; }
            size_t writebyte() const { return buffer_.size() - writeindex_; }
            char *begin() { return &*buffer_.begin(); }
            bool checkmove() { return readindex_ != khead; }
            char* peek() {return &buffer_[readindex_];}     
            void Capity(int len)
            {
                if (writebyte() < len)
                {
                    if (checkmove()) //能否左移
                    {
                        ssize_t readable = readbyte();
                        std::copy(buffer_.begin() + readindex_, buffer_.begin() + writeindex_, buffer_.begin() + khead);
                        readindex_ = khead;
                        writeindex_ = readindex_ + readable;
                    }
                    if (writebyte() < len)
                        buffer_.resize(len + writeindex_);
                }
            }
            void append(void *data, int len)
            {
                const char *t = static_cast<char *>(data);
                append(t, len);
            }
            void append(const char *data, int len)
            {
                if (writebyte() < len)
                {
                    Capity(len);
                }
                else
                {
                    std::copy(data, data + len, buffer_.begin() + writeindex_);
                    writeindex_ += len;
                }
            }
            void expends(int len)
            {
                if (buffer_.size() < len)
                    buffer_.resize(len);
            }
            void retrieve(size_t len)
            {
                assert(len <= readbyte());
                if (len < readbyte())
                {
                    readindex_ += len;
                }
                else
                {
                    retrieveAll();
                }
            }
            void retrieveAll()
            {
                readindex_ = khead;
                writeindex_ = kinitsize;
            } //读完
            void debug()
            {
                LOG(DEBUG) << "readindex" << readindex_ << "writeindex" << writeindex_ << "buffer" << &buffer_[0] << endl;
            }
            ssize_t readfd(int fd, int *err);
        private:
            std::vector<char> buffer_;
            size_t readindex_;
            size_t writeindex_;
        };
    }
}

#endif