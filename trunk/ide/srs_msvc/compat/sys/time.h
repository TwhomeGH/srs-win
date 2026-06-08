#ifdef _WIN32
#ifndef SRS_COMPAT_SYS_TIME_H
#define SRS_COMPAT_SYS_TIME_H
#include <winsock2.h>
#include <windows.h>
struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};
static inline int gettimeofday(struct timeval *tv, struct timezone *tz) {
    FILETIME ft;
    ULARGE_INTEGER li;
    GetSystemTimeAsFileTime(&ft);
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    const long long EPOCH_DIFF = 116444736000000000LL;
    long long us = (li.QuadPart - EPOCH_DIFF) / 10;
    tv->tv_sec = (long)(us / 1000000);
    tv->tv_usec = (long)(us % 1000000);
    if (tz) {
        TIME_ZONE_INFORMATION tzi;
        GetTimeZoneInformation(&tzi);
        tz->tz_minuteswest = tzi.Bias;
        tz->tz_dsttime = 0;
    }
    return 0;
}
#define ITIMER_REAL 0
#define ITIMER_VIRTUAL 1
#define ITIMER_PROF 2
struct itimerval {
    struct timeval it_interval;
    struct timeval it_value;
};
static inline int setitimer(int which, const struct itimerval *newv, struct itimerval *oldv) {
    (void)which; (void)newv; (void)oldv;
    return 0;
}
static inline int getitimer(int which, struct itimerval *cur) {
    (void)which; (void)cur;
    memset(cur, 0, sizeof(*cur));
    return 0;
}
#endif
#else
#include_next <sys/time.h>
#endif
