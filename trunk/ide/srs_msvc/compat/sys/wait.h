#ifdef _WIN32
#ifndef SRS_COMPAT_SYS_WAIT_H
#define SRS_COMPAT_SYS_WAIT_H
#include <process.h>
#include <windows.h>
#include <srs_win32_errno.h>
#define WNOHANG 1
#define WUNTRACED 2
#define WCONTINUED 8
#define WEXITSTATUS(s) (((s) & 0xff00) >> 8)
#define WTERMSIG(s) ((s) & 0x7f)
#define WIFEXITED(s) (WTERMSIG(s) == 0)
#define WIFSIGNALED(s) (((s) & 0x7f) > 0 && ((s) & 0x7f) < 0x7f)
#define WIFSTOPPED(s) (((s) & 0xff) == 0x7f)
#define WSTOPSIG(s) WEXITSTATUS(s)

// Windows can only wait for a specific process. POSIX wait(-1) has no
// equivalent without enumerating all children, so fail explicitly.
static inline pid_t wait(int *status) {
    (void)status;
    errno = ENOSYS;
    return -1;
}

static inline pid_t waitpid(pid_t pid, int *status, int options) {
    if (pid <= 0) {
        errno = ENOSYS;
        return -1;
    }

    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, (DWORD)pid);
    if (!h) {
        // OpenProcess uses ERROR_INVALID_PARAMETER for a nonexistent pid.
        DWORD error = GetLastError();
        errno = error == ERROR_INVALID_PARAMETER ? ESRCH : EINVAL;
        if (error != ERROR_INVALID_PARAMETER) {
            srs_win32_error_to_errno(error);
        }
        return -1;
    }

    DWORD wait_result = WaitForSingleObject(h, (options & WNOHANG) ? 0 : INFINITE);
    if (wait_result == WAIT_TIMEOUT) {
        CloseHandle(h);
        return 0;
    }
    if (wait_result == WAIT_FAILED) {
        srs_win32_error_to_errno(GetLastError());
        CloseHandle(h);
        return -1;
    }

    DWORD exit_code = 0;
    if (!GetExitCodeProcess(h, &exit_code)) {
        srs_win32_error_to_errno(GetLastError());
        CloseHandle(h);
        return -1;
    }
    CloseHandle(h);

    if (status) {
        // POSIX stores the exit code in bits 8-15. Windows has no signal
        // status, so a terminated process is reported as a normal exit.
        *status = (int)((exit_code & 0xff) << 8);
    }
    return pid;
}
#endif
#else
#include_next <sys/wait.h>
#endif
