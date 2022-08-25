#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#ifdef __wasi__
#include <wasi_socket_ext.h>
#endif

#define OPTION_ASSERT(A, B, OPTION)           \
    if (A == B) {                             \
        printf("%s is expected\n", OPTION);   \
    }                                         \
    else {                                    \
        printf("%s is unexpected\n", OPTION); \
        return EXIT_FAILURE;                  \
    }

int
main(int argc, char *argv[])
{
    int socket_fd = 0;

    printf("[Client] Create socket\n");
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("Create socket failed");
        return EXIT_FAILURE;
    }

// TODO: Replace this with posix sockopts when available
#ifdef __wasi__
    uint64_t timeout_us = 5000;
    __wasi_sock_set_recv_timeout(socket_fd, &timeout_us);
    __wasi_sock_get_recv_timeout(socket_fd, &timeout_us);
    OPTION_ASSERT(timeout_us, 5000, "recv_timeout");

    timeout_us = 10000;
    __wasi_sock_set_send_timeout(socket_fd, &timeout_us);
    __wasi_sock_get_send_timeout(socket_fd, &timeout_us);
    OPTION_ASSERT(timeout_us, 10000, "send_timeout");
#endif
    printf("[Client] Close socket\n");
    close(socket_fd);
    return EXIT_SUCCESS;
}
