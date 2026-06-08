//
// Copyright (c) 2013-2025 The SRS Authors
//
// SPDX-License-Identifier: MIT
//

#ifndef SRS_CORE_PLATFORM_HPP
#define SRS_CORE_PLATFORM_HPP

// For 32bit os, 2G big file limit for unistd io,
// ie. read/write/lseek to use 64bits size for huge file.
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

// For int64_t print using PRId64 format.
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

// For RTC/FFMPEG build.
#if !defined(__STDC_CONSTANT_MACROS)
#define __STDC_CONSTANT_MACROS
#endif

// For srs-librtmp, @see https://github.com/ossrs/srs/issues/213
#include <inttypes.h>

#include <stddef.h>
#ifndef _WIN32
#include <sys/types.h>
#else
// Windows: winsock2.h must be included before windows.h
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef FD_SETSIZE
#define FD_SETSIZE 8192
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>
#include <io.h>
#include <stdint.h>
// Some MSVC compatibility defines
#define _FILE_OFFSET_BITS 64
#ifndef _SSIZE_T_DEFINED
#ifdef _WIN64
typedef __int64 ssize_t;
#else
typedef int ssize_t;
#endif
#define _SSIZE_T_DEFINED
#endif
// POSIX function name mappings
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
// socklen_t
#ifndef socklen_t
#define socklen_t int
#endif
#endif

// For CentOS 6 or C++98, @see https://github.com/ossrs/srs/issues/2815
#ifndef UINT32_MAX
#define UINT32_MAX (4294967295U)
#endif

#endif
