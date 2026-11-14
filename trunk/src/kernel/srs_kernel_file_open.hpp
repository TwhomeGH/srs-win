//
// Copyright (c) 2013-2025 The SRS Authors
//
// SPDX-License-Identifier: MIT
//

#ifndef SRS_KERNEL_FILE_OPEN_HPP
#define SRS_KERNEL_FILE_OPEN_HPP

#include <cerrno>
#include <fcntl.h>
#include <string>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#include <srs_win32_errno.h>
#else
#include <unistd.h>
#endif

// Open a file for reading.
//
// On Windows the CRT _open path uses a share mode that does not include
// FILE_SHARE_DELETE. HLS refreshes the same m3u8 while an HTTP client may be
// reading it, so opt into delete sharing explicitly; otherwise MoveFileEx
// fails with ERROR_SHARING_VIOLATION.
inline int srs_open_readonly(const char *path, int oflag, ...)
{
#ifdef _WIN32
    (void)oflag;
    HANDLE handle = CreateFileA(
        path,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        srs_win32_error_to_errno(GetLastError());
        return -1;
    }

    int fd = _open_osfhandle((intptr_t)handle, _O_RDONLY | _O_BINARY);
    if (fd < 0) {
        CloseHandle(handle);
        errno = EMFILE;
        return -1;
    }
    return fd;
#else
    return ::open(path, oflag);
#endif
}

#endif
