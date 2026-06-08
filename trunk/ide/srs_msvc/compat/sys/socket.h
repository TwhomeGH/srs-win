#ifdef _WIN32
#ifndef SRS_COMPAT_SYS_SOCKET_H
#define SRS_COMPAT_SYS_SOCKET_H
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#undef SOCKET
#define SOCKET int
#ifndef AF_UNIX
#define AF_UNIX AF_INET
#endif
struct sockaddr_un {
    short sun_family;
    char sun_path[108];
};
#define SUN_LEN(su) (sizeof(*(su)))

// struct msghdr for Windows (POSIX socket ancillary data)
struct msghdr {
    void *msg_name;
    int msg_namelen;
    struct iovec *msg_iov;
    int msg_iovlen;
    void *msg_control;
    int msg_controllen;
    int msg_flags;
};

// Note: getsockopt/setsockopt POSIX wrappers are in force_include.h
// (since many SRS files don't include <sys/socket.h> but still use these)
#endif
#else
#include_next <sys/socket.h>
#endif
