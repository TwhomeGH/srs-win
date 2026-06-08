#ifdef _WIN32
#ifndef SRS_COMPAT_SYS_SYSCTL_H
#define SRS_COMPAT_SYS_SYSCTL_H
#include <windows.h>
#include <stdint.h>
static inline int sysctl(int *name, unsigned int namelen, void *oldp, size_t *oldlenp, void *newp, size_t newlen) {
    (void)name; (void)namelen; (void)oldp; (void)oldlenp; (void)newp; (void)newlen;
    errno = ENOSYS;
    return -1;
}
#endif
#else
#include_next <sys/sysctl.h>
#endif
