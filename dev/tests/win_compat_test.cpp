//
// Regression tests for Windows POSIX compatibility stubs.
//
// A stub that returns success without doing the work is worse than an explicit
// failure: callers cannot detect that reload, locking, or event notification
// is unavailable. This test pins the fail-loud contract.
//

#include <cerrno>
#include <cstdio>

#ifdef _WIN32
#include <sys/epoll.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

static int check_fcntl_locks(void)
{
    struct flock lock = {};
    errno = 0;
    if (srs_fcntl(-1, F_SETLK, &lock) != -1 || errno != ENOSYS) {
        fprintf(stderr, "fcntl(F_SETLK) should fail with ENOSYS, errno=%d\n", errno);
        return 0;
    }

    errno = 0;
    if (srs_fcntl(-1, F_SETLKW, &lock) != -1 || errno != ENOSYS) {
        fprintf(stderr, "fcntl(F_SETLKW) should fail with ENOSYS, errno=%d\n", errno);
        return 0;
    }

    errno = 0;
    if (srs_fcntl(-1, F_GETFD) != 0) {
        fprintf(stderr, "fcntl(F_GETFD) should remain a safe no-op\n");
        return 0;
    }

    return 1;
}

static int check_epoll(void)
{
    struct epoll_event event = {};
    errno = 0;
    if (epoll_create(1) != -1 || errno != ENOSYS) {
        fprintf(stderr, "epoll_create should fail with ENOSYS, errno=%d\n", errno);
        return 0;
    }

    errno = 0;
    if (epoll_ctl(-1, EPOLL_CTL_ADD, -1, &event) != -1 || errno != ENOSYS) {
        fprintf(stderr, "epoll_ctl should fail with ENOSYS, errno=%d\n", errno);
        return 0;
    }

    errno = 0;
    if (epoll_wait(-1, &event, 1, 0) != -1 || errno != ENOSYS) {
        fprintf(stderr, "epoll_wait should fail with ENOSYS, errno=%d\n", errno);
        return 0;
    }

    return 1;
}

static int check_inotify(void)
{
    errno = 0;
    if (srs_inotify_init1(0) != -1 || errno != ENOSYS) {
        fprintf(stderr, "inotify_init1 should fail with ENOSYS, errno=%d\n", errno);
        return 0;
    }

    errno = 0;
    if (srs_inotify_add_watch(-1, "x", 0) != -1 || errno != ENOSYS) {
        fprintf(stderr, "inotify_add_watch should fail with ENOSYS, errno=%d\n", errno);
        return 0;
    }

    return 1;
}

static int check_noop_rejections(void)
{
    struct itimerval timer = {};
    errno = 0;
    if (setitimer(ITIMER_REAL, &timer, NULL) != -1 || errno != ENOSYS) {
        fprintf(stderr, "setitimer should fail with ENOSYS, errno=%d\n", errno);
        return 0;
    }

    errno = 0;
    if (getitimer(ITIMER_REAL, &timer) != -1 || errno != ENOSYS) {
        fprintf(stderr, "getitimer should fail with ENOSYS, errno=%d\n", errno);
        return 0;
    }

    struct rlimit limit = {};
    errno = 0;
    if (getrlimit(RLIMIT_STACK, &limit) != -1 || errno != ENOSYS) {
        fprintf(stderr, "getrlimit should fail with ENOSYS, errno=%d\n", errno);
        return 0;
    }

    errno = 0;
    if (setrlimit(RLIMIT_STACK, &limit) != -1 || errno != ENOSYS) {
        fprintf(stderr, "setrlimit should fail with ENOSYS, errno=%d\n", errno);
        return 0;
    }

    int status = 0;
    errno = 0;
    if (wait(&status) != -1 || errno != ENOSYS) {
        fprintf(stderr, "wait should fail with ENOSYS, errno=%d\n", errno);
        return 0;
    }

    errno = 0;
    if (waitpid(-1, &status, WNOHANG) != -1 || errno != ENOSYS) {
        fprintf(stderr, "waitpid(-1) should fail with ENOSYS, errno=%d\n", errno);
        return 0;
    }

    return 1;
}

int main()
{
    int ok = check_fcntl_locks() && check_epoll() && check_inotify() && check_noop_rejections();
    if (!ok) {
        return 1;
    }

    printf("win compat stubs: PASS\n");
    return 0;
}
#else
int main()
{
    return 0;
}
#endif
