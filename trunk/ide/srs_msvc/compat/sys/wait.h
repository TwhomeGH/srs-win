#ifdef _WIN32
#ifndef SRS_COMPAT_SYS_WAIT_H
#define SRS_COMPAT_SYS_WAIT_H
#include <process.h>
#include <windows.h>
#define WNOHANG 1
#define WUNTRACED 2
#define WCONTINUED 8
#define WEXITSTATUS(s) (((s) & 0xff00) >> 8)
#define WTERMSIG(s) ((s) & 0x7f)
#define WIFEXITED(s) (WTERMSIG(s) == 0)
#define WIFSIGNALED(s) (((s) & 0x7f) > 0 && ((s) & 0x7f) < 0x7f)
#define WIFSTOPPED(s) (((s) & 0xff) == 0x7f)
#define WSTOPSIG(s) WEXITSTATUS(s)
static inline pid_t wait(int *status) {
    (void)status;
    return -1;
}
static inline pid_t waitpid(pid_t pid, int *status, int options) {
    (void)pid; (void)status; (void)options;
    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, (DWORD)pid);
    if (!h) return -1;
    WaitForSingleObject(h, (options & WNOHANG) ? 0 : INFINITE);
    if (options & WNOHANG) {
        DWORD ec;
        if (!GetExitCodeProcess(h, &ec)) { CloseHandle(h); return -1; }
        if (ec == STILL_ACTIVE) { CloseHandle(h); return 0; }
    }
    CloseHandle(h);
    return pid;
}
#endif
#else
#include_next <sys/wait.h>
#endif
