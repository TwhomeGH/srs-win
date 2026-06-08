#ifdef _WIN32
#ifndef SRS_COMPAT_NET_IF_H
#define SRS_COMPAT_NET_IF_H
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#ifndef IFNAMSIZ
#define IFNAMSIZ 256
#endif
struct ifreq {
    char ifr_name[IFNAMSIZ];
    union {
        struct sockaddr ifru_addr;
        struct sockaddr ifru_dstaddr;
        struct sockaddr ifru_broadaddr;
        struct sockaddr ifru_netmask;
        short ifru_flags;
        int ifru_index;
    } ifr_ifru;
};
#define ifr_addr ifr_ifru.ifru_addr
#define ifr_dstaddr ifr_ifru.ifru_dstaddr
#define ifr_broadaddr ifr_ifru.ifru_broadaddr
#define ifr_netmask ifr_ifru.ifru_netmask
#define ifr_flags ifr_ifru.ifru_flags
#define ifr_index ifr_ifru.ifru_index
#define SIOCGIFADDR 0x8915
#define SIOCGIFNETMASK 0x891b
#define SIOCGIFBRDADDR 0x8919
#define SIOCGIFFLAGS 0x8913
#define SIOCGIFMTU 0x8921
#define SIOCGIFINDEX 0x8933
#endif
#else
#include_next <net/if.h>
#endif
