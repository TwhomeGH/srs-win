#ifdef _WIN32
#ifndef SRS_COMPAT_NETINET_TCP_H
#define SRS_COMPAT_NETINET_TCP_H
#include <winsock2.h>
#include <ws2tcpip.h>
#ifndef TCP_NODELAY
#define TCP_NODELAY 0x0001
#endif
#ifndef TCP_QUICKACK
#define TCP_QUICKACK 0x000c
#endif
#ifndef TCP_KEEPIDLE
#define TCP_KEEPIDLE 3
#endif
#ifndef TCP_KEEPINTVL
#define TCP_KEEPINTVL 5
#endif
#ifndef TCP_KEEPCNT
#define TCP_KEEPCNT 6
#endif
#endif
#else
#include_next <netinet/tcp.h>
#endif
