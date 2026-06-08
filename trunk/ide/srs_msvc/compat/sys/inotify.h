#ifdef _WIN32
/* Windows: stubs only if HAVE_INOTIFY is not defined (force_include.h handles this)
 * This header is included by files that explicitly #include <sys/inotify.h>.
 * If force_include.h already provided the stubs, we must not redefine them.
 * If force_include.h did NOT provide stubs (e.g. HAVE_INOTIFY is defined but
 * we're still on Windows, unlikely), stubs are needed here.
 */
#ifndef SRS_COMPAT_SYS_INOTIFY_H
#define SRS_COMPAT_SYS_INOTIFY_H
#include <windows.h>
#include <stdint.h>
#ifndef IN_NONBLOCK
#define IN_NONBLOCK 0x800
#endif
#ifndef IN_ACCESS
#define IN_ACCESS         0x00000001
#define IN_MODIFY         0x00000002
#define IN_ATTRIB         0x00000004
#define IN_CLOSE_WRITE    0x00000008
#define IN_CLOSE_NOWRITE  0x00000010
#define IN_OPEN           0x00000020
#define IN_MOVED_FROM     0x00000040
#define IN_MOVED_TO       0x00000080
#define IN_CREATE         0x00000100
#define IN_DELETE         0x00000200
#define IN_DELETE_SELF    0x00000400
#define IN_MOVE_SELF      0x00000800
#define IN_ALL_EVENTS     (IN_ACCESS | IN_MODIFY | IN_ATTRIB | IN_CLOSE_WRITE | IN_CLOSE_NOWRITE | IN_OPEN | IN_MOVED_FROM | IN_MOVED_TO | IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF)
#define IN_ONLYDIR        0x01000000
#define IN_DONT_FOLLOW    0x02000000
#define IN_EXCL_UNLINK    0x04000000
#define IN_MASK_ADD       0x20000000
#define IN_ISDIR          0x40000000
#define IN_ONESHOT        0x80000000
#endif
#ifndef IN_CLOSE
#define IN_CLOSE (IN_CLOSE_WRITE | 0x00000010)
#endif
#ifndef IN_MOVE
#define IN_MOVE (IN_MOVED_TO | 0x00000040)
#endif
/* Define struct only if not already provided by force_include.h */
#ifndef HAVE_INOTIFY
#ifndef inotify_init1
#pragma warning(push)
#pragma warning(disable:4100)
static inline int srs_inotify_init1(int flags) { flags = 0; errno = ENOSYS; return -1; }
#pragma warning(pop)
#define inotify_init1(flags) srs_inotify_init1((flags))
#endif
#ifndef inotify_add_watch
#pragma warning(push)
#pragma warning(disable:4100)
static inline int srs_inotify_add_watch(int fd, const char *pathname, uint32_t mask) { fd = 0; pathname = NULL; mask = 0; errno = ENOSYS; return -1; }
#pragma warning(pop)
#define inotify_add_watch(fd, path, mask) srs_inotify_add_watch((fd), (path), (mask))
#endif
#endif
#endif
#else
#include_next <sys/inotify.h>
#endif
