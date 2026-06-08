#ifdef _WIN32
#ifndef SRS_COMPAT_NETDB_H
#define SRS_COMPAT_NETDB_H
// winsock2.h already provides hostent, servent, protoent.
// We only provide the extras that POSIX netdb.h has but Windows lacks.
#include <winsock2.h>
#include <ws2tcpip.h>
#ifndef AI_NUMERICSERV
#define AI_NUMERICSERV 0x00000004
#endif
#ifndef gai_strerror
static inline const char *srs_gai_strerror(int ecode) {
    switch (ecode) {
        case EAI_AGAIN: return "Temporary failure in name resolution";
        case EAI_BADFLAGS: return "Invalid flags";
        case EAI_FAIL: return "Non-recoverable failure in name resolution";
        case EAI_FAMILY: return "Address family not supported";
        case EAI_MEMORY: return "Memory allocation failure";
        case EAI_NONAME: return "Name or service not known";
        case EAI_SERVICE: return "Service not supported for socket type";
        case EAI_SOCKTYPE: return "Socket type not supported";
        case EAI_SYSTEM: return "System error";
        default: return "Unknown error";
    }
}
#define gai_strerror srs_gai_strerror
#endif
#ifndef h_errno
#define h_errno errno
#endif
#ifndef HOST_NOT_FOUND
#define HOST_NOT_FOUND 1
#endif
#ifndef TRY_AGAIN
#define TRY_AGAIN 2
#endif
#ifndef NO_RECOVERY
#define NO_RECOVERY 3
#endif
#ifndef NO_ADDRESS
#define NO_ADDRESS 4
#endif
#ifndef NO_DATA
#define NO_DATA NO_ADDRESS
#endif
#endif
#else
#include_next <netdb.h>
#endif
