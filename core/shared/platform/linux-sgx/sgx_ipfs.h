/*
 * Copyright (C) 2022 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _LIBC_WASI_SGX_PFS_H
#define _LIBC_WASI_SGX_PFS_H

#include "wasmtime_ssp.h"

#ifdef __cplusplus
extern "C" {
#endif

int
ipfs_posix_fallocate(void *sgx_file, off_t offset, size_t len);
size_t
ipfs_readv(void *sgx_file, const struct iovec *iov, int iovcnt);
size_t
ipfs_pread(void *sgx_file, void *buffer, size_t size, off_t offset);
size_t
ipfs_writev(void *sgx_file, const struct iovec *iov, int iovcnt);
size_t
ipfs_pwrite(void *sgx_file, const void *buffer, size_t size, off_t offset);
int
ipfs_close(void *sgx_file);
void *
ipfs_fopen(const char *filename, __wasi_rights_t fs_rights_base,
           __wasi_oflags_t oflags, __wasi_fdflags_t fs_flags);
int
ipfs_fflush(void *sgx_file);
off_t
ipfs_lseek(void *sgx_file, off_t offset, int nwhence);
off_t
ipfs_ftell(void *sgx_file);
int
ipfs_ftruncate(void *sgx_file, off_t len);

#ifdef __cplusplus
}
#endif

#endif /* end of _LIBC_WASI_SGX_PFS_H */