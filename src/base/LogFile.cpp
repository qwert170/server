#include "LogFile.h"
using namespace YTT;

bool LogFile::rollfile()
{
    time_t now(0);
    std::string filename(basename);
    filename = LogFile::addtime(filename, &now);
    time_t nowday = now - (now % daylight);
    if (now < lastRoll)
    {
        lastRoll = now;
        lastFlush = now;
        lastday = nowday;
        file.reset(new FileAdd(filename));
        return true;
    }
    return false;
}

std::string LogFile::addtime(std::string filename, time_t *t)
{
    *t = time(NULL);
    Timestamp tap(*t);
    return filename + tap.toFormattedString(true) + ".log";
}

void LogFile::flush()
{
    if (mutex_)
    {
        std::lock_guard<std::mutex> t(*mutex_);
        file->flush();
    }
    else
    {
        file->flush();
    }
}

void LogFile::append(const char *log, int len)
{
    if (mutex_)
    {
        std::lock_guard<std::mutex> t(*mutex_);
        unlock_append(log, len);
    }
    else
    {
        unlock_append(log, len);
    }
}

void LogFile::unlock_append(const char *log, int len)
{
    file->append(log, len);
    if (file->writebyte() > rollsize)
    {
        rollfile();
    }
    else
    {
        ++count;
        if (count > checkevery)
        {
            count = 0;
            time_t now = time(NULL);
            time_t thisday = now - (now % daylight);
            if (thisday != lastday)
            {
                rollfile();
            }
            else if (now - lastFlush > flushtime)
            {
                lastFlush = now;
                flush();
            }
        }
    }
}