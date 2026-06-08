// Force-included header for all SRS files when compiling on Windows/MSVC.
// This handles POSIX compatibility before any SRS code is compiled.
// Used via: /FI"force_include.h" in CMakeLists.txt
//


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
#include <tlhelp32.h>

// wingdi.h defines ERROR as 0 which breaks SRS error codes (ERROR_xxx)
#ifdef ERROR
#undef ERROR
#endif

// MSVC-specific suppress warnings
#pragma warning(disable: 4996)  // _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4267)  // conversion from size_t to int
#pragma warning(disable: 4244)  // conversion loss of data
#pragma warning(disable: 4018)  // signed/unsigned mismatch
#pragma warning(disable: 4800)  // forcing value to bool
#pragma warning(disable: 4250)  // inherits via dominance
#pragma warning(disable: 4506)  // inline function
#pragma warning(disable: 4068)  // unknown pragma (e.g. #pragma GCC)

// POSIX types that MSVC doesn't define
#ifndef _SSIZE_T_DEFINED
#ifdef _WIN64
typedef __int64 ssize_t;
#else
typedef int ssize_t;
#endif
#define _SSIZE_T_DEFINED
#endif

#ifndef socklen_t
#define socklen_t int
#endif

#ifndef _MODE_T_DEFINED
typedef unsigned short mode_t;
#define _MODE_T_DEFINED
#endif

// iovec struct (Winsock uses WSABUF which is different layout)
#ifndef HAVE_STRUCT_IOVEC
struct iovec {
    void *iov_base;
    size_t iov_len;
};
#define HAVE_STRUCT_IOVEC
#endif

// O_* flags (system <fcntl.h> may not define all POSIX ones)
#ifndef O_RDONLY
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2
#endif
#ifndef O_CREAT
#define O_CREAT  0x100
#define O_TRUNC  0x200
#define O_APPEND 0x400
#define O_EXCL   0x800
#endif
#ifndef O_TEXT
#define O_TEXT   0x4000
#define O_BINARY 0x8000
#endif
#ifndef O_CLOEXEC
#define O_CLOEXEC 0x1000000
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK 0x4000
#endif
#ifndef O_NDELAY
#define O_NDELAY O_NONBLOCK
#endif
#ifndef F_GETFL
#define F_GETFL 3
#define F_SETFL 4
#define F_GETFD 1
#define F_SETFD 2
#define FD_CLOEXEC 1
#endif

// POSIX sysconf constants (not available on Windows)
#ifndef _SC_IOV_MAX
#define _SC_IOV_MAX 0
#endif
#ifndef _SC_CLK_TCK
#define _SC_CLK_TCK 2
#endif
#ifndef _SC_OPEN_MAX
#define _SC_OPEN_MAX 3
#endif
#ifndef _SC_NPROCESSORS_CONF
#define _SC_NPROCESSORS_CONF 4
#endif
#ifndef _SC_NPROCESSORS_ONLN
#define _SC_NPROCESSORS_ONLN 5
#endif
static inline long sysconf(int name) {
    if (name == _SC_IOV_MAX) return 1024;
    if (name == _SC_CLK_TCK) return 100;
    if (name == _SC_OPEN_MAX) return 8192;
    if (name == _SC_NPROCESSORS_CONF || name == _SC_NPROCESSORS_ONLN) {
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        return sysinfo.dwNumberOfProcessors;
    }
    return -1;
}

// POSIX function name mappings
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define chdir _chdir
#define getcwd _getcwd
#define putenv _putenv
#define unlink _unlink
#define access _access

// setenv/clearenv (Windows doesn't have these natively)
static inline int srs_setenv(const char *name, const char *value, int overwrite) {
    if (!overwrite && GetEnvironmentVariableA(name, NULL, 0) > 0) return 0;
    return SetEnvironmentVariableA(name, value) ? 0 : -1;
}
static inline int srs_unsetenv(const char *name) {
    return SetEnvironmentVariableA(name, NULL) ? 0 : -1;
}
#define setenv srs_setenv
#define unsetenv srs_unsetenv

// Socket constants
#ifndef SHUT_RD
#define SHUT_RD 0
#define SHUT_WR 1
#define SHUT_RDWR 2
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

// SIGPIPE (not used on Windows, but referenced)
#ifndef SIGPIPE
#define SIGPIPE 13
#endif

// ============================================================
// sys/stat.h POSIX additions (not provided by Windows SDK)
// ============================================================
#ifndef S_ISREG
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)  (0)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISLNK(m)  (0)
#define S_ISSOCK(m) (0)
#endif
#ifndef S_IRWXU
#define S_IRWXU 0700
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRWXG 0070
#define S_IRGRP 0040
#define S_IWGRP 0020
#define S_IXGRP 0010
#define S_IRWXO 0007
#define S_IROTH 0004
#define S_IWOTH 0002
#define S_IXOTH 0001
#endif
#ifndef S_ISUID
#define S_ISUID 04000
#define S_ISGID 02000
#define S_ISVTX 01000
#endif

// Map POSIX stat/fstat/lstat to Windows 64-bit versions
#ifndef stat
#define stat _stat64
#define fstat _fstat64
#define lstat _stat64
#endif
#ifndef mkdir
#define mkdir(path,mode) _mkdir(path)
#endif

// ============================================================
// sys/types.h POSIX additions (not provided by Windows SDK)
// ============================================================
#ifndef _PID_T_DEFINED
typedef int pid_t;
#define _PID_T_DEFINED
#endif
#ifndef _UID_T_DEFINED
typedef unsigned int uid_t;
typedef unsigned int gid_t;
#define _UID_T_DEFINED
#endif
#ifndef _OFF_T_DEFINED
typedef long off_t;
#define _OFF_T_DEFINED
#endif
#ifndef _INO_T_DEFINED
typedef unsigned long ino_t;
#define _INO_T_DEFINED
#endif
#ifndef _DEV_T_DEFINED
typedef unsigned long dev_t;
#define _DEV_T_DEFINED
#endif
#ifndef _NLINK_T_DEFINED
typedef unsigned long nlink_t;
#define _NLINK_T_DEFINED
#endif
#ifndef _BLKSIZE_T_DEFINED
typedef long blksize_t;
typedef long long blkcnt_t;
#define _BLKSIZE_T_DEFINED
#endif
typedef long long off64_t;
typedef unsigned long long ino64_t;
typedef int clockid_t;

// ============================================================
// signal.h POSIX additions (not provided by Windows SDK)
// ============================================================
#ifndef SIG_BLOCK
#define SIG_BLOCK 1
#define SIG_UNBLOCK 2
#define SIG_SETMASK 3
#endif
#ifndef SIGCHLD
#define SIGCHLD 17
#endif
#ifndef SIGUSR1
#define SIGUSR1 10
#define SIGUSR2 12
#endif
#ifndef SIGHUP
#define SIGHUP 1
#endif
#ifndef SIGKILL
#define SIGKILL 9
#endif
#ifndef SIGSTOP
#define SIGSTOP 17
#endif
#ifndef SIGCONT
#define SIGCONT 18
#endif
#ifndef SIGTERM
#define SIGTERM 15
#endif
#ifndef SIGQUIT
#define SIGQUIT 3
#endif
#ifndef SIGINT
#define SIGINT 2
#endif
#ifndef SA_SIGINFO
#define SA_SIGINFO 2
#endif
typedef void (*sighandler_t)(int);

// ============================================================
// POSIX signal type (Windows SDK lacks sigset_t)
// ============================================================
#ifndef HAVE_SIGSET_T
typedef unsigned long sigset_t;
#define HAVE_SIGSET_T
#endif

// ============================================================
// POSIX process API stubs for Windows
// ============================================================

// sigaction structure (sigaction() does not exist on Windows)
#ifndef HAVE_SIGACTION
struct sigaction {
    void (*sa_handler)(int);
    sigset_t sa_mask;
    int sa_flags;
};
#define HAVE_SIGACTION 1
#endif
#ifndef sigemptyset
#define sigemptyset(set) (*(set) = 0, 0)
#endif
#ifndef sigfillset
#define sigfillset(set) (*(set) = ~0, 0)
#endif
// We provide sigaction via signal() — the API won't match perfectly but is
// sufficient for compilation. Use SA_RESTART=0, SA_SIGINFO=2 etc.
static inline int srs_sigaction(int signum, const struct sigaction *act, struct sigaction *oldact) {
    if (act) signal(signum, act->sa_handler);
    return 0;
}
#ifndef sigaction
#define sigaction(signum, act, oldact) srs_sigaction((signum), (act), (oldact))
#endif

// flock structure and file locking (no-op stubs; Windows uses LockFileEx)
#ifndef HAVE_FLOCK_LINUX
struct flock {
    short l_type;
    short l_whence;
    off_t l_start;
    off_t l_len;
    pid_t l_pid;
};
#ifndef F_RDLCK
#define F_RDLCK 0
#define F_WRLCK 1
#define F_UNLCK 2
#endif
#ifndef F_SETLK
#define F_SETLK 6
#define F_SETLKW 7
#endif
#endif

// fcntl() — Windows SDK has _open/_fcntl but no POSIX fcntl().
// Provide a basic stub for F_GETFD, F_SETFD, F_SETLK operations.
#ifndef HAVE_FCNTL_STUB
static inline int srs_fcntl(int fd, int cmd, ...) {
    (void)fd;
    if (cmd == F_GETFD) return 0;
    if (cmd == F_SETFD) return 0;
    if (cmd == F_GETFL) return 0;
    if (cmd == F_SETFL) return 0;
    if (cmd == F_SETLK || cmd == F_SETLKW) return 0;
    errno = ENOSYS; return -1;
}
#define fcntl srs_fcntl
#endif

// pipe() — wrap _pipe
#ifndef HAVE_PIPE
static inline int srs_pipe(int fildes[2]) {
    return _pipe(fildes, 4096, _O_BINARY);
}
#ifndef pipe
#define pipe(fds) srs_pipe(fds)
#endif
#endif

// ftruncate() — wrap _chsize_s
#ifndef HAVE_FTRUNCATE
static inline int srs_ftruncate(int fd, off_t length) {
    return _chsize_s(fd, length);
}
#ifndef ftruncate
#define ftruncate(fd, length) srs_ftruncate((fd), (length))
#endif
#endif

// Process management: fork, kill, getppid, closefrom
// Windows does not support fork. We return -1 (error) for all.
// Note: waitpid is also handled by compat/sys/wait.h.
#ifndef HAVE_FORK
static inline int srs_fork(void) { errno = ENOSYS; return -1; }
#ifndef fork
#define fork() srs_fork()
#endif
static inline int srs_kill(pid_t pid, int sig) {
    (void)pid; (void)sig;
    errno = ENOSYS; return -1;
}
#ifndef kill
#define kill(pid, sig) srs_kill((pid), (sig))
#endif
// getppid also defined in compat/unistd.h, but force_include ensures availability.
static inline int srs_getppid(void) { return 0; }
#ifndef getppid
#define getppid() srs_getppid()
#endif
static inline void srs_closefrom(int lowfd) { (void)lowfd; }
#ifndef closefrom
#define closefrom(lowfd) srs_closefrom((lowfd))
#endif
#endif

// inotify stubs (Windows has no inotify)
#ifndef HAVE_INOTIFY
#ifndef IN_NONBLOCK
#define IN_NONBLOCK 0x800
#define IN_MODIFY 0x00000002
#define IN_CREATE 0x00000100
#define IN_MOVED_TO 0x00000080
#define IN_ACCESS 0x00000001
#define IN_ATTRIB 0x00000004
#define IN_CLOSE_WRITE 0x00000008
#define IN_DELETE 0x00000200
#define IN_DELETE_SELF 0x00000400
#define IN_CLOSE (IN_CLOSE_WRITE | 0x00000010)
#define IN_MOVE (IN_MOVED_TO | 0x00000040)
#endif
struct inotify_event {
    int wd;
    uint32_t mask;
    uint32_t cookie;
    uint32_t len;
    char name[];
};
static inline int srs_inotify_init1(int flags) { (void)flags; errno = ENOSYS; return -1; }
#ifndef inotify_init1
#define inotify_init1(flags) srs_inotify_init1((flags))
#endif
static inline int srs_inotify_add_watch(int fd, const char *pathname, uint32_t mask) {
    (void)fd; (void)pathname; (void)mask; errno = ENOSYS; return -1;
}
#ifndef inotify_add_watch
#define inotify_add_watch(fd, path, mask) srs_inotify_add_watch((fd), (path), (mask))
#endif
#endif

// Endian conversion (Windows is always little-endian)
#ifndef be32toh
#include <stdlib.h>
#define be32toh(x) _byteswap_ulong((x))
#define htobe32(x) _byteswap_ulong((x))
#define be16toh(x) _byteswap_ushort((x))
#define htobe16(x) _byteswap_ushort((x))
#endif

// ============================================================
// gmtime_r / localtime_r (thread-safe, Windows has _s versions)
// MSVC time_t is __int64; struct timeval.tv_sec is long.
// Provide overloads for both pointer types (C++ only).
// ============================================================
static inline struct tm *gmtime_r(const time_t *timep, struct tm *result) {
    errno_t ret = gmtime_s(result, timep);
    return ret == 0 ? result : NULL;
}
#ifdef __cplusplus
static inline struct tm *gmtime_r(const long *timep, struct tm *result) {
    time_t t = (time_t)*timep;
    return gmtime_r(&t, result);
}
#endif
static inline struct tm *localtime_r(const time_t *timep, struct tm *result) {
    errno_t ret = localtime_s(result, timep);
    return ret == 0 ? result : NULL;
}
#ifdef __cplusplus
static inline struct tm *localtime_r(const long *timep, struct tm *result) {
    time_t t = (time_t)*timep;
    return localtime_r(&t, result);
}
#endif

// ============================================================
// srandom / random (used by srs_kernel_utility.cpp)
// ============================================================
#include <stdlib.h>
static inline void srandom(unsigned int seed) {
    srand(seed);
}
static inline long random(void) {
    return rand();
}

// ============================================================
// bzero (BSD function, not standard on Windows; used by SRS)
// ============================================================
#ifndef bzero
#define bzero(buf, len) memset((buf), 0, (len))
#endif

// ============================================================
// u_int8_t (BSD-style, not standard on Windows; used by SRS)
// ============================================================
#ifndef u_int8_t
#define u_int8_t unsigned char
#endif

// ============================================================
// getsockopt / setsockopt POSIX wrappers (char* vs void* mismatch)
// ============================================================
static inline int srs_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen) {
    return getsockopt(s, level, optname, (char *)optval, optlen);
}
static inline int srs_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen) {
    return setsockopt(s, level, optname, (const char *)optval, optlen);
}
#ifndef getsockopt
#define getsockopt(s,l,n,v,lp) srs_getsockopt((s),(l),(n),(v),(lp))
#endif
#ifndef setsockopt
#define setsockopt(s,l,n,v,ln) srs_setsockopt((s),(l),(n),(v),(ln))
#endif

// asan callback stub (may be used by code guarded by SRS_SANITIZER_LOG)
// Always define to avoid unresolved symbol; harmless if real asan is absent.
static inline void __asan_set_error_report_callback(void (*cb)(const char*)) {}

// OpenSSL API compatibility shim (included after any SRS header includes openssl)
#include <openssl/compat.h>

#endif // _WIN32
