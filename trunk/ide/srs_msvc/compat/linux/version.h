#ifdef _WIN32
#ifndef SRS_COMPAT_LINUX_VERSION_H
#define SRS_COMPAT_LINUX_VERSION_H
#define LINUX_VERSION_CODE 0
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#endif
#else
#include_next <linux/version.h>
#endif
