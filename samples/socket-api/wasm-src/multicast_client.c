#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
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

static void
init_sockaddr_inet(struct sockaddr_in *addr)
{
    addr->sin_family = AF_INET;
    addr->sin_port = htons(1234);
}

static void
init_sockaddr_inet6(struct sockaddr_in6 *addr)
{
    addr->sin6_family = AF_INET6;
    addr->sin6_port = htons(1234);
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

int
main(int argc, char *argv[])
{
    struct ipv6_mreq ipv6_group;
    struct ip_mreq ipv4_group;
    int sd;
    int datalen;
    char databuf[1024];
    char multicast_addr_buffer[16];
    struct sockaddr_storage local_address = { 0 };
    int addr_type = -1;
    int read_result;

    if (argc < 2) {
        printf("Usage is <Multicast IP>\n");
        return -1;
    }

    addr_type = get_ip_addr_type(argv[1], multicast_addr_buffer);

    if (!is_valid_addr_type(addr_type)) {
        printf("Not a valid ipv4 or ipv6 address\n");
        return -1;
    }

    sd = guard(socket(addr_type, SOCK_DGRAM, 0), "Failed opening socket");
    guard(set_and_get_bool_opt(sd, SOL_SOCKET, SO_REUSEADDR, 1),
          "Failed setting SO_REUSEADDR");

    if (addr_type == AF_INET) {
        init_sockaddr_inet((struct sockaddr_in *)&local_address);
        memcpy(&(ipv4_group.imr_multiaddr), multicast_addr_buffer, 4);
        ipv4_group.imr_interface.s_addr = htonl(INADDR_ANY);
    }
    else {
        init_sockaddr_inet6((struct sockaddr_in6 *)&local_address);
        memcpy(&(ipv6_group.ipv6mr_multiaddr), multicast_addr_buffer, 16);
        ipv6_group.ipv6mr_interface = 0;
        guard(setsockopt(sd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &ipv6_group,
                         sizeof(ipv6_group)),
              "Failed adding multicast group");
    }

    guard(bind(sd, (struct sockaddr *)&local_address, sizeof(local_address)),
          "Failed binding socket");

    printf("Joined multicast group. Waiting for datagram...\n");

    datalen = sizeof(databuf);
    read_result = read(sd, databuf, datalen);

    if (read_result < 0) {
        printf("Reading datagram failed\n");
        return -1;
    }

    printf("Reading datagram message...OK.\n");
    printf("The message from multicast server is: \"%s\"\n", databuf);

    return EXIT_SUCCESS;
}