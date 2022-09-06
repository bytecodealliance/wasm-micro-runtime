/*
 * Copyright (C) 2021 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_vmcore.h"
#include "platform_api_extension.h"

#include <arpa/inet.h>
#include <netdb.h>

static bool
textual_addr_to_sockaddr(const char *textual, int port, struct sockaddr *out)
{
    struct sockaddr_in *v4;
    struct sockaddr_in6 *v6;

    assert(textual);

    v4 = (struct sockaddr_in *)out;
    if (inet_pton(AF_INET, textual, &v4->sin_addr.s_addr) == 1) {
        v4->sin_family = AF_INET;
        v4->sin_port = htons(port);
        return true;
    }

    v6 = (struct sockaddr_in6 *)out;
    if (inet_pton(AF_INET6, textual, &v6->sin6_addr.s6_addr) == 1) {
        v6->sin6_family = AF_INET6;
        v6->sin6_port = htons(port);
        return true;
    }

    return false;
}

static int
sockaddr_to_bh_sockaddr(const struct sockaddr *sockaddr, socklen_t socklen,
                        bh_sockaddr_t *bh_sockaddr)
{
    switch (sockaddr->sa_family) {
        case AF_INET:
        {
            struct sockaddr_in *addr = (struct sockaddr_in *)sockaddr;

            assert(socklen >= sizeof(struct sockaddr_in));

            bh_sockaddr->port = ntohs(addr->sin_port);
            bh_sockaddr->addr_bufer.ipv4 = ntohl(addr->sin_addr.s_addr);
            bh_sockaddr->is_ipv4 = true;
            return BHT_OK;
        }
        case AF_INET6:
        {
            struct sockaddr_in6 *addr = (struct sockaddr_in6 *)sockaddr;
            size_t i;

            assert(socklen >= sizeof(struct sockaddr_in6));

            bh_sockaddr->port = ntohs(addr->sin6_port);

            for (i = 0; i < sizeof(bh_sockaddr->addr_bufer.ipv6)
                                / sizeof(bh_sockaddr->addr_bufer.ipv6[0]);
                 i++) {
                uint16 part_addr = addr->sin6_addr.s6_addr[i * 2]
                                   | (addr->sin6_addr.s6_addr[i * 2 + 1] << 8);
                bh_sockaddr->addr_bufer.ipv6[i] = ntohs(part_addr);
            }

            bh_sockaddr->is_ipv4 = false;
            return BHT_OK;
        }
        default:
            errno = EAFNOSUPPORT;
            return BHT_ERROR;
    }
}

static void
bh_sockaddr_to_sockaddr(const bh_sockaddr_t *bh_sockaddr,
                        struct sockaddr_storage *sockaddr, socklen_t *socklen)
{
    if (bh_sockaddr->is_ipv4) {
        struct sockaddr_in *addr = (struct sockaddr_in *)sockaddr;
        addr->sin_port = htons(bh_sockaddr->port);
        addr->sin_family = AF_INET;
        addr->sin_addr.s_addr = htonl(bh_sockaddr->addr_bufer.ipv4);
        *socklen = sizeof(*addr);
    }
    else {
        struct sockaddr_in6 *addr = (struct sockaddr_in6 *)sockaddr;
        size_t i;
        addr->sin6_port = htons(bh_sockaddr->port);
        addr->sin6_family = AF_INET6;

        for (i = 0; i < sizeof(bh_sockaddr->addr_bufer.ipv6)
                            / sizeof(bh_sockaddr->addr_bufer.ipv6[0]);
             i++) {
            uint16 part_addr = htons(bh_sockaddr->addr_bufer.ipv6[i]);
            addr->sin6_addr.s6_addr[i * 2] = 0xff & part_addr;
            addr->sin6_addr.s6_addr[i * 2 + 1] = (0xff00 & part_addr) >> 8;
        }

        *socklen = sizeof(*addr);
    }
}

int
os_socket_create(bh_socket_t *sock, bool is_ipv4, bool is_tcp)
{
    int af = is_ipv4 ? AF_INET : AF_INET6;

    if (!sock) {
        return BHT_ERROR;
    }

    if (is_tcp) {
        *sock = socket(af, SOCK_STREAM, IPPROTO_TCP);
    }
    else {
        *sock = socket(af, SOCK_DGRAM, 0);
    }

    return (*sock == -1) ? BHT_ERROR : BHT_OK;
}

int
os_socket_bind(bh_socket_t socket, const char *host, int *port)
{
    struct sockaddr_storage addr;
    struct linger ling;
    socklen_t socklen;
    int ret;

    assert(host);
    assert(port);

    ling.l_onoff = 1;
    ling.l_linger = 0;

    ret = fcntl(socket, F_SETFD, FD_CLOEXEC);
    if (ret < 0) {
        goto fail;
    }

    ret = setsockopt(socket, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling));
    if (ret < 0) {
        goto fail;
    }

    if (!textual_addr_to_sockaddr(host, *port, (struct sockaddr *)&addr)) {
        goto fail;
    }

    ret = bind(socket, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        goto fail;
    }

    socklen = sizeof(addr);
    if (getsockname(socket, (void *)&addr, &socklen) == -1) {
        goto fail;
    }

    *port = ntohs(addr.ss_family == AF_INET
                      ? ((struct sockaddr_in *)&addr)->sin_port
                      : ((struct sockaddr_in6 *)&addr)->sin6_port);

    return BHT_OK;

fail:
    return BHT_ERROR;
}

int
os_socket_settimeout(bh_socket_t socket, uint64 timeout_us)
{
    struct timeval tv;
    tv.tv_sec = timeout_us / 1000000UL;
    tv.tv_usec = timeout_us % 1000000UL;

    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv,
                   sizeof(tv))
        != 0) {
        return BHT_ERROR;
    }

    return BHT_OK;
}

int
os_socket_listen(bh_socket_t socket, int max_client)
{
    if (listen(socket, max_client) != 0) {
        return BHT_ERROR;
    }

    return BHT_OK;
}

int
os_socket_accept(bh_socket_t server_sock, bh_socket_t *sock, void *addr,
                 unsigned int *addrlen)
{
    struct sockaddr addr_tmp;
    unsigned int len = sizeof(struct sockaddr);

    *sock = accept(server_sock, &addr_tmp, &len);

    if (*sock < 0) {
        return BHT_ERROR;
    }

    return BHT_OK;
}

int
os_socket_connect(bh_socket_t socket, const char *addr, int port)
{
    struct sockaddr_storage addr_in = { 0 };
    socklen_t addr_len = sizeof(struct sockaddr_storage);
    int ret = 0;

    if (!textual_addr_to_sockaddr(addr, port, (struct sockaddr *)&addr_in)) {
        return BHT_ERROR;
    }

    ret = connect(socket, (struct sockaddr *)&addr_in, addr_len);
    if (ret == -1) {
        return BHT_ERROR;
    }

    return BHT_OK;
}

int
os_socket_recv(bh_socket_t socket, void *buf, unsigned int len)
{
    return recv(socket, buf, len, 0);
}

int
os_socket_recv_from(bh_socket_t socket, void *buf, unsigned int len, int flags,
                    bh_sockaddr_t *src_addr)
{
    struct sockaddr_storage sock_addr = {};
    socklen_t socklen = sizeof(sock_addr);
    int ret;

    ret = recvfrom(socket, buf, len, flags, (struct sockaddr *)&sock_addr,
                   &socklen);

    if (ret < 0) {
        return ret;
    }

    if (src_addr) {
        sockaddr_to_bh_sockaddr((struct sockaddr *)&sock_addr, socklen,
                                src_addr);
    }

    return ret;
}

int
os_socket_send(bh_socket_t socket, const void *buf, unsigned int len)
{
    return send(socket, buf, len, 0);
}

int
os_socket_send_to(bh_socket_t socket, const void *buf, unsigned int len,
                  int flags, const bh_sockaddr_t *dest_addr)
{
    struct sockaddr_storage sock_addr = {};
    socklen_t socklen = 0;

    bh_sockaddr_to_sockaddr(dest_addr, &sock_addr, &socklen);

    return sendto(socket, buf, len, 0, (const struct sockaddr *)&sock_addr,
                  socklen);
}

int
os_socket_close(bh_socket_t socket)
{
    close(socket);
    return BHT_OK;
}

int
os_socket_shutdown(bh_socket_t socket)
{
    shutdown(socket, O_RDWR);
    return BHT_OK;
}

int
os_socket_inet_network(bool is_ipv4, const char *cp, bh_ip_addr_buffer_t *out)
{
    if (!cp)
        return BHT_ERROR;

    if (is_ipv4) {
        if (inet_pton(AF_INET, cp, &out->ipv4) != 1) {
            return BHT_ERROR;
        }
        /* Note: ntohl(INADDR_NONE) == INADDR_NONE */
        out->ipv4 = ntohl(out->ipv4);
    }
    else {
        if (inet_pton(AF_INET6, cp, out->ipv6) != 1) {
            return BHT_ERROR;
        }
        for (int i = 0; i < 8; i++) {
            out->ipv6[i] = ntohs(out->ipv6[i]);
        }
    }

    return BHT_OK;
}

static int
getaddrinfo_error_to_errno(int error)
{
    switch (error) {
        case EAI_AGAIN:
            return EAGAIN;
        case EAI_FAIL:
            return EFAULT;
        case EAI_MEMORY:
            return ENOMEM;
        case EAI_SYSTEM:
            return errno;
        default:
            return EINVAL;
    }
}

static int
is_addrinfo_supported(struct addrinfo *info)
{
    return
        // Allow only IPv4 and IPv6
        (info->ai_family == AF_INET || info->ai_family == AF_INET6)
        // Allow only UDP and TCP
        && (info->ai_socktype == SOCK_DGRAM || info->ai_socktype == SOCK_STREAM)
        && (info->ai_protocol == IPPROTO_TCP
            || info->ai_protocol == IPPROTO_UDP);
}

int
os_socket_addr_resolve(const char *host, const char *service,
                       uint8_t *hint_is_tcp, uint8_t *hint_is_ipv4,
                       bh_addr_info_t *addr_info, size_t addr_info_size,
                       size_t *max_info_size)
{
    struct addrinfo hints = { 0 }, *res, *result;
    int hints_enabled = hint_is_tcp || hint_is_ipv4;
    int ret;
    size_t pos = 0;

    if (hints_enabled) {
        if (hint_is_ipv4) {
            hints.ai_family = *hint_is_ipv4 ? AF_INET : AF_INET6;
        }
        if (hint_is_tcp) {
            hints.ai_socktype = *hint_is_tcp ? SOCK_STREAM : SOCK_DGRAM;
        }
    }

    ret = getaddrinfo(host, strlen(service) == 0 ? NULL : service,
                      hints_enabled ? &hints : NULL, &result);
    if (ret != BHT_OK) {
        errno = getaddrinfo_error_to_errno(ret);
        return BHT_ERROR;
    }

    res = result;
    while (res) {
        if (addr_info_size > pos) {
            if (!is_addrinfo_supported(res)) {
                res = res->ai_next;
                continue;
            }

            sockaddr_to_bh_sockaddr(res->ai_addr, sizeof(struct sockaddr_in),
                                    &addr_info[pos].sockaddr);

            addr_info[pos].is_tcp = res->ai_socktype == SOCK_STREAM;
        }

        pos++;
        res = res->ai_next;
    }

    *max_info_size = pos;
    freeaddrinfo(result);

    return BHT_OK;
}

int
os_socket_set_send_timeout(bh_socket_t socket, uint64 timeout_us)
{
    struct timeval tv;
    tv.tv_sec = timeout_us / 1000000UL;
    tv.tv_usec = timeout_us % 1000000UL;
    if (setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) != 0) {
        return BHT_ERROR;
    }
    return BHT_OK;
}

int
os_socket_get_send_timeout(bh_socket_t socket, uint64 *timeout_us)
{
    struct timeval tv;
    socklen_t tv_len = sizeof(tv);
    if (getsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &tv, &tv_len) != 0) {
        return BHT_ERROR;
    }
    *timeout_us = (tv.tv_sec * 1000000UL) + tv.tv_usec;
    return BHT_OK;
}

int
os_socket_set_recv_timeout(bh_socket_t socket, uint64 timeout_us)
{
    struct timeval tv;
    tv.tv_sec = timeout_us / 1000000UL;
    tv.tv_usec = timeout_us % 1000000UL;
    if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) {
        return BHT_ERROR;
    }
    return BHT_OK;
}

int
os_socket_get_recv_timeout(bh_socket_t socket, uint64 *timeout_us)
{
    struct timeval tv;
    socklen_t tv_len = sizeof(tv);
    if (getsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &tv, &tv_len) != 0) {
        return BHT_ERROR;
    }
    *timeout_us = (tv.tv_sec * 1000000UL) + tv.tv_usec;
    return BHT_OK;
}

int
os_socket_addr_local(bh_socket_t socket, bh_sockaddr_t *sockaddr)
{
    struct sockaddr_storage addr_storage = { 0 };
    socklen_t addr_len = sizeof(addr_storage);
    int ret;

    ret = getsockname(socket, (struct sockaddr *)&addr_storage, &addr_len);

    if (ret != BHT_OK) {
        return BHT_ERROR;
    }

    return sockaddr_to_bh_sockaddr((struct sockaddr *)&addr_storage, addr_len,
                                   sockaddr);
}

int
os_socket_addr_remote(bh_socket_t socket, bh_sockaddr_t *sockaddr)
{
    struct sockaddr_storage addr_storage = { 0 };
    socklen_t addr_len = sizeof(addr_storage);
    int ret;

    ret = getpeername(socket, (struct sockaddr *)&addr_storage, &addr_len);

    if (ret != BHT_OK) {
        return BHT_ERROR;
    }

    return sockaddr_to_bh_sockaddr((struct sockaddr *)&addr_storage, addr_len,
                                   sockaddr);
}