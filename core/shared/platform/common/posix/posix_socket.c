/*
 * Copyright (C) 2021 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_vmcore.h"
#include "platform_api_extension.h"

#include <arpa/inet.h>
#include <netdb.h>

static void
textual_addr_to_sockaddr(const char *textual, int port, struct sockaddr_in *out)
{
    assert(textual);

    out->sin_family = AF_INET;
    out->sin_port = htons(port);
    out->sin_addr.s_addr = inet_addr(textual);
}

int
os_socket_create(bh_socket_t *sock, int tcp_or_udp)
{
    if (!sock) {
        return BHT_ERROR;
    }

    if (1 == tcp_or_udp) {
        *sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }
    else if (0 == tcp_or_udp) {
        *sock = socket(AF_INET, SOCK_DGRAM, 0);
    }

    return (*sock == -1) ? BHT_ERROR : BHT_OK;
}

int
os_socket_bind(bh_socket_t socket, const char *host, int *port)
{
    struct sockaddr_in addr;
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

    addr.sin_addr.s_addr = inet_addr(host);
    addr.sin_port = htons(*port);
    addr.sin_family = AF_INET;

    ret = bind(socket, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        goto fail;
    }

    socklen = sizeof(addr);
    if (getsockname(socket, (void *)&addr, &socklen) == -1) {
        goto fail;
    }

    *port = ntohs(addr.sin_port);

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

    *sock = accept(server_sock, (struct sockaddr *)&addr_tmp, &len);

    if (*sock < 0) {
        return BHT_ERROR;
    }

    return BHT_OK;
}

int
os_socket_connect(bh_socket_t socket, const char *addr, int port)
{
    struct sockaddr_in addr_in = { 0 };
    socklen_t addr_len = sizeof(struct sockaddr_in);
    int ret = 0;

    textual_addr_to_sockaddr(addr, port, &addr_in);

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
os_socket_send(bh_socket_t socket, const void *buf, unsigned int len)
{
    return send(socket, buf, len, 0);
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
os_socket_inet_network(const char *cp, uint32 *out)
{
    if (!cp)
        return BHT_ERROR;

    /* Note: ntohl(INADDR_NONE) == INADDR_NONE */
    *out = ntohl(inet_addr(cp));
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
            hints.ai_family = *hint_is_ipv4 ? PF_INET : PF_INET6;
        }
        if (hint_is_tcp) {
            hints.ai_socktype = *hint_is_tcp ? SOCK_STREAM : SOCK_DGRAM;
        }
    }

    ret = getaddrinfo(host, service, hints_enabled ? &hints : NULL, &result);
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

            if (res->ai_family == AF_INET) {
                struct sockaddr_in *addr_in =
                    (struct sockaddr_in *)res->ai_addr;

                addr_info[pos].port = addr_in->sin_port;
                addr_info[pos].is_ipv4 = 1;
                memcpy(addr_info[pos].addr, &addr_in->sin_addr,
                       sizeof(addr_in->sin_addr));
            }
            else {
                struct sockaddr_in6 *addr_in =
                    (struct sockaddr_in6 *)res->ai_addr;

                addr_info[pos].port = addr_in->sin6_port;
                addr_info[pos].is_ipv4 = 0;
                for (int i = 0; i < 8; i++) {
                    ((uint16 *)addr_info[pos].addr)[i] =
                        ntohs(((uint16_t *)&addr_in->sin6_addr)[i]);
                }
            }

            addr_info[pos].is_tcp = res->ai_socktype == SOCK_STREAM;
        }

        pos++;
        res = res->ai_next;
    }

    *max_info_size = pos;
    freeaddrinfo(result);

    return BHT_OK;
}