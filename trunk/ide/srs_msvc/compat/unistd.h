#ifdef _WIN32
#ifndef SRS_COMPAT_UNISTD_H
#define SRS_COMPAT_UNISTD_H
#include <io.h>
#include <process.h>
#include <direct.h>
#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#ifndef __GNUC__
#define getpid _getpid
#define getcwd _getcwd
#define chdir _chdir
#endif
static inline int srs_gethostname(char *name, size_t len) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) return -1;
    int r = gethostname(name, (int)len);
    WSACleanup();
    return r;
}
#define gethostname(s,n) srs_gethostname((s),(n))
#define usleep(us) Sleep((DWORD)((us)/1000))
#define sleep(s) Sleep((DWORD)(s)*1000)
// Note: Do NOT define dup/dup2/lseek/close/read/write as macros.
// They break C++ member function calls like obj->close(), obj->lseek(), etc.
// MSVC provides POSIX names (::close, ::lseek, ::dup, ::dup2, ::read, ::write)
// when _CRT_NONSTDC_NO_DEPRECATE is defined (set in force_include.h).
#define isatty _isatty
#define link _link
#define symlink _symlink
#define readlink _readlink
#define getlogin _getlogin
#define getuid() 0
#define getgid() 0
#define geteuid() 0
#define getegid() 0
#ifndef getppid
#define getppid() 0
#endif
#endif
#else
#include_next <unistd.h>
#endif
