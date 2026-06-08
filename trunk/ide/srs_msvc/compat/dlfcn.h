#ifdef _WIN32
#ifndef SRS_COMPAT_DLFCN_H
#define SRS_COMPAT_DLFCN_H
#include <windows.h>
#define RTLD_LAZY   0
#define RTLD_NOW    0
#define RTLD_GLOBAL 0
#define RTLD_LOCAL  0
static inline void *dlopen(const char *file, int mode) {
    return (void*)LoadLibraryA(file);
}
static inline int dlclose(void *handle) {
    return FreeLibrary((HMODULE)handle) ? 0 : -1;
}
static inline void *dlsym(void *handle, const char *name) {
    return (void*)GetProcAddress((HMODULE)handle, name);
}
static inline const char *dlerror(void) {
    static char buf[256];
    DWORD err = GetLastError();
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, buf, sizeof(buf), NULL);
    return buf;
}
#endif
#else
#include_next <dlfcn.h>
#endif
