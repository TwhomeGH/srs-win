#ifdef _WIN32
#ifndef SRS_COMPAT_ARPA_INET_H
#define SRS_COMPAT_ARPA_INET_H
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#undef inet_ntop
static inline const char *srs_inet_ntop(int af, const void *src, char *dst, socklen_t size) {
    if (af == AF_INET) {
        struct sockaddr_in sin;
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        memcpy(&sin.sin_addr, src, sizeof(struct in_addr));
        DWORD len = size;
        if (WSAAddressToStringA((struct sockaddr*)&sin, sizeof(sin), NULL, dst, &len) == 0)
            return dst;
    } else if (af == AF_INET6) {
        struct sockaddr_in6 sin6;
        memset(&sin6, 0, sizeof(sin6));
        sin6.sin6_family = AF_INET6;
        memcpy(&sin6.sin6_addr, src, sizeof(struct in6_addr));
        DWORD len = size;
        if (WSAAddressToStringA((struct sockaddr*)&sin6, sizeof(sin6), NULL, dst, &len) == 0)
            return dst;
    }
    return NULL;
}
#define inet_ntop srs_inet_ntop
static inline int srs_inet_pton(int af, const char *src, void *dst) {
    struct sockaddr_storage ss;
    int ss_len = sizeof(ss);
    if (WSAStringToAddressA((char*)src, af, NULL, (struct sockaddr*)&ss, &ss_len) == 0) {
        if (af == AF_INET) {
            memcpy(dst, &((struct sockaddr_in*)&ss)->sin_addr, sizeof(struct in_addr));
            return 1;
        } else if (af == AF_INET6) {
            memcpy(dst, &((struct sockaddr_in6*)&ss)->sin6_addr, sizeof(struct in6_addr));
            return 1;
        }
    }
    return 0;
}
#define inet_pton srs_inet_pton
#endif
#else
#include_next <arpa/inet.h>
#endif
