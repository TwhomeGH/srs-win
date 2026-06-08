#ifdef _WIN32
#ifndef SRS_COMPAT_IFADDRS_H
#define SRS_COMPAT_IFADDRS_H
#include <winsock2.h>
#include <iphlpapi.h>
#include <stdint.h>
struct ifaddrs {
    struct ifaddrs *ifa_next;
    char *ifa_name;
    unsigned int ifa_flags;
    struct sockaddr *ifa_addr;
    struct sockaddr *ifa_netmask;
    struct sockaddr *ifa_dstaddr;
    void *ifa_data;
};
#ifndef IFF_UP
#define IFF_UP 0x1
#endif
#ifndef IFF_BROADCAST
#define IFF_BROADCAST 0x2
#endif
#ifndef IFF_LOOPBACK
#define IFF_LOOPBACK 0x8
#endif
#ifndef IFF_POINTOPOINT
#define IFF_POINTOPOINT 0x10
#endif
#ifndef IFF_MULTICAST
#define IFF_MULTICAST 0x1000
#endif
#ifndef IFF_RUNNING
#define IFF_RUNNING 0x40
#endif
#ifndef IFF_PROMISC
#define IFF_PROMISC 0x100
#endif
static inline int getifaddrs(struct ifaddrs **ifap) {
    DWORD bufsize = 0;
    GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, NULL, &bufsize);
    IP_ADAPTER_ADDRESSES *buf = (IP_ADAPTER_ADDRESSES*)malloc(bufsize);
    if (!buf) return -1;
    ULONG r = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, buf, &bufsize);
    if (r != NO_ERROR) { free(buf); return -1; }
    struct ifaddrs *head = NULL, *tail = NULL;
    for (IP_ADAPTER_ADDRESSES *pa = buf; pa; pa = pa->Next) {
        for (IP_ADAPTER_UNICAST_ADDRESS *ua = pa->FirstUnicastAddress; ua; ua = ua->Next) {
            struct ifaddrs *ifa = (struct ifaddrs*)calloc(1, sizeof(struct ifaddrs));
            if (!ifa) continue;
            ifa->ifa_name = _strdup(pa->AdapterName);
            ifa->ifa_flags = (pa->IfType == IF_TYPE_SOFTWARE_LOOPBACK ? IFF_LOOPBACK : IFF_UP) | IFF_RUNNING;
            ifa->ifa_addr = (struct sockaddr*)malloc(ua->Address.iSockaddrLength);
            memcpy(ifa->ifa_addr, ua->Address.lpSockaddr, ua->Address.iSockaddrLength);
            ifa->ifa_next = NULL;
            if (tail) tail->ifa_next = ifa;
            else head = ifa;
            tail = ifa;
        }
    }
    free(buf);
    *ifap = head;
    return 0;
}
static inline void freeifaddrs(struct ifaddrs *ifa) {
    while (ifa) {
        struct ifaddrs *next = ifa->ifa_next;
        free(ifa->ifa_name);
        free(ifa->ifa_addr);
        free(ifa);
        ifa = next;
    }
}
#endif
#else
#include_next <ifaddrs.h>
#endif
