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
    if (!wasm_runtime_begin_blocking_op(exec_env)) {
        errno = EINTR;
        return -1;
    }
    ssize_t ret = readv(fd, iov, iovcnt);
    wasm_runtime_end_blocking_op(exec_env);
    return ret;
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
#else  /* CONFIG_HAS_PREADV */
ssize_t
blocking_op_pread(wasm_exec_env_t exec_env, int fd, void *p, size_t nb,
                  off_t offset)
{
    if (!wasm_runtime_begin_blocking_op(exec_env)) {
        errno = EINTR;
        return -1;
    }
    ssize_t ret = pread(fd, p, nb, offset);
    wasm_runtime_end_blocking_op(exec_env);
    return ret;
}
#endif /* CONFIG_HAS_PREADV */

ssize_t
blocking_op_writev(wasm_exec_env_t exec_env, int fd, const struct iovec *iov,
                  int iovcnt)
{
    if (!wasm_runtime_begin_blocking_op(exec_env)) {
        errno = EINTR;
        return -1;
    }
    ssize_t ret = writev(fd, iov, iovcnt);
    wasm_runtime_end_blocking_op(exec_env);
    return ret;
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
#else  /* CONFIG_HAS_PREADV */
ssize_t
blocking_op_pwrite(wasm_exec_env_t exec_env, int fd, const void *p, size_t nb,
                  off_t offset)
{
    if (!wasm_runtime_begin_blocking_op(exec_env)) {
        errno = EINTR;
        return -1;
    }
    ssize_t ret = pwrite(fd, p, nb, offset);
    wasm_runtime_end_blocking_op(exec_env);
    return ret;
}
#endif /* CONFIG_HAS_PREADV */
