/*
 * Copyright (C) 2023 Midokura Japan KK.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <errno.h>

#include "ssp_config.h"
#include "blocking_op.h"

int
blocking_op_close(wasm_exec_env_t exec_env, int fd)
{
    if (!wasm_runtime_begin_blocking_op(exec_env)) {
        errno = EINTR;
        return -1;
    }
    int ret = close(fd);
    wasm_runtime_end_blocking_op(exec_env);
    return ret;
}

ssize_t
blocking_op_readv(wasm_exec_env_t exec_env, int fd, const struct iovec *iov,
                  int iovcnt)
{
#ifdef BH_PLATFORM_WINDOWS
    errno = ENOTSUP;
    return -1;
#else
    if (!wasm_runtime_begin_blocking_op(exec_env)) {
        errno = EINTR;
        return -1;
    }
    ssize_t ret = readv(fd, iov, iovcnt);
    wasm_runtime_end_blocking_op(exec_env);
    return ret;
#endif
}

#if CONFIG_HAS_PREADV
ssize_t
blocking_op_preadv(wasm_exec_env_t exec_env, int fd, const struct iovec *iov,
                   int iovcnt, off_t offset)
{
    if (!wasm_runtime_begin_blocking_op(exec_env)) {
        errno = EINTR;
        return -1;
    }
    ssize_t ret = preadv(fd, iov, iovcnt, offset);
    wasm_runtime_end_blocking_op(exec_env);
    return ret;
}
#else /* CONFIG_HAS_PREADV */
ssize_t
blocking_op_pread(wasm_exec_env_t exec_env, int fd, void *p, size_t nb,
                  off_t offset)
{
#ifdef BH_PLATFORM_WINDOWS
    errno = ENOTSUP;
    return -1;
#else
    if (!wasm_runtime_begin_blocking_op(exec_env)) {
        errno = EINTR;
        return -1;
    }
    ssize_t ret = pread(fd, p, nb, offset);
    wasm_runtime_end_blocking_op(exec_env);
    return ret;
#endif
}
#endif /* CONFIG_HAS_PREADV */

ssize_t
blocking_op_writev(wasm_exec_env_t exec_env, int fd, const struct iovec *iov,
                   int iovcnt)
{
#ifdef BH_PLATFORM_WINDOWS
    errno = ENOTSUP;
    return -1;
#else
    if (!wasm_runtime_begin_blocking_op(exec_env)) {
        errno = EINTR;
        return -1;
    }
    ssize_t ret = writev(fd, iov, iovcnt);
    wasm_runtime_end_blocking_op(exec_env);
    return ret;
#endif
}

#if CONFIG_HAS_PWRITEV
ssize_t
blocking_op_pwritev(wasm_exec_env_t exec_env, int fd, const struct iovec *iov,
                    int iovcnt, off_t offset)
{
    if (!wasm_runtime_begin_blocking_op(exec_env)) {
        errno = EINTR;
        return -1;
    }
    ssize_t ret = pwritev(fd, iov, iovcnt, offset);
    wasm_runtime_end_blocking_op(exec_env);
    return ret;
}
#else /* CONFIG_HAS_PWRITEV */
ssize_t
blocking_op_pwrite(wasm_exec_env_t exec_env, int fd, const void *p, size_t nb,
                   off_t offset)
{
#ifdef BH_PLATFORM_WINDOWS
    errno = ENOTSUP;
    return -1;
#else
    if (!wasm_runtime_begin_blocking_op(exec_env)) {
        errno = EINTR;
        return -1;
    }
    ssize_t ret = pwrite(fd, p, nb, offset);
    wasm_runtime_end_blocking_op(exec_env);
    return ret;
#endif
}
#endif /* CONFIG_HAS_PWRITEV */

int
blocking_op_socket_accept(wasm_exec_env_t exec_env, bh_socket_t server_sock,
                          bh_socket_t *sockp, void *addr,
                          unsigned int *addrlenp)
{
    if (!wasm_runtime_begin_blocking_op(exec_env)) {
        errno = EINTR;
        return -1;
    }
    int ret = os_socket_accept(server_sock, sockp, addr, addrlenp);
    wasm_runtime_end_blocking_op(exec_env);
    return ret;
}

int
blocking_op_socket_connect(wasm_exec_env_t exec_env, bh_socket_t sock,
                           const char *addr, int port)
{
    if (!wasm_runtime_begin_blocking_op(exec_env)) {
        errno = EINTR;
        return -1;
    }
    int ret = os_socket_connect(sock, addr, port);
    wasm_runtime_end_blocking_op(exec_env);
    return ret;
}

int
blocking_op_socket_recv_from(wasm_exec_env_t exec_env, bh_socket_t sock,
                             void *buf, unsigned int len, int flags,
                             bh_sockaddr_t *src_addr)
{
    if (!wasm_runtime_begin_blocking_op(exec_env)) {
        errno = EINTR;
        return -1;
    }
    int ret = os_socket_recv_from(sock, buf, len, flags, src_addr);
    wasm_runtime_end_blocking_op(exec_env);
    return ret;
}

int
blocking_op_socket_send_to(wasm_exec_env_t exec_env, bh_socket_t sock,
                           const void *buf, unsigned int len, int flags,
                           const bh_sockaddr_t *dest_addr)
{
    if (!wasm_runtime_begin_blocking_op(exec_env)) {
        errno = EINTR;
        return -1;
    }
    int ret = os_socket_send_to(sock, buf, len, flags, dest_addr);
    wasm_runtime_end_blocking_op(exec_env);
    return ret;
}

int
blocking_op_socket_addr_resolve(wasm_exec_env_t exec_env, const char *host,
                                const char *service, uint8_t *hint_is_tcp,
                                uint8_t *hint_is_ipv4,
                                bh_addr_info_t *addr_info,
                                size_t addr_info_size, size_t *max_info_size)
{
    /*
     * Note: Unlike others, os_socket_addr_resolve() is not a simple system
     * call. It's likely backed by a complex libc function, getaddrinfo().
     * Depending on the implementation of getaddrinfo() and underlying
     * DNS resolver, it might or might not be possible to make it return
     * with os_wakeup_blocking_op().
     *
     * Unfortunately, many of ISC/bind based resolvers just keep going on
     * interrupted system calls. It includes macOS and glibc.
     *
     * On the other hand, NuttX as of writing this returns EAI_AGAIN
     * on EINTR.
     */
    if (!wasm_runtime_begin_blocking_op(exec_env)) {
        errno = EINTR;
        return -1;
    }
    int ret = os_socket_addr_resolve(host, service, hint_is_tcp, hint_is_ipv4,
                                     addr_info, addr_info_size, max_info_size);
    wasm_runtime_end_blocking_op(exec_env);
    return ret;
}

int
blocking_op_openat(wasm_exec_env_t exec_env, int fd, const char *path,
                   int oflags, mode_t mode)
{
#ifdef BH_PLATFORM_WINDOWS
    errno = ENOTSUP;
    return -1;
#else
    if (!wasm_runtime_begin_blocking_op(exec_env)) {
        errno = EINTR;
        return -1;
    }
    int ret = openat(fd, path, oflags, mode);
    wasm_runtime_end_blocking_op(exec_env);
    return ret;
#endif
}
