// SPDX-License-Identifier: MIT
// Windows platform compatibility header for SRS.
// Include this first in all SRS source files when building on Windows/MSVC.

#ifndef SRS_WIN32_COMPAT_H
#define SRS_WIN32_COMPAT_H

#ifdef _WIN32

// Must define WIN32_LEAN_AND_MEAN before including windows headers to reduce bloat
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Define FD_SETSIZE large enough for SRS usage
#ifndef FD_SETSIZE
#define FD_SETSIZE 8192
#endif

// Suppress MSVC deprecation warnings for POSIX functions
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <mswsock.h>
#include <process.h>
#include <io.h>
#include <direct.h>

// POSIX compatibility defines for Windows

// Socket I/O - these wrap Winsock functions
// For SRS code that uses read()/write() on sockets, redirect to recv/send
#ifdef read
#undef read
#endif
#ifdef write
#undef write
#endif
#ifdef close
#undef close
#endif

static inline int srs_read_sock(int fd, void *buf, size_t n) {
    int r = recv((SOCKET)fd, (char*)buf, (int)n, 0);
    if (r == SOCKET_ERROR) {
        errno = EIO;
        int wsa_err = WSAGetLastError();
        if (wsa_err == WSAEWOULDBLOCK) errno = EAGAIN;
        else if (wsa_err == WSAEINTR) errno = EINTR;
        else if (wsa_err == WSAECONNRESET) errno = ECONNRESET;
    }
    return r;
}

static inline int srs_write_sock(int fd, const void *buf, size_t n) {
    int r = send((SOCKET)fd, (const char*)buf, (int)n, 0);
    if (r == SOCKET_ERROR) {
        errno = EIO;
        int wsa_err = WSAGetLastError();
        if (wsa_err == WSAEWOULDBLOCK) errno = EAGAIN;
        else if (wsa_err == WSAEINTR) errno = EINTR;
        else if (wsa_err == WSAECONNRESET) errno = ECONNRESET;
    }
    return r;
}

static inline int srs_close_sock(int fd) {
    int r = closesocket((SOCKET)fd);
    if (r == SOCKET_ERROR) {
        errno = EIO;
        int wsa_err = WSAGetLastError();
        if (wsa_err == WSAENOTSOCK) errno = ENOTSOCK;
    }
    return r;
}

// For plain old file I/O (not sockets), use the CRT functions
#define srs_read_file(fd, buf, n)  _read((fd), (buf), (unsigned int)(n))
#define srs_write_file(fd, buf, n) _write((fd), (buf), (unsigned int)(n))
#define srs_close_file(fd)         _close((fd))
#define srs_open(path, flags, ...) _open((path), (flags), ##__VA_ARGS__)
#define srs_lseek(fd, off, wh)     _lseek((fd), (off), (wh))

// POSIX function name mappings for MSVC
#define strcasecmp    _stricmp
#define strncasecmp   _strnicmp
#define access        _access
#define unlink        _unlink
#define mkdir(p, m)   _mkdir(p)
#define rmdir         _rmdir
#define getcwd        _getcwd
#define chdir         _chdir
#define getpid        _getpid

// ioctl mapping
#ifdef ioctl
#undef ioctl
#endif
#define ioctl(fd, req, arg) ioctlsocket((SOCKET)(fd), (req), (u_long*)(arg))

// ssize_t (MSVC defines it only in some modes)
#ifndef _SSIZE_T_DEFINED
#ifdef _WIN64
typedef __int64 ssize_t;
#else
typedef int ssize_t;
#endif
#define _SSIZE_T_DEFINED
#endif

// mode_t (MSVC defines it in sys/stat.h)
#ifndef _MODE_T_DEFINED
typedef unsigned short mode_t;
#define _MODE_T_DEFINED
#endif

// socklen_t
#ifndef socklen_t
#define socklen_t int
#endif

// O_NONBLOCK (MSVC doesn't define this)
#ifndef O_NONBLOCK
#define O_NONBLOCK 0x4000
#endif

// SHUT_RD, SHUT_WR, SHUT_RDWR
#ifndef SHUT_RD
#define SHUT_RD   0
#define SHUT_WR   1
#define SHUT_RDWR 2
#endif

// MSG_NOSIGNAL (not available on Windows)
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

// SO_REUSEPORT (not available on Windows, use SO_REUSEADDR instead)
#ifndef SO_REUSEPORT
#define SO_REUSEPORT SO_REUSEADDR
#endif

// AI_NUMERICSERV (may not be defined in older SDKs)
#ifndef AI_NUMERICSERV
#define AI_NUMERICSERV 0x00000004
#endif

// SIGPIPE (not available on Windows)
#ifndef SIGPIPE
#define SIGPIPE 13
#endif
#ifndef SIG_BLOCK
#define SIG_BLOCK 1
#endif

// getpagesize() - on Windows, use GetSystemInfo
static inline int srs_getpagesize(void) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (int)si.dwPageSize;
}

// clock_gettime emulation using QueryPerformanceCounter
struct timespec;
static inline int srs_clock_gettime_monotonic(struct timespec *ts) {
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    ts->tv_sec = (long)(counter.QuadPart / freq.QuadPart);
    ts->tv_nsec = (long)((counter.QuadPart % freq.QuadPart) * 1000000000LL / freq.QuadPart);
    return 0;
}

// gettimeofday emulation
struct timezone;
static inline int srs_gettimeofday(struct timeval *tv, struct timezone *tz) {
    FILETIME ft;
    ULARGE_INTEGER li;
    GetSystemTimeAsFileTime(&ft);
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    // Convert from 100-ns intervals since Jan 1 1601 to microseconds since Jan 1 1970
    const long long EPOCH_DIFFERENCE = 116444736000000000LL;
    long long us = (li.QuadPart - EPOCH_DIFFERENCE) / 10;
    tv->tv_sec = (long)(us / 1000000);
    tv->tv_usec = (long)(us % 1000000);
    return 0;
}

// socketpair emulation (for SRS internal pipes)
static inline int srs_socketpair(int domain, int type, int protocol, int sv[2]) {
    // On Windows, socketpair is not directly available.
    // Use a loopback pair via localhost TCP connection.
    if (domain != AF_UNIX) {
        errno = EOPNOTSUPP;
        return -1;
    }
    // Fallback: create a TCP loopback connection
    SOCKET listener = socket(AF_INET, type, protocol);
    if (listener == INVALID_SOCKET) return -1;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(listener); return -1;
    }
    if (listen(listener, 1) == SOCKET_ERROR) {
        closesocket(listener); return -1;
    }
    int len = sizeof(addr);
    getsockname(listener, (struct sockaddr*)&addr, &len);
    SOCKET client = socket(AF_INET, type, protocol);
    if (client == INVALID_SOCKET) {
        closesocket(listener); return -1;
    }
    if (connect(client, (struct sockaddr*)&addr, len) == SOCKET_ERROR) {
        closesocket(listener); closesocket(client); return -1;
    }
    SOCKET server = accept(listener, (struct sockaddr*)&addr, &len);
    if (server == INVALID_SOCKET) {
        closesocket(listener); closesocket(client); return -1;
    }
    closesocket(listener);
    sv[0] = (int)client;
    sv[1] = (int)server;
    return 0;
}

// Environment variable helper
static inline char *srs_getenv(const char *name) {
    static char buf[4096];
    DWORD ret = GetEnvironmentVariableA(name, buf, sizeof(buf));
    if (ret == 0 || ret >= sizeof(buf)) return NULL;
    return buf;
}

// FIONBIO helper for Winsock
static inline int srs_set_nonblock(int fd, int nonblock) {
    u_long mode = (nonblock ? 1 : 0);
    return ioctlsocket((SOCKET)fd, FIONBIO, &mode);
}

// Set close-on-exec for socket (Windows uses SetHandleInformation)
static inline int srs_set_cloexec(int fd) {
    HANDLE h = (HANDLE)_get_osfhandle(fd);
    if (h == INVALID_HANDLE_VALUE) return -1;
    if (!SetHandleInformation(h, HANDLE_FLAG_INHERIT, 0)) return -1;
    return 0;
}

#else
// On non-Windows, these are just pass-through
#define srs_getpagesize() getpagesize()
#define srs_set_nonblock(fd, nb) (-1)
#define srs_set_cloexec(fd) (-1)
#endif // _WIN32

#endif // SRS_WIN32_COMPAT_H
