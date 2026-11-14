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

// Map Win32 file errors back to the errno values SRS logs and callers expect.
inline void srs_win32_error_to_errno(DWORD error)
{
    switch (error) {
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
}

inline std::string srs_rename_backup_path(const std::string &to)
{
    for (int i = 0; i < 100; i++) {
        std::string candidate = to + ".old." + std::to_string(i);
        if (GetFileAttributesA(candidate.c_str()) == INVALID_FILE_ATTRIBUTES) {
            return candidate;
        }
    }
    return to + ".old";
}
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

    DWORD error = GetLastError();
    if (error != ERROR_ACCESS_DENIED && error != ERROR_SHARING_VIOLATION) {
        srs_win32_error_to_errno(error);
        return -1;
    }

    // Windows cannot replace a destination that is still open, even when the
    // reader allowed FILE_SHARE_DELETE. Move the open destination aside, put
    // the new file in place, then release the old name. This keeps an HTTP
    // reader on its old handle without making the playlist refresh fail.
    if (GetFileAttributesA(to.c_str()) == INVALID_FILE_ATTRIBUTES) {
        srs_win32_error_to_errno(error);
        return -1;
    }

    std::string backup = srs_rename_backup_path(to);
    if (!MoveFileExA(to.c_str(), backup.c_str(), 0)) {
        srs_win32_error_to_errno(GetLastError());
        return -1;
    }

    if (!MoveFileExA(from.c_str(), to.c_str(), 0)) {
        DWORD move_error = GetLastError();
        // Best effort: put the old playlist back so a later refresh can retry.
        MoveFileExA(backup.c_str(), to.c_str(), 0);
        srs_win32_error_to_errno(move_error);
        return -1;
    }

    // The old handle may still be open, so this can fail transiently; leave
    // the unreferenced backup for the next cleanup rather than failing HLS.
    DeleteFileA(backup.c_str());
    return 0;
#else
    return ::rename(from.c_str(), to.c_str());
#endif
}

#endif
