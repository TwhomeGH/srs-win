#ifdef _WIN32
#ifndef SRS_COMPAT_NETINET_IN_H
#define SRS_COMPAT_NETINET_IN_H
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2def.h>
#ifndef IN6_IS_ADDR_MULTICAST
#define IN6_IS_ADDR_MULTICAST(a) (((uint8_t *)(a))[0] == 0xff)
#endif
#ifndef IN6_IS_ADDR_LINKLOCAL
#define IN6_IS_ADDR_LINKLOCAL(a) ((((uint8_t *)(a))[0] == 0xfe) && ((((uint8_t *)(a))[1] & 0xc0) == 0x80))
#endif
#ifndef IN6_IS_ADDR_SITELOCAL
#define IN6_IS_ADDR_SITELOCAL(a) ((((uint8_t *)(a))[0] == 0xfe) && ((((uint8_t *)(a))[1] & 0xc0) == 0xc0))
#endif
#ifndef IN6_IS_ADDR_LOOPBACK
#define IN6_IS_ADDR_LOOPBACK(a) (memcmp((a), &in6addr_loopback, sizeof(struct in6_addr)) == 0)
#endif
#ifndef IN6_IS_ADDR_UNSPECIFIED
#define IN6_IS_ADDR_UNSPECIFIED(a) (memcmp((a), &in6addr_any, sizeof(struct in6_addr)) == 0)
#endif
#ifndef IN6_IS_ADDR_V4MAPPED
#define IN6_IS_ADDR_V4MAPPED(a) ((((uint8_t *)(a))[0] == 0) && (((uint8_t *)(a))[1] == 0) && (((uint8_t *)(a))[2] == 0) && (((uint8_t *)(a))[3] == 0) && (((uint8_t *)(a))[4] == 0) && (((uint8_t *)(a))[5] == 0) && (((uint8_t *)(a))[6] == 0) && (((uint8_t *)(a))[7] == 0) && (((uint8_t *)(a))[8] == 0) && (((uint8_t *)(a))[9] == 0) && (((uint8_t *)(a))[10] == 0xff) && (((uint8_t *)(a))[11] == 0xff))
#endif
#endif
#else
#include_next <netinet/in.h>
#endif
