#include "mytime.h"
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#else
#include "WinTypes.h"
#include <sys/select.h>
#endif

#ifdef _WIN32
void usleep(unsigned long usec)
{
    LARGE_INTEGER    dwStart;

    LARGE_INTEGER     dwCurrent;

    LARGE_INTEGER     dwFrequence;

    LONGLONG              counter;

    if (!QueryPerformanceFrequency(&dwFrequence))
    {
        return;
    }

    QueryPerformanceCounter(&dwStart);

    counter = dwFrequence.QuadPart * usec / 1000 / 1000;

    dwCurrent = dwStart;

    while ((dwCurrent.QuadPart - dwStart.QuadPart) < counter)
    {
        QueryPerformanceCounter(&dwCurrent);
    }
}
#else
void usleep(unsigned long usec)
{
    struct timeval tval;

    tval.tv_sec=usec/1000000;

    tval.tv_usec=(usec)%1000000;

    select(0,NULL,NULL,NULL,&tval);
}
#endif
/*
void gettimeofday(struct timeval *tp, void *tzp)
{
  uint64_t intervals;
  FILETIME ft;

  GetSystemTimeAsFileTime(&ft);

 //  * A file time is a 64-bit value that represents the number
 //  * of 100-nanosecond intervals that have elapsed since
 //  * January 1, 1601 12:00 A.M. UTC.
 //  *
 //  * Between January 1, 1970 (Epoch) and January 1, 1601 there were
 //  * 134744 days,
 //  * 11644473600 seconds or
 //  * 11644473600,000,000,0 100-nanosecond intervals.
 //  *
 //  * See also MSKB Q167296.

intervals = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
intervals -= 116444736000000000;

tp->tv_sec = (long)(intervals / 10000000);
tp->tv_usec = (long)((intervals % 10000000) / 10);
}
*/

void gettimeofday(struct timeval* tv, void* tzp)
{

    auto time_now = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();
    auto duration_in_s = std::chrono::duration_cast<std::chrono::seconds>(time_now.time_since_epoch()).count();
    auto duration_in_us = std::chrono::duration_cast<std::chrono::microseconds>(time_now.time_since_epoch()).count();

    tv->tv_sec = duration_in_s;
    tv->tv_usec = duration_in_us;
}
