/*
 * Copyright (C) 2023 Midokura Japan KK.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"
#include "wasm_export.h"

int
blocking_op_close(wasm_exec_env_t exec_env, int fd);
ssize_t
blocking_op_readv(wasm_exec_env_t exec_env, int fd, const struct iovec *iov,
                  int iovcnt);
ssize_t
blocking_op_preadv(wasm_exec_env_t exec_env, int fd, const struct iovec *iov,
                   int iovcnt, off_t offset);
ssize_t
blocking_op_pread(wasm_exec_env_t exec_env, int fd, void *p, size_t nb,
                  off_t offset);
ssize_t
blocking_op_writev(wasm_exec_env_t exec_env, int fd, const struct iovec *iov,
                   int iovcnt);
ssize_t
blocking_op_pwritev(wasm_exec_env_t exec_env, int fd, const struct iovec *iov,
                    int iovcnt, off_t offset);
ssize_t
blocking_op_pwrite(wasm_exec_env_t exec_env, int fd, const void *p, size_t nb,
                   off_t offset);
int
blocking_op_socket_accept(wasm_exec_env_t exec_env, bh_socket_t server_sock,
                          bh_socket_t *sockp, void *addr,
                          unsigned int *addrlenp);
int
blocking_op_socket_connect(wasm_exec_env_t exec_env, bh_socket_t sock,
                           const char *addr, int port);
int
blocking_op_socket_recv_from(wasm_exec_env_t exec_env, bh_socket_t sock,
                             void *buf, unsigned int len, int flags,
                             bh_sockaddr_t *src_addr);
int
blocking_op_socket_send_to(wasm_exec_env_t exec_env, bh_socket_t sock,
                           const void *buf, unsigned int len, int flags,
                           const bh_sockaddr_t *dest_addr);
int
blocking_op_socket_addr_resolve(wasm_exec_env_t exec_env, const char *host,
                                const char *service, uint8_t *hint_is_tcp,
                                uint8_t *hint_is_ipv4,
                                bh_addr_info_t *addr_info,
                                size_t addr_info_size, size_t *max_info_size);
int
blocking_op_openat(wasm_exec_env_t exec_env, int fd, const char *path,
                   int oflags, mode_t mode);
