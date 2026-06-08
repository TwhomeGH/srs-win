#ifdef _WIN32
#ifndef SRS_COMPAT_SANITIZER_ASAN_INTERFACE_H
#define SRS_COMPAT_SANITIZER_ASAN_INTERFACE_H
// Stub: AddressSanitizer not available on MSVC by default.
#endif
#else
#include_next <sanitizer/asan_interface.h>
#endif
