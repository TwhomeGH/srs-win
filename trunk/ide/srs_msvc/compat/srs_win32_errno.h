//
// Copyright (c) 2013-2025 The SRS Authors
//
// SPDX-License-Identifier: MIT
//

#ifndef SRS_WIN32_ERRNO_H
#define SRS_WIN32_ERRNO_H

#ifdef _WIN32
#include <cerrno>
#include <windows.h>

// Translate a Win32 error into the errno value SRS logs and checks.
//
// File and process helpers must not return a bare -1: callers frequently log
// errno, and a stale or zero errno makes platform failures indistinguishable
// from success. Socket code uses WSAGetLastError() separately.
inline void srs_win32_error_to_errno(DWORD error)
{
    switch (error) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            errno = ENOENT;
            break;
        case ERROR_INVALID_HANDLE:
            errno = EBADF;
            break;
        case ERROR_ACCESS_DENIED:
        case ERROR_SHARING_VIOLATION:
            errno = EACCES;
            break;
        case ERROR_ALREADY_EXISTS:
        case ERROR_FILE_EXISTS:
            errno = EEXIST;
            break;
        case ERROR_DISK_FULL:
        case ERROR_HANDLE_DISK_FULL:
            errno = ENOSPC;
            break;
        case ERROR_NOT_SAME_DEVICE:
            errno = EXDEV;
            break;
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_OUTOFMEMORY:
            errno = ENOMEM;
            break;
        case ERROR_INVALID_PARAMETER:
            errno = EINVAL;
            break;
        case ERROR_BROKEN_PIPE:
        case ERROR_NO_DATA:
            errno = EPIPE;
            break;
        case ERROR_OPERATION_ABORTED:
            errno = EINTR;
            break;
        case ERROR_TIMEOUT:
            errno = ETIMEDOUT;
            break;
        case ERROR_NETNAME_DELETED:
            errno = ECONNRESET;
            break;
        default:
            errno = EIO;
            break;
    }
}

#endif
#endif
