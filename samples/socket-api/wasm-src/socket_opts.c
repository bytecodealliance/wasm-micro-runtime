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

struct timeval
to_timeval(time_t tv_sec, suseconds_t tv_usec)
{
    struct timeval tv = { tv_sec, tv_usec };
    return tv;
}

int
main(int argc, char *argv[])
{
    int socket_fd = 0;
    struct timeval tv;
    socklen_t tv_len = sizeof(tv);

    printf("[Client] Create socket\n");
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("Create socket failed");
        return EXIT_FAILURE;
    }

    tv = to_timeval(123, 1000);
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    tv = to_timeval(0, 0);
    getsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, &tv_len);
    OPTION_ASSERT(tv.tv_sec, 123, "SO_RCVTIMEO tv_sec");
    OPTION_ASSERT(tv.tv_usec, 1000, "SO_RCVTIMEO tv_usec");

    tv = to_timeval(456, 2000);
    setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    tv = to_timeval(0, 0);
    getsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, &tv_len);
    OPTION_ASSERT(tv.tv_sec, 456, "SO_SNDTIMEO tv_sec");
    OPTION_ASSERT(tv.tv_usec, 2000, "SO_SNDTIMEO tv_usec");

    printf("[Client] Close socket\n");
    close(socket_fd);
    return EXIT_SUCCESS;
}
