#ifdef _WIN32
#ifndef SRS_COMPAT_SYS_RESOURCE_H
#define SRS_COMPAT_SYS_RESOURCE_H
#include <winsock2.h>
#include <windows.h>
#define RUSAGE_SELF 0
#define RUSAGE_CHILDREN -1
struct rusage {
    struct timeval ru_utime;
    struct timeval ru_stime;
    long ru_maxrss;
    long ru_ixrss;
    long ru_idrss;
    long ru_isrss;
    long ru_minflt;
    long ru_majflt;
    long ru_nswap;
    long ru_inblock;
    long ru_oublock;
    long ru_msgsnd;
    long ru_msgrcv;
    long ru_nsignals;
    long ru_nvcsw;
    long ru_nivcsw;
};
#define RLIMIT_NOFILE 7
#define RLIMIT_STACK 3
#define RLIMIT_AS 9
#define RLIM_INFINITY ((unsigned long long)-1)
struct rlimit {
    unsigned long long rlim_cur;
    unsigned long long rlim_max;
};
static inline int getrusage(int who, struct rusage *usage) {
    (void)who;
    FILETIME ct, et, kt, ut;
    GetProcessTimes(GetCurrentProcess(), &ct, &et, &kt, &ut);
    usage->ru_utime.tv_sec = (long)(((ULARGE_INTEGER*)&ut)->QuadPart / 10000000);
    usage->ru_utime.tv_usec = (long)(((ULARGE_INTEGER*)&ut)->QuadPart % 10000000 / 10);
    usage->ru_stime.tv_sec = (long)(((ULARGE_INTEGER*)&kt)->QuadPart / 10000000);
    usage->ru_stime.tv_usec = (long)(((ULARGE_INTEGER*)&kt)->QuadPart % 10000000 / 10);
    MEMORYSTATUSEX ms;
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatusEx(&ms);
    usage->ru_maxrss = (long)(ms.ullTotalPhys / 1024);
    return 0;
}
static inline int getrlimit(int resource, struct rlimit *rlim) {
    (void)resource;
    rlim->rlim_cur = RLIM_INFINITY;
    rlim->rlim_max = RLIM_INFINITY;
    return 0;
}
static inline int setrlimit(int resource, const struct rlimit *rlim) {
    (void)resource; (void)rlim;
    return 0;
}
#define RLIMIT_NPROC 6
#define RLIMIT_MEMLOCK 8
#endif
#else
#include_next <sys/resource.h>
#endif
