//
// Copyright (c) 2013-2025 The SRS Authors
//
// SPDX-License-Identifier: MIT
//

#ifndef SRS_KERNEL_FILE_RENAME_HPP
#define SRS_KERNEL_FILE_RENAME_HPP

#include <cerrno>
#include <cstdio>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

// Rename a file, replacing the destination when it exists.
//
// POSIX rename() does this atomically, but the Windows CRT rename() fails with
// EEXIST when the destination exists. HLS rewrites the same playlist path on
// every segment, so use MoveFileEx with replace semantics there.
//
// @return 0 on success, or -1 with errno set.
inline int srs_path_rename(std::string from, std::string to)
{
#ifdef _WIN32
    if (MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING)) {
        return 0;
    }

    switch (GetLastError()) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            errno = ENOENT;
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
            errno = ENOSPC;
            break;
        case ERROR_NOT_SAME_DEVICE:
            errno = EXDEV;
            break;
        default:
            errno = EIO;
            break;
    }
    return -1;
#else
    return ::rename(from.c_str(), to.c_str());
#endif
}

#endif
