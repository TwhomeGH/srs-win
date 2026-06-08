#ifdef _WIN32
#ifndef SRS_COMPAT_SYS_MMAN_H
#define SRS_COMPAT_SYS_MMAN_H
#include <windows.h>
#include <errno.h>
#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define PROT_EXEC  0x4
#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_FIXED     0x10
#define MAP_ANONYMOUS 0x20
#define MAP_FAILED ((void*)-1)
#define MS_SYNC 0
#define MS_ASYNC 1
#define MS_INVALIDATE 2
#define MADV_NORMAL 0
#define MADV_RANDOM 1
#define MADV_SEQUENTIAL 2
#define MADV_WILLNEED 3
#define MADV_DONTNEED 4
static inline void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    (void)addr; (void)prot; (void)fd; (void)offset;
    DWORD type = (flags & MAP_ANONYMOUS) ? MEM_RESERVE | MEM_COMMIT : MEM_RESERVE | MEM_COMMIT;
    void *p = VirtualAlloc(NULL, length, type, PAGE_READWRITE);
    if (!p) return MAP_FAILED;
    return p;
}
static inline int munmap(void *addr, size_t length) {
    return VirtualFree(addr, 0, MEM_RELEASE) ? 0 : -1;
}
static inline int mprotect(void *addr, size_t len, int prot) {
    (void)addr; (void)len; (void)prot;
    return 0;
}
static inline int msync(void *addr, size_t len, int flags) {
    (void)addr; (void)len; (void)flags;
    return 0;
}
static inline int madvise(void *addr, size_t len, int advice) {
    (void)addr; (void)len; (void)advice;
    return 0;
}
#endif
#else
#include_next <sys/mman.h>
#endif
