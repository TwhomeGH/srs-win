#ifdef _WIN32
#ifndef SRS_COMPAT_SYS_UTSNAME_H
#define SRS_COMPAT_SYS_UTSNAME_H
#include <windows.h>
#include <sysinfoapi.h>
struct utsname {
    char sysname[65];
    char nodename[257];
    char release[65];
    char version[65];
    char machine[65];
};
static inline int uname(struct utsname *name) {
    OSVERSIONINFOA vi;
    vi.dwOSVersionInfoSize = sizeof(vi);
    GetVersionExA(&vi);
    strncpy(name->sysname, "Windows", sizeof(name->sysname));
    DWORD sz = sizeof(name->nodename);
    GetComputerNameA(name->nodename, &sz);
    snprintf(name->release, sizeof(name->release), "%lu.%lu", vi.dwMajorVersion, vi.dwMinorVersion);
    snprintf(name->version, sizeof(name->version), "Build %lu", vi.dwBuildNumber);
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    switch (si.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64: strcpy(name->machine, "x86_64"); break;
        case PROCESSOR_ARCHITECTURE_INTEL: strcpy(name->machine, "i686"); break;
        case PROCESSOR_ARCHITECTURE_ARM64: strcpy(name->machine, "aarch64"); break;
        default: strcpy(name->machine, "unknown"); break;
    }
    return 0;
}
#endif
#else
#include_next <sys/utsname.h>
#endif
