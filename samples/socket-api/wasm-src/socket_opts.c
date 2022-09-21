#include <arpa/inet.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#ifdef __wasi__
#include <wasi_socket_ext.h>
#endif

#define MULTICAST_ADDR 16777440
#define OPTION_ASSERT(A, B, OPTION)           \
    if (A == B) {                             \
        printf("%s is expected\n", OPTION);   \
    }                                         \
    else {                                    \
        printf("%s is unexpected\n", OPTION); \
        perror("assertion failed");           \
        return EXIT_FAILURE;                  \
    }

int
guard(int n, char *err)
{
    if (n == -1) {
        perror(err);
        exit(1);
    }
    return n;
}

struct timeval
to_timeval(time_t tv_sec, suseconds_t tv_usec)
{
    struct timeval tv = { tv_sec, tv_usec };
    return tv;
}

int
set_and_get_bool_opt(int socket_fd, int level, int optname, int val)
{
    int bool_opt = val;
    int ret = -1;
    socklen_t opt_len = sizeof(bool_opt);

    ret = setsockopt(socket_fd, level, optname, &bool_opt, sizeof(bool_opt));
    if (ret != 0)
        return !val;

    bool_opt = !bool_opt;
    ret = getsockopt(socket_fd, level, optname, &bool_opt, &opt_len);
    if (ret != 0)
        return !val;

    return bool_opt;
}

int
test_set_and_get_opts()
{
    int tcp_socket_fd = 0;
    int udp_socket_fd = 0;
    int udp_ipv6_socket_fd = 0;
    struct timeval tv;
    socklen_t opt_len;
    int buf_len;
    int result;
    struct linger linger_opt;
    uint32_t time_s;
    struct ip_mreq mcast;
    struct ipv6_mreq mcast_ipv6;
    unsigned char ttl;

    printf("[Client] Create TCP socket\n");
    tcp_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket_fd == -1) {
        perror("Create socket failed");
        return EXIT_FAILURE;
    }

    printf("[Client] Create UDP socket\n");
    udp_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket_fd == -1) {
        perror("Create socket failed");
        return EXIT_FAILURE;
    }

    printf("[Client] Create UDP IPv6 socket\n");
    udp_ipv6_socket_fd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (udp_ipv6_socket_fd == -1) {
        perror("Create socket failed");
        return EXIT_FAILURE;
    }

    // SO_RCVTIMEO
    tv = to_timeval(123, 1000);
    result =
        setsockopt(tcp_socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    OPTION_ASSERT(result, 0, "setsockopt SO_RCVTIMEO result")

    tv = to_timeval(0, 0);
    opt_len = sizeof(tv);
    result = getsockopt(tcp_socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, &opt_len);
    OPTION_ASSERT(result, 0, "getsockopt SO_RCVTIMEO result")
    OPTION_ASSERT(tv.tv_sec, 123, "SO_RCVTIMEO tv_sec");
    // OPTION_ASSERT(tv.tv_usec, 1000, "SO_RCVTIMEO tv_usec");

    // SO_SNDTIMEO
    tv = to_timeval(456, 2000);
    result =
        setsockopt(tcp_socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    OPTION_ASSERT(result, 0, "setsockopt SO_SNDTIMEO result")

    tv = to_timeval(0, 0);
    opt_len = sizeof(tv);
    result = getsockopt(tcp_socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, &opt_len);
    OPTION_ASSERT(result, 0, "getsockopt SO_SNDTIMEO result")
    OPTION_ASSERT(tv.tv_sec, 456, "SO_SNDTIMEO tv_sec");
    // OPTION_ASSERT(tv.tv_usec, 2000, "SO_SNDTIMEO tv_usec");

    // SO_SNDBUF
    buf_len = 8192;
    result = setsockopt(tcp_socket_fd, SOL_SOCKET, SO_SNDBUF, &buf_len,
                        sizeof(buf_len));
    OPTION_ASSERT(result, 0, "setsockopt SO_SNDBUF result")

    buf_len = 0;
    opt_len = sizeof(buf_len);
    result =
        getsockopt(tcp_socket_fd, SOL_SOCKET, SO_SNDBUF, &buf_len, &opt_len);
    OPTION_ASSERT(result, 0, "getsockopt SO_SNDBUF result")
    OPTION_ASSERT(buf_len, 16384, "SO_SNDBUF buf_len");

    // SO_RCVBUF
    buf_len = 4096;
    result = setsockopt(tcp_socket_fd, SOL_SOCKET, SO_RCVBUF, &buf_len,
                        sizeof(buf_len));
    OPTION_ASSERT(result, 0, "setsockopt SO_RCVBUF result")

    buf_len = 0;
    opt_len = sizeof(buf_len);
    result =
        getsockopt(tcp_socket_fd, SOL_SOCKET, SO_RCVBUF, &buf_len, &opt_len);
    OPTION_ASSERT(result, 0, "getsockopt SO_RCVBUF result")
    OPTION_ASSERT(buf_len, 8192, "SO_RCVBUF buf_len");

    // SO_KEEPALIVE
    OPTION_ASSERT(
        set_and_get_bool_opt(tcp_socket_fd, SOL_SOCKET, SO_KEEPALIVE, 1), 1,
        "SO_KEEPALIVE enabled");
    OPTION_ASSERT(
        set_and_get_bool_opt(tcp_socket_fd, SOL_SOCKET, SO_KEEPALIVE, 0), 0,
        "SO_KEEPALIVE disabled");

    // SO_REUSEADDR
    OPTION_ASSERT(
        set_and_get_bool_opt(tcp_socket_fd, SOL_SOCKET, SO_REUSEADDR, 1), 1,
        "SO_REUSEADDR enabled");
    OPTION_ASSERT(
        set_and_get_bool_opt(tcp_socket_fd, SOL_SOCKET, SO_REUSEADDR, 0), 0,
        "SO_REUSEADDR disabled");

    // SO_REUSEPORT
    OPTION_ASSERT(
        set_and_get_bool_opt(tcp_socket_fd, SOL_SOCKET, SO_REUSEPORT, 1), 1,
        "SO_REUSEPORT enabled");
    OPTION_ASSERT(
        set_and_get_bool_opt(tcp_socket_fd, SOL_SOCKET, SO_REUSEPORT, 0), 0,
        "SO_REUSEPORT disabled");

    // SO_LINGER
    linger_opt.l_onoff = 1;
    linger_opt.l_linger = 10;
    result = setsockopt(tcp_socket_fd, SOL_SOCKET, SO_LINGER, &linger_opt,
                        sizeof(linger_opt));
    OPTION_ASSERT(result, 0, "setsockopt SO_LINGER result")

    linger_opt.l_onoff = 0;
    linger_opt.l_linger = 0;
    opt_len = sizeof(linger_opt);
    result =
        getsockopt(tcp_socket_fd, SOL_SOCKET, SO_LINGER, &linger_opt, &opt_len);
    OPTION_ASSERT(result, 0, "getsockopt SO_LINGER result")
    OPTION_ASSERT(linger_opt.l_onoff, 1, "SO_LINGER l_onoff");
    OPTION_ASSERT(linger_opt.l_linger, 10, "SO_LINGER l_linger");

    // SO_BROADCAST
    OPTION_ASSERT(
        set_and_get_bool_opt(udp_socket_fd, SOL_SOCKET, SO_BROADCAST, 1), 1,
        "SO_BROADCAST enabled");
    OPTION_ASSERT(
        set_and_get_bool_opt(udp_socket_fd, SOL_SOCKET, SO_BROADCAST, 0), 0,
        "SO_BROADCAST disabled");

    // TCP_KEEPIDLE
    time_s = 16;
    result = setsockopt(tcp_socket_fd, IPPROTO_TCP, TCP_KEEPIDLE, &time_s,
                        sizeof(time_s));
    OPTION_ASSERT(result, 0, "setsockopt TCP_KEEPIDLE result")

    time_s = 0;
    opt_len = sizeof(time_s);
    result =
        getsockopt(tcp_socket_fd, IPPROTO_TCP, TCP_KEEPIDLE, &time_s, &opt_len);
    OPTION_ASSERT(result, 0, "getsockopt TCP_KEEPIDLE result")
    OPTION_ASSERT(time_s, 16, "TCP_KEEPIDLE");

    // TCP_KEEPINTVL
    time_s = 8;
    result = setsockopt(tcp_socket_fd, IPPROTO_TCP, TCP_KEEPINTVL, &time_s,
                        sizeof(time_s));
    OPTION_ASSERT(result, 0, "setsockopt TCP_KEEPINTVL result")

    time_s = 0;
    opt_len = sizeof(time_s);
    result = getsockopt(tcp_socket_fd, IPPROTO_TCP, TCP_KEEPINTVL, &time_s,
                        &opt_len);
    OPTION_ASSERT(result, 0, "getsockopt TCP_KEEPINTVL result")
    OPTION_ASSERT(time_s, 8, "TCP_KEEPINTVL");

    // TCP_FASTOPEN_CONNECT
    OPTION_ASSERT(set_and_get_bool_opt(tcp_socket_fd, IPPROTO_TCP,
                                       TCP_FASTOPEN_CONNECT, 1),
                  1, "TCP_FASTOPEN_CONNECT enabled");
    OPTION_ASSERT(set_and_get_bool_opt(tcp_socket_fd, IPPROTO_TCP,
                                       TCP_FASTOPEN_CONNECT, 0),
                  0, "TCP_FASTOPEN_CONNECT disabled");

    // TCP_NODELAY
    OPTION_ASSERT(
        set_and_get_bool_opt(tcp_socket_fd, IPPROTO_TCP, TCP_NODELAY, 1), 1,
        "TCP_NODELAY enabled");
    OPTION_ASSERT(
        set_and_get_bool_opt(tcp_socket_fd, IPPROTO_TCP, TCP_NODELAY, 0), 0,
        "TCP_NODELAY disabled");

    // TCP_QUICKACK
    OPTION_ASSERT(
        set_and_get_bool_opt(tcp_socket_fd, IPPROTO_TCP, TCP_QUICKACK, 1), 1,
        "TCP_QUICKACK enabled");
    OPTION_ASSERT(
        set_and_get_bool_opt(tcp_socket_fd, IPPROTO_TCP, TCP_QUICKACK, 0), 0,
        "TCP_QUICKACK disabled");

    // IP_TTL
    ttl = 8;
    result = setsockopt(tcp_socket_fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
    OPTION_ASSERT(result, 0, "IP_TIL");
    ttl = 0;
    opt_len = sizeof(ttl);
    result = getsockopt(tcp_socket_fd, IPPROTO_IP, IP_TTL, &ttl, &opt_len);
    OPTION_ASSERT(ttl, 8, "IP_TTL");
    OPTION_ASSERT(result, 0, "IP_TIL");

    // IPV6_V6ONLY
    OPTION_ASSERT(
        set_and_get_bool_opt(udp_ipv6_socket_fd, IPPROTO_IPV6, IPV6_V6ONLY, 1),
        1, "IPV6_V6ONLY enabled");
    OPTION_ASSERT(
        set_and_get_bool_opt(udp_ipv6_socket_fd, IPPROTO_IPV6, IPV6_V6ONLY, 0),
        0, "IPV6_V6ONLY disabled");

    // IP_MULTICAST_LOOP
    OPTION_ASSERT(
        set_and_get_bool_opt(udp_socket_fd, IPPROTO_IP, IP_MULTICAST_LOOP, 1),
        1, "IP_MULTICAST_LOOP enabled");
    OPTION_ASSERT(
        set_and_get_bool_opt(udp_socket_fd, IPPROTO_IP, IP_MULTICAST_LOOP, 0),
        0, "IP_MULTICAST_LOOP disabled");

    // IP_ADD_MEMBERSHIP
    mcast.imr_multiaddr.s_addr = MULTICAST_ADDR;
    mcast.imr_interface.s_addr = htonl(INADDR_ANY);
    result = setsockopt(udp_socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mcast,
                        sizeof(mcast));
    OPTION_ASSERT(result, 0, "IP_ADD_MEMBERSHIP");

    // IP_DROP_MEMBERSHIP
    result = setsockopt(udp_socket_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mcast,
                        sizeof(mcast));
    OPTION_ASSERT(result, 0, "IP_DROP_MEMBERSHIP");

    // IP_MULTICAST_TTL
    ttl = 8;
    result = setsockopt(udp_socket_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl,
                        sizeof(ttl));
    OPTION_ASSERT(result, 0, "IP_MULTICAST_TTL");
    ttl = 0;
    opt_len = sizeof(ttl);
    result =
        getsockopt(udp_socket_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, &opt_len);
    OPTION_ASSERT(ttl, 8, "IP_MULTICAST_TTL");
    OPTION_ASSERT(result, 0, "IP_MULTICAST_TTL");

    // IPV6_MULTICAST_LOOP
    OPTION_ASSERT(set_and_get_bool_opt(udp_ipv6_socket_fd, IPPROTO_IPV6,
                                       IPV6_MULTICAST_LOOP, 1),
                  1, "IPV6_MULTICAST_LOOP enabled");
    OPTION_ASSERT(set_and_get_bool_opt(udp_ipv6_socket_fd, IPPROTO_IPV6,
                                       IPV6_MULTICAST_LOOP, 0),
                  0, "IPV6_MULTICAST_LOOP disabled");

    // IPV6_JOIN_GROUP
    result = setsockopt(udp_ipv6_socket_fd, IPPROTO_IPV6, IPV6_JOIN_GROUP,
                        &mcast_ipv6, sizeof(mcast_ipv6));
    // OPTION_ASSERT(result, 0, "IPV6_JOIN_GROUP");

    // IPV6_LEAVE_GROUP
    result = setsockopt(udp_ipv6_socket_fd, IPPROTO_IPV6, IPV6_LEAVE_GROUP,
                        &mcast_ipv6, sizeof(mcast_ipv6));
    // OPTION_ASSERT(result, 0, "IPV6_LEAVE_GROUP");

    printf("[Client] Close sockets\n");
    close(tcp_socket_fd);
    close(udp_socket_fd);
    return EXIT_SUCCESS;
}

int
test_send_and_recv_timeout_client()
{
    int socket_fd;
    struct sockaddr_in addr;
    struct timeval tv = to_timeval(0, 1);
    const int snd_buf_len = 8;
    const int data_buf_len = 1000000;
    char *buffer = (char *)malloc(sizeof(char) * data_buf_len);
    int result;
    socklen_t opt_len = sizeof(snd_buf_len);
    struct timeval snd_start_time, snd_end_time;

    /* 127.0.0.1:1234 */
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socket_fd = guard(socket(AF_INET, SOCK_STREAM, 0), "Create socket failed");
    guard(set_and_get_bool_opt(socket_fd, SOL_SOCKET, SO_REUSEADDR, 1),
          "Failed to set REUSEADDR");
    guard(setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)),
          "Failed to set SO_RCVTIMEO");
    guard(setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)),
          "Failed to set SO_SNDTIMEO");
    guard(setsockopt(socket_fd, SOL_SOCKET, SO_SNDBUF, &data_buf_len,
                     sizeof(data_buf_len)),
          "Failed to set buffer length");
    guard(connect(socket_fd, (struct sockaddr *)&addr, sizeof(addr)),
          "Connect failed");
    getsockopt(socket_fd, SOL_SOCKET, SO_SNDBUF, (void *)&data_buf_len,
               &opt_len);

    printf("Waiting on recv, which should timeout\n");
    result = recv(socket_fd, buffer, 1, 0);
    OPTION_ASSERT(result, -1, "recv timeout");
    OPTION_ASSERT(errno, EAGAIN, "errno EAGAIN");

    printf("Waiting on send, which should timeout\n");
    gettimeofday(&snd_start_time, NULL);
    result = send(socket_fd, buffer, data_buf_len, 0);
    gettimeofday(&snd_end_time, NULL);

    OPTION_ASSERT((result < data_buf_len), 1,
                  "expect partial send transmission");
    OPTION_ASSERT((snd_start_time.tv_sec == snd_end_time.tv_sec), 1,
                  "expected quick send return");

    close(socket_fd);
    printf("Closing socket \n");
    return EXIT_SUCCESS;
}

int
test_send_and_recv_timeout_server()
{
    int socket_fd;
    int client_socket_fd;
    struct sockaddr_in addr = { 0 };
    int addrlen = sizeof(addr);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    socket_fd = guard(socket(AF_INET, SOCK_STREAM, 0), "Create socket failed");
    guard(set_and_get_bool_opt(socket_fd, SOL_SOCKET, SO_REUSEADDR, 1),
          "Failed to set REUSEADDR");
    guard(bind(socket_fd, (struct sockaddr *)&addr, addrlen),
          "Bind socket failed");
    guard(listen(socket_fd, 1), "Listen failed");

    printf("Wait for client to connect\n");
    client_socket_fd = guard(
        accept(socket_fd, (struct sockaddr *)&addr, (socklen_t *)&addrlen),
        "Accept failed");

    printf("Client connected, sleeping for 10s\n");
    sleep(10);

    printf("Shuting down\n");
    shutdown(client_socket_fd, SHUT_RDWR);
    shutdown(socket_fd, SHUT_RDWR);
    return EXIT_SUCCESS;
}

int 
test_multicast_client() {
    struct sockaddr_in localSock;
    struct ip_mreq group;
    int sd;
    int datalen;
    char databuf[1024];
    sd = guard(socket(AF_INET, SOCK_DGRAM, 0), "Failed opening socket");
    guard(set_and_get_bool_opt(sd, SOL_SOCKET, SO_REUSEADDR, 1), "Failed setting SO_REUSEADDR");

    localSock.sin_family = AF_INET;
    localSock.sin_port = htons(4321);
    localSock.sin_addr.s_addr = INADDR_ANY;

    guard(bind(sd, (struct sockaddr*)&localSock, sizeof(localSock)), "Failed binding socket");

    group.imr_multiaddr.s_addr = MULTICAST_ADDR;
    group.imr_interface.s_addr = htonl(INADDR_ANY);

    guard(setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)), "Failed adding multicast group");

    datalen = sizeof(databuf);
    int result = read(sd, databuf, datalen);

    OPTION_ASSERT((result < 0), 0, "read response");

    printf("Reading datagram message...OK.\n");
    printf("The message from multicast server is: \"%s\"\n", databuf);

    return EXIT_SUCCESS;
}

int 
test_multicast_server() {
    struct in_addr localInterface;
    struct sockaddr_in groupSock;
    int sd;
    char * databuf = "Test message";
    int datalen = strlen(databuf);
    sd = guard(socket(AF_INET, SOCK_DGRAM, 0), "Failed to open socket");

    groupSock.sin_family = AF_INET;
    groupSock.sin_addr.s_addr = MULTICAST_ADDR;
    groupSock.sin_port = htons(4321);
    localInterface.s_addr = htons(INADDR_ANY);

    guard(setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface)), "Failed setting local interface");
    guard(sendto(sd, databuf, datalen, 0, (struct sockaddr*)&groupSock, sizeof(groupSock)), "Failed sending datagram");

    return EXIT_SUCCESS;
}

#define RUN_WITH_ARG(ARG, NAME, FUNC) \
    if (strcmp(ARG, NAME) == 0) {     \
        guard(FUNC(), "Failed");      \
    }

int
main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: %s <setget|timeout_client|timeout_server|multicast_client|multicast_server>\n", argv[0]);
        return EXIT_FAILURE;
    }

    RUN_WITH_ARG(argv[1], "setget", test_set_and_get_opts)
    RUN_WITH_ARG(argv[1], "timeout_client", test_send_and_recv_timeout_client)
    RUN_WITH_ARG(argv[1], "timeout_server", test_send_and_recv_timeout_server)
    RUN_WITH_ARG(argv[1], "multicast_client", test_multicast_client)
    RUN_WITH_ARG(argv[1], "multicast_server", test_multicast_server)

    printf("socket_opts finished\n");
    return EXIT_SUCCESS;
}
