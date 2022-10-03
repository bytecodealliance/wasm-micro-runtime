#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#ifdef __wasi__
#include <wasi_socket_ext.h>
#endif

int
guard(int n, char *err)
{
    if (n == -1) {
        perror(err);
        exit(1);
    }
    return n;
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

static int
get_ip_addr_type(char *addr, char *buf)
{
    if (inet_pton(AF_INET6, addr, buf)) {
        return AF_INET6;
    }
    if (inet_pton(AF_INET, addr, buf)) {
        return AF_INET;
    }
    return -1;
}

static int
is_valid_addr_type(int addr_type)
{
    return !(addr_type == -1
             || (addr_type != AF_INET && addr_type != AF_INET6));
}

static void
init_sockaddr_inet(struct sockaddr_in *addr, char *addr_buffer)
{
    addr->sin_family = AF_INET;
    addr->sin_port = htons(1234);
    memcpy(&(addr->sin_addr), addr_buffer, 4);
}

static void
init_sockaddr_inet6(struct sockaddr_in6 *addr, char *addr_buffer)
{
    addr->sin6_family = AF_INET6;
    addr->sin6_port = htons(1234);
    memcpy(&(addr->sin6_addr), addr_buffer, 16);
}

int
main(int argc, char *argv[])
{
    struct sockaddr_storage addr = { 0 };
    int sd;
    char *databuf = "Test message";
    int datalen = strlen(databuf) + 1;
    char multicast_addr_buffer[16];
    int addr_type = -1;
    int multicast_interface;

    if (argc < 2) {
        printf("Usage is <Multicast IP>\n");
        return -1;
    }

    addr_type = get_ip_addr_type(argv[1], multicast_addr_buffer);

    if (!is_valid_addr_type(addr_type)) {
        printf("Not a valid ipv4 or ipv6 address\n");
        return -1;
    }

    sd = guard(socket(addr_type, SOCK_DGRAM, 0), "Failed to open socket");
    guard(set_and_get_bool_opt(sd, SOL_SOCKET, SO_REUSEADDR, 1),
          "Failed setting SO_REUSEADDR");

    if (addr_type == AF_INET) {
        multicast_interface = htonl(INADDR_ANY);
        guard(setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF,
                         (char *)&multicast_interface,
                         sizeof(multicast_interface)),
              "Failed setting local interface");
        init_sockaddr_inet((struct sockaddr_in *)&addr, multicast_addr_buffer);
    }
    else {
        multicast_interface = 0;
        guard(setsockopt(sd, IPPROTO_IPV6, IPV6_MULTICAST_IF,
                         (char *)&multicast_interface,
                         sizeof(multicast_interface)),
              "Failed setting local interface");
        init_sockaddr_inet6((struct sockaddr_in6 *)&addr,
                            multicast_addr_buffer);
    }

    guard(
        sendto(sd, databuf, datalen, 0, (struct sockaddr *)&addr, sizeof(addr)),
        "Failed sending datagram");

    printf("Datagram sent\n");

    return EXIT_SUCCESS;
}