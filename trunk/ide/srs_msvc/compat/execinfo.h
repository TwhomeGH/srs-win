#ifdef _WIN32
#ifndef SRS_COMPAT_EXECINFO_H
#define SRS_COMPAT_EXECINFO_H
#include <windows.h>
#include <dbghelp.h>
static inline int backtrace(void **buffer, int size) {
    return CaptureStackBackTrace(0, size, buffer, NULL);
}
static inline char **backtrace_symbols(void *const *buffer, int size) {
    char **symbols = (char**)malloc(sizeof(char*) * size);
    for (int i = 0; i < size; i++) {
        char buf[256];
        snprintf(buf, sizeof(buf), "[%d] 0x%p", i, buffer[i]);
        symbols[i] = _strdup(buf);
    }
    return symbols;
}
#endif
#else
#include_next <execinfo.h>
#endif
