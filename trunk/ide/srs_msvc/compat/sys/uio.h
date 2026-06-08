#ifdef _WIN32
#ifndef SRS_COMPAT_SYS_UIO_H
#define SRS_COMPAT_SYS_UIO_H
#include <winsock2.h>
#ifndef HAVE_STRUCT_IOVEC
struct iovec {
    void *iov_base;
    size_t iov_len;
};
#endif
static inline int readv(int fd, const struct iovec *iov, int iovcnt) {
    DWORD bytesRead;
    DWORD flags = 0;
    int r = WSARecv((SOCKET)fd, (WSABUF*)iov, iovcnt, &bytesRead, &flags, NULL, NULL);
    if (r == SOCKET_ERROR) { errno = EIO; return -1; }
    return (int)bytesRead;
}
static inline int writev(int fd, const struct iovec *iov, int iovcnt) {
    DWORD bytesWritten;
    int r = WSASend((SOCKET)fd, (WSABUF*)iov, iovcnt, &bytesWritten, 0, NULL, NULL);
    if (r == SOCKET_ERROR) { errno = EIO; return -1; }
    return (int)bytesWritten;
}
#endif
#else
#include_next <sys/uio.h>
#endif
