/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <wasi/api.h>
#include <wasi_socket_ext.h>

#define HANDLE_ERROR(error)              \
    if (error != __WASI_ERRNO_SUCCESS) { \
        errno = error;                   \
        return -1;                       \
    }

/* addr_num and port are in network order */
static void
ipv4_addr_to_wasi_addr(uint32_t addr_num, uint16_t port, __wasi_addr_t *out)
{
    addr_num = ntohl(addr_num);
    out->kind = IPv4;
    out->addr.ip4.port = ntohs(port);
    out->addr.ip4.addr.n0 = (addr_num & 0xFF000000) >> 24;
    out->addr.ip4.addr.n1 = (addr_num & 0x00FF0000) >> 16;
    out->addr.ip4.addr.n2 = (addr_num & 0x0000FF00) >> 8;
    out->addr.ip4.addr.n3 = (addr_num & 0x000000FF);
}

static void
ipv6_addr_to_wasi_addr(uint16_t *addr, uint16_t port, __wasi_addr_t *out)
{
    out->kind = IPv6;
    out->addr.ip6.port = ntohs(port);
    out->addr.ip6.addr.n0 = ntohs(addr[0]);
    out->addr.ip6.addr.n1 = ntohs(addr[1]);
    out->addr.ip6.addr.n2 = ntohs(addr[2]);
    out->addr.ip6.addr.n3 = ntohs(addr[3]);
    out->addr.ip6.addr.h0 = ntohs(addr[4]);
    out->addr.ip6.addr.h1 = ntohs(addr[5]);
    out->addr.ip6.addr.h2 = ntohs(addr[6]);
    out->addr.ip6.addr.h3 = ntohs(addr[7]);
}

static __wasi_errno_t
sockaddr_to_wasi_addr(const struct sockaddr *sock_addr, socklen_t addrlen,
                      __wasi_addr_t *wasi_addr)
{
    __wasi_errno_t ret = __WASI_ERRNO_SUCCESS;
    if (AF_INET == sock_addr->sa_family) {
        assert(sizeof(struct sockaddr_in) <= addrlen);

        ipv4_addr_to_wasi_addr(
            ((struct sockaddr_in *)sock_addr)->sin_addr.s_addr,
            ((struct sockaddr_in *)sock_addr)->sin_port, wasi_addr);
    }
    else if (AF_INET6 == sock_addr->sa_family) {
        assert(sizeof(struct sockaddr_in6) <= addrlen);
        ipv6_addr_to_wasi_addr(
            (uint16_t *)((struct sockaddr_in6 *)sock_addr)->sin6_addr.s6_addr,
            ((struct sockaddr_in6 *)sock_addr)->sin6_port, wasi_addr);
    }
    else {
        ret = __WASI_ERRNO_AFNOSUPPORT;
    }

    return ret;
}

static __wasi_errno_t
wasi_addr_to_sockaddr(const __wasi_addr_t *wasi_addr,
                      struct sockaddr *sock_addr, socklen_t *addrlen)
{
    switch (wasi_addr->kind) {
        case IPv4:
        {
            struct sockaddr_in sock_addr_in = { 0 };
            uint32_t s_addr;

            s_addr = (wasi_addr->addr.ip4.addr.n0 << 24)
                     | (wasi_addr->addr.ip4.addr.n1 << 16)
                     | (wasi_addr->addr.ip4.addr.n2 << 8)
                     | wasi_addr->addr.ip4.addr.n3;

            sock_addr_in.sin_family = AF_INET;
            sock_addr_in.sin_addr.s_addr = htonl(s_addr);
            sock_addr_in.sin_port = htons(wasi_addr->addr.ip4.port);
            memcpy(sock_addr, &sock_addr_in, sizeof(sock_addr_in));

            *addrlen = sizeof(sock_addr_in);
            break;
        }
        case IPv6:
        {
            struct sockaddr_in6 sock_addr_in6 = { 0 };
            uint16_t *addr_buf = (uint16_t *)sock_addr_in6.sin6_addr.s6_addr;

            addr_buf[0] = htons(wasi_addr->addr.ip6.addr.n0);
            addr_buf[1] = htons(wasi_addr->addr.ip6.addr.n1);
            addr_buf[2] = htons(wasi_addr->addr.ip6.addr.n2);
            addr_buf[3] = htons(wasi_addr->addr.ip6.addr.n3);
            addr_buf[4] = htons(wasi_addr->addr.ip6.addr.h0);
            addr_buf[5] = htons(wasi_addr->addr.ip6.addr.h1);
            addr_buf[6] = htons(wasi_addr->addr.ip6.addr.h2);
            addr_buf[7] = htons(wasi_addr->addr.ip6.addr.h3);

            sock_addr_in6.sin6_family = AF_INET6;
            sock_addr_in6.sin6_port = htons(wasi_addr->addr.ip6.port);
            memcpy(sock_addr, &sock_addr_in6, sizeof(sock_addr_in6));

            *addrlen = sizeof(sock_addr_in6);
            break;
        }
        default:
            return __WASI_ERRNO_AFNOSUPPORT;
    }
    return __WASI_ERRNO_SUCCESS;
}

int
accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    __wasi_addr_t wasi_addr = { 0 };
    __wasi_fd_t new_sockfd;
    __wasi_errno_t error;

    error = __wasi_sock_accept(sockfd, &new_sockfd);
    HANDLE_ERROR(error)

    error = getpeername(new_sockfd, addr, addrlen);
    HANDLE_ERROR(error)

    return new_sockfd;
}

int
bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    __wasi_addr_t wasi_addr = { 0 };
    __wasi_errno_t error;

    error = sockaddr_to_wasi_addr(addr, addrlen, &wasi_addr);
    HANDLE_ERROR(error)

    error = __wasi_sock_bind(sockfd, &wasi_addr);
    HANDLE_ERROR(error)

    return __WASI_ERRNO_SUCCESS;
}

int
connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    __wasi_addr_t wasi_addr = { 0 };
    __wasi_errno_t error;

    if (NULL == addr) {
        HANDLE_ERROR(__WASI_ERRNO_INVAL)
    }

    error = sockaddr_to_wasi_addr(addr, addrlen, &wasi_addr);
    HANDLE_ERROR(error)

    error = __wasi_sock_connect(sockfd, &wasi_addr);
    HANDLE_ERROR(error)

    return __WASI_ERRNO_SUCCESS;
}

int
listen(int sockfd, int backlog)
{
    __wasi_errno_t error = __wasi_sock_listen(sockfd, backlog);
    HANDLE_ERROR(error)
    return __WASI_ERRNO_SUCCESS;
}

ssize_t
recvmsg(int sockfd, struct msghdr *msg, int flags)
{
    // Prepare input parameters.
    __wasi_iovec_t *ri_data = NULL;
    size_t i = 0;
    size_t ro_datalen = 0;
    __wasi_roflags_t ro_flags = 0;

    if (NULL == msg) {
        HANDLE_ERROR(__WASI_ERRNO_INVAL)
    }

    // Validate flags.
    if (flags != 0) {
        HANDLE_ERROR(__WASI_ERRNO_NOPROTOOPT)
    }

    // __wasi_ciovec_t -> struct iovec
    if (!(ri_data = malloc(sizeof(__wasi_iovec_t) * msg->msg_iovlen))) {
        HANDLE_ERROR(__WASI_ERRNO_NOMEM)
    }

    for (i = 0; i < msg->msg_iovlen; i++) {
        ri_data[i].buf = msg->msg_iov[i].iov_base;
        ri_data[i].buf_len = msg->msg_iov[i].iov_len;
    }

    // Perform system call.
    __wasi_errno_t error = __wasi_sock_recv(sockfd, ri_data, msg->msg_iovlen, 0,
                                            &ro_datalen, &ro_flags);
    free(ri_data);
    HANDLE_ERROR(error)

    return ro_datalen;
}

ssize_t
sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
    // Prepare input parameters.
    __wasi_ciovec_t *si_data = NULL;
    size_t so_datalen = 0;
    size_t i = 0;

    if (NULL == msg) {
        HANDLE_ERROR(__WASI_ERRNO_INVAL)
    }

    // This implementation does not support any flags.
    if (flags != 0) {
        HANDLE_ERROR(__WASI_ERRNO_NOPROTOOPT)
    }

    // struct iovec -> __wasi_ciovec_t
    if (!(si_data = malloc(sizeof(__wasi_ciovec_t) * msg->msg_iovlen))) {
        HANDLE_ERROR(__WASI_ERRNO_NOMEM)
    }

    for (i = 0; i < msg->msg_iovlen; i++) {
        si_data[i].buf = msg->msg_iov[i].iov_base;
        si_data[i].buf_len = msg->msg_iov[i].iov_len;
    }

    // Perform system call.
    __wasi_errno_t error =
        __wasi_sock_send(sockfd, si_data, msg->msg_iovlen, 0, &so_datalen);
    free(si_data);
    HANDLE_ERROR(error)

    return so_datalen;
}

ssize_t
sendto(int sockfd, const void *buf, size_t len, int flags,
       const struct sockaddr *dest_addr, socklen_t addrlen)
{
    // Prepare input parameters.
    __wasi_ciovec_t iov = { .buf = buf, .buf_len = len };
    uint32_t so_datalen = 0;
    __wasi_addr_t wasi_addr;
    __wasi_errno_t error;
    size_t si_data_len = 1;
    __wasi_siflags_t si_flags = 0;

    // This implementation does not support any flags.
    if (flags != 0) {
        HANDLE_ERROR(__WASI_ERRNO_NOPROTOOPT)
    }

    error = sockaddr_to_wasi_addr(dest_addr, addrlen, &wasi_addr);
    HANDLE_ERROR(error);

    // Perform system call.
    error = __wasi_sock_send_to(sockfd, &iov, si_data_len, si_flags, &wasi_addr,
                                &so_datalen);
    HANDLE_ERROR(error)

    return so_datalen;
}

ssize_t
recvfrom(int sockfd, void *buf, size_t len, int flags,
         struct sockaddr *src_addr, socklen_t *addrlen)
{
    // Prepare input parameters.
    __wasi_ciovec_t iov = { .buf = buf, .buf_len = len };
    uint32_t so_datalen = 0;
    __wasi_addr_t wasi_addr;
    __wasi_errno_t error;
    size_t si_data_len = 1;
    __wasi_siflags_t si_flags = 0;

    // This implementation does not support any flags.
    if (flags != 0) {
        HANDLE_ERROR(__WASI_ERRNO_NOPROTOOPT)
    }

    if (!src_addr) {
        return recv(sockfd, buf, len, flags);
    }

    // Perform system call.
    error = __wasi_sock_recv_from(sockfd, &iov, si_data_len, si_flags,
                                  &wasi_addr, &so_datalen);
    HANDLE_ERROR(error);

    error = wasi_addr_to_sockaddr(&wasi_addr, src_addr, addrlen);
    HANDLE_ERROR(error);

    return so_datalen;
}

int
socket(int domain, int type, int protocol)
{
    // the stub of address pool fd
    __wasi_fd_t poolfd = -1;
    __wasi_fd_t sockfd;
    __wasi_errno_t error;
    __wasi_address_family_t af;
    __wasi_sock_type_t socktype;

    if (AF_INET == domain) {
        af = INET4;
    }
    else if (AF_INET6 == domain) {
        af = INET6;
    }
    else {
        return __WASI_ERRNO_NOPROTOOPT;
    }

    if (SOCK_DGRAM == type) {
        socktype = SOCKET_DGRAM;
    }
    else if (SOCK_STREAM == type) {
        socktype = SOCKET_STREAM;
    }
    else {
        return __WASI_ERRNO_NOPROTOOPT;
    }

    error = __wasi_sock_open(poolfd, af, socktype, &sockfd);
    HANDLE_ERROR(error)

    return sockfd;
}

int
getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    __wasi_addr_t wasi_addr = { 0 };
    __wasi_errno_t error;

    error = __wasi_sock_addr_local(sockfd, &wasi_addr);
    HANDLE_ERROR(error)

    error = wasi_addr_to_sockaddr(&wasi_addr, addr, addrlen);
    HANDLE_ERROR(error)

    return __WASI_ERRNO_SUCCESS;
}

int
getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    __wasi_addr_t wasi_addr = { 0 };
    __wasi_errno_t error;

    error = __wasi_sock_addr_remote(sockfd, &wasi_addr);
    HANDLE_ERROR(error)

    error = wasi_addr_to_sockaddr(&wasi_addr, addr, addrlen);
    HANDLE_ERROR(error)

    return __WASI_ERRNO_SUCCESS;
}

struct aibuf {
    struct addrinfo ai;
    union sa {
        struct sockaddr_in sin;
        struct sockaddr_in6 sin6;
    } sa;
};

static __wasi_errno_t
addrinfo_hints_to_wasi_hints(const struct addrinfo *hints,
                             __wasi_addr_info_hints_t *wasi_hints)
{
    if (hints) {
        wasi_hints->hints_enabled = 1;

        switch (hints->ai_family) {
            case AF_INET:
                wasi_hints->family = INET4;
                break;
            case AF_INET6:
                wasi_hints->family = INET6;
                break;
            default:
                return __WASI_ERRNO_AFNOSUPPORT;
        }
        switch (hints->ai_socktype) {
            case SOCK_STREAM:
                wasi_hints->type = SOCKET_STREAM;
                break;
            case SOCK_DGRAM:
                wasi_hints->type = SOCKET_DGRAM;
                break;
            default:
                return __WASI_ERRNO_NOTSUP;
        }

        if (hints->ai_protocol != 0) {
            return __WASI_ERRNO_NOTSUP;
        }

        if (hints->ai_flags != 0) {
            return __WASI_ERRNO_NOTSUP;
        }
    }
    else {
        wasi_hints->hints_enabled = 0;
    }

    return __WASI_ERRNO_SUCCESS;
}

static __wasi_errno_t
wasi_addr_info_to_addr_info(const __wasi_addr_info_t *addr_info,
                            struct addrinfo *ai)
{
    ai->ai_socktype =
        addr_info->type == SOCKET_DGRAM ? SOCK_DGRAM : SOCK_STREAM;
    ai->ai_protocol = 0;
    ai->ai_canonname = NULL;

    if (addr_info->addr.kind == IPv4) {
        ai->ai_family = AF_INET;
        ai->ai_addrlen = sizeof(struct sockaddr_in);
    }
    else {
        ai->ai_family = AF_INET6;
        ai->ai_addrlen = sizeof(struct sockaddr_in6);
    }

    return wasi_addr_to_sockaddr(&addr_info->addr, ai->ai_addr,
                                 &ai->ai_addrlen); // TODO err handling
}

int
getaddrinfo(const char *node, const char *service, const struct addrinfo *hints,
            struct addrinfo **res)
{
    __wasi_addr_info_hints_t wasi_hints;
    __wasi_addr_info_t *addr_info = NULL;
    __wasi_size_t addr_info_size, i;
    __wasi_size_t max_info_size = 16;
    __wasi_errno_t error;
    struct aibuf *aibuf_res;

    error = addrinfo_hints_to_wasi_hints(hints, &wasi_hints);
    HANDLE_ERROR(error)

    do {
        if (addr_info)
            free(addr_info);

        addr_info_size = max_info_size;
        addr_info = (__wasi_addr_info_t *)malloc(addr_info_size
                                                 * sizeof(__wasi_addr_info_t));

        if (!addr_info) {
            HANDLE_ERROR(__WASI_ERRNO_NOMEM)
        }

        error = __wasi_sock_addr_resolve(node, service == NULL ? "" : service,
                                         &wasi_hints, addr_info, addr_info_size,
                                         &max_info_size);
        if (error != __WASI_ERRNO_SUCCESS) {
            free(addr_info);
            HANDLE_ERROR(error);
        }
    } while (max_info_size > addr_info_size);

    if (addr_info_size == 0) {
        free(addr_info);
        *res = NULL;
        return __WASI_ERRNO_SUCCESS;
    }

    aibuf_res = calloc(1, addr_info_size * sizeof(struct aibuf));
    if (!aibuf_res) {
        free(addr_info);
        HANDLE_ERROR(__WASI_ERRNO_NOMEM)
    }

    *res = &aibuf_res[0].ai;

    if (addr_info_size) {
        addr_info_size = max_info_size;
    }

    for (i = 0; i < addr_info_size; i++) {
        struct addrinfo *ai = &aibuf_res[i].ai;
        ai->ai_addr = (struct sockaddr *)&aibuf_res[i].sa;

        error = wasi_addr_info_to_addr_info(&addr_info[i], ai);
        if (error != __WASI_ERRNO_SUCCESS) {
            free(addr_info);
            free(aibuf_res);
            HANDLE_ERROR(error)
        }
        ai->ai_next = i == addr_info_size - 1 ? NULL : &aibuf_res[i + 1].ai;
    }

    free(addr_info);

    return __WASI_ERRNO_SUCCESS;
}

void
freeaddrinfo(struct addrinfo *res)
{
    /* res is a pointer to a first field in the first element
     * of aibuf array allocated in getaddrinfo, therefore this call
     * frees the memory of the entire array. */
    free(res);
}

struct timeval
time_us_to_timeval(uint64_t time_us)
{
    struct timeval tv;
    tv.tv_sec = time_us / 1000000UL;
    tv.tv_usec = time_us % 1000000UL;
    return tv;
}

uint64_t
timeval_to_time_us(struct timeval tv)
{
    return (tv.tv_sec * 1000000UL) + tv.tv_usec;
}

int
get_sol_socket_option(int sockfd, int optname, void *__restrict optval,
                      socklen_t *__restrict optlen)
{
    __wasi_errno_t error;
    uint64_t timeout_us;

    switch (optname) {
        case SO_RCVTIMEO:
            assert(*optlen == sizeof(struct timeval));
            error = __wasi_sock_get_recv_timeout(sockfd, &timeout_us);
            HANDLE_ERROR(error);
            *(struct timeval *)optval = time_us_to_timeval(timeout_us);
            return error;
        case SO_SNDTIMEO:
            assert(*optlen == sizeof(struct timeval));
            error = __wasi_sock_get_send_timeout(sockfd, &timeout_us);
            HANDLE_ERROR(error);
            *(struct timeval *)optval = time_us_to_timeval(timeout_us);
            return error;
    }

    HANDLE_ERROR(__WASI_ERRNO_NOTSUP);
}

int
getsockopt(int sockfd, int level, int optname, void *__restrict optval,
           socklen_t *__restrict optlen)
{
    switch (level) {
        case SOL_SOCKET:
            return get_sol_socket_option(sockfd, optname, optval, optlen);
    }

    HANDLE_ERROR(__WASI_ERRNO_NOTSUP);
}

int
set_sol_socket_option(int sockfd, int optname, const void *optval,
                      socklen_t optlen)
{
    __wasi_errno_t error;
    uint64_t timeout_us;

    switch (optname) {
        case SO_RCVTIMEO:
            assert(optlen == sizeof(struct timeval));
            timeout_us = timeval_to_time_us(*(struct timeval *)optval);
            error = __wasi_sock_set_recv_timeout(sockfd, timeout_us);
            HANDLE_ERROR(error);
            return error;
        case SO_SNDTIMEO:
            assert(optlen == sizeof(struct timeval));
            timeout_us = timeval_to_time_us(*(struct timeval *)optval);
            error = __wasi_sock_set_send_timeout(sockfd, timeout_us);
            HANDLE_ERROR(error);
            return error;
    }

    HANDLE_ERROR(__WASI_ERRNO_NOTSUP);
}

int
setsockopt(int sockfd, int level, int optname, const void *optval,
           socklen_t optlen)
{
    switch (level) {
        case SOL_SOCKET:
            return set_sol_socket_option(sockfd, optname, optval, optlen);
    }

    HANDLE_ERROR(__WASI_ERRNO_NOTSUP);
}
