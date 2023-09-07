/*
 * Copyright (C) 2023 Midokura Japan KK.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <sys/types.h>
#include <sys/uio.h>

#include <unistd.h>

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
