// Minimal C header for ffmpeg_stubs.c compilation.
// This avoids pulling in C++ headers that cause MSVC to fail when compiling .c files.

#ifdef _WIN32

// Must be defined before including windows.h
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Exclude some windows.h bloat
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

// Big FD_SETSIZE for SRS select() usage
#ifndef FD_SETSIZE
#define FD_SETSIZE 8192
#endif

// Suppress deprecation warnings
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

// Include Windows headers first
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <windows.h>
#include <process.h>
#include <io.h>
#include <direct.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>

// wingdi.h defines ERROR as 0 which breaks SRS error codes (ERROR_xxx)
#ifdef ERROR
#undef ERROR
#endif

// MSVC-specific suppress warnings
#pragma warning(disable: 4996)  // _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4267)  // conversion from size_t to int
#pragma warning(disable: 4244)  // conversion loss of data

#endif // _WIN32