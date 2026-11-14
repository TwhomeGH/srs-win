// st-srs regression test: coroutine context switching, timer scheduling, and
// the select() dispatch path that the SRS listeners depend on.
//
// This test exists because three Windows-only defects (64-bit stack pointer
// stored through a 32-bit index, the idle coroutine's 64KB stack overflowing
// on three fd_set copies, and an ignored failed read spinning the scheduler)
// all shipped without any test that exercised the event loop.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET test_socket_t;
#define TEST_INVALID_SOCKET INVALID_SOCKET
static int test_close_socket(test_socket_t s) { return closesocket(s); }
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int test_socket_t;
#define TEST_INVALID_SOCKET (-1)
static int test_close_socket(test_socket_t s) { return close(s); }
#endif

#include <st.h>

static int failures = 0;

#define CHECK(cond, message)                                                        \
    do {                                                                            \
        if (!(cond)) {                                                              \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, message);       \
            failures++;                                                             \
        }                                                                           \
    } while (0)

#define TEST_TIMEOUT (10 * 1000 * 1000) /* 10s, in st microseconds */

// ---------------------------------------------------------------------------
// 1. Context switching: locals and arguments must survive many yields.
// ---------------------------------------------------------------------------
struct worker_args {
    int id;
    int loops;
    int observed;
};

static void *worker_main(void *opaque)
{
    struct worker_args *args = (struct worker_args *)opaque;
    int local = args->id * 1000;

    for (int i = 0; i < args->loops; i++) {
        local++;
        // Exercise the timer/scheduler path, not just an immediate yield.
        st_usleep(1000);
    }

    args->observed = local;
    return NULL;
}

static void test_context_switch(void)
{
    enum { WORKERS = 8, LOOPS = 100 };
    struct worker_args args[WORKERS];
    st_thread_t threads[WORKERS];

    for (int i = 0; i < WORKERS; i++) {
        args[i].id = i;
        args[i].loops = LOOPS;
        args[i].observed = 0;
        threads[i] = st_thread_create(worker_main, &args[i], 1, 0);
        CHECK(threads[i] != NULL, "st_thread_create failed");
    }

    for (int i = 0; i < WORKERS; i++) {
        if (threads[i]) {
            st_thread_join(threads[i], NULL);
        }
    }

    for (int i = 0; i < WORKERS; i++) {
        CHECK(args[i].observed == args[i].id * 1000 + LOOPS, "coroutine local was corrupted across switches");
    }

    printf("context switch: %d coroutines x %d yields\n", WORKERS, LOOPS);
}

// ---------------------------------------------------------------------------
// 2. select() dispatch: accept/read/write over a loopback TCP connection.
// ---------------------------------------------------------------------------
static test_socket_t make_listener(int *port)
{
    test_socket_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == TEST_INVALID_SOCKET) {
        return s;
    }

    int yes = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) != 0 || listen(s, 64) != 0) {
        test_close_socket(s);
        return TEST_INVALID_SOCKET;
    }

    int len = sizeof(addr);
    if (getsockname(s, (struct sockaddr *)&addr, &len) != 0) {
        test_close_socket(s);
        return TEST_INVALID_SOCKET;
    }

    *port = ntohs(addr.sin_port);
    return s;
}

static test_socket_t connect_to(int port)
{
    test_socket_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == TEST_INVALID_SOCKET) {
        return s;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons((unsigned short)port);

    if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        test_close_socket(s);
        return TEST_INVALID_SOCKET;
    }

    return s;
}

struct echo_args {
    int listen_fd;
    int received;
    int clients;
};

static void *echo_server(void *opaque)
{
    struct echo_args *args = (struct echo_args *)opaque;
    st_netfd_t lfd = st_netfd_open_socket(args->listen_fd);
    if (!lfd) {
        fprintf(stderr, "server: st_netfd_open_socket failed, errno=%d\n", errno);
        args->received = -1;
        return NULL;
    }

    for (int i = 0; i < args->clients; i++) {
        st_netfd_t cfd = st_accept(lfd, NULL, NULL, TEST_TIMEOUT);
        if (!cfd) {
            fprintf(stderr, "server: st_accept failed at %d/%d, errno=%d\n", i, args->clients, errno);
            args->received = -1;
            break;
        }

        char buf[8];
        int n = (int)st_read(cfd, buf, 1, TEST_TIMEOUT);
        if (n == 1) {
            args->received++;
            int w = (int)st_write(cfd, "pong", 4, TEST_TIMEOUT);
            if (w != 4) {
                fprintf(stderr, "server: st_write failed, w=%d errno=%d\n", w, errno);
            }
        } else {
            fprintf(stderr, "server: st_read failed at %d/%d, n=%d errno=%d\n", i, args->clients, n, errno);
        }
        st_netfd_close(cfd);
    }

    st_netfd_close(lfd);
    return NULL;
}

struct client_args {
    int port;
    int ok;
};

static void *echo_client(void *opaque)
{
    struct client_args *args = (struct client_args *)opaque;
    test_socket_t s = connect_to(args->port);
    if (s == TEST_INVALID_SOCKET) {
        args->ok = 0;
        return NULL;
    }

    st_netfd_t cfd = st_netfd_open_socket((int)s);
    if (!cfd) {
        test_close_socket(s);
        args->ok = 0;
        return NULL;
    }

    char reply[4];
    int sent = (int)st_write(cfd, "p", 1, TEST_TIMEOUT);
    int got = (int)st_read(cfd, reply, 4, TEST_TIMEOUT);
    args->ok = (sent == 1 && got == 4 && memcmp(reply, "pong", 4) == 0);
    if (!args->ok) {
        fprintf(stderr, "client: sent=%d got=%d reply=%.4s errno=%d\n", sent, got, reply, errno);
    }

    st_netfd_close(cfd);
    return NULL;
}

static void test_select_roundtrip(void)
{
    enum { CLIENTS = 32 };
    int port = 0;
    test_socket_t listener = make_listener(&port);
    CHECK(listener != TEST_INVALID_SOCKET, "failed to create listener");
    if (listener == TEST_INVALID_SOCKET) {
        return;
    }

    struct echo_args server_args;
    server_args.listen_fd = (int)listener;
    server_args.received = 0;
    server_args.clients = CLIENTS;

    st_thread_t server = st_thread_create(echo_server, &server_args, 1, 0);
    CHECK(server != NULL, "failed to create echo server coroutine");
    if (!server) {
        test_close_socket(listener);
        return;
    }
    st_usleep(1000);

    struct client_args clients[CLIENTS];
    st_thread_t client_threads[CLIENTS];
    for (int i = 0; i < CLIENTS; i++) {
        clients[i].port = port;
        clients[i].ok = 0;
        client_threads[i] = st_thread_create(echo_client, &clients[i], 1, 0);
        CHECK(client_threads[i] != NULL, "failed to create client coroutine");
    }

    for (int i = 0; i < CLIENTS; i++) {
        if (client_threads[i]) {
            st_thread_join(client_threads[i], NULL);
        }
    }
    st_thread_join(server, NULL);

    CHECK(server_args.received == CLIENTS, "server did not accept every connection");
    for (int i = 0; i < CLIENTS; i++) {
        CHECK(clients[i].ok, "client round-trip failed");
    }

    printf("select round-trip: %d loopback connections\n", CLIENTS);
}

int main(void)
{
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }
    st_set_eventsys(ST_EVENTSYS_SELECT);
#endif

    if (st_init() != 0) {
        fprintf(stderr, "st_init failed\n");
        return 1;
    }

    test_context_switch();
    test_select_roundtrip();

    st_destroy();

#ifdef _WIN32
    WSACleanup();
#endif

    if (failures) {
        fprintf(stderr, "%d check(s) failed\n", failures);
        return 1;
    }

    printf("st event loop: all checks passed\n");
    return 0;
}
