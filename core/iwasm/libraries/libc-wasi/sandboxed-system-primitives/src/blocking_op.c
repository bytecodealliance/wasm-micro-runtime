/*
 * Copyright (C) 2023 Midokura Japan KK.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <errno.h>

#include "ssp_config.h"
#include "blocking_op.h"

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
