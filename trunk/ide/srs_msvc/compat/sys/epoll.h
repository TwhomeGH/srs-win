#ifdef _WIN32
#ifndef SRS_COMPAT_SYS_EPOLL_H
#define SRS_COMPAT_SYS_EPOLL_H
#include <winsock2.h>
#define EPOLLIN  (POLLIN)
#define EPOLLOUT (POLLOUT)
#define EPOLLERR (POLLERR)
#define EPOLLHUP (POLLHUP)
#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_DEL 2
#define EPOLL_CTL_MOD 3
typedef union epoll_data {
    void *ptr;
    int fd;
    uint32_t u32;
    uint64_t u64;
} epoll_data_t;
struct epoll_event {
    uint32_t events;
    epoll_data_t data;
};
static inline int epoll_create(int size) {
    (void)size;
    return -1;
}
static inline int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event) {
    (void)epfd; (void)op; (void)fd; (void)event;
    return -1;
}
static inline int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout) {
    (void)epfd; (void)events; (void)maxevents; (void)timeout;
    return -1;
}
#endif
#else
#include_next <sys/epoll.h>
#endif
