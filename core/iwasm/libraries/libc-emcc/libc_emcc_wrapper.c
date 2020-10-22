/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_common.h"
#include "bh_log.h"
#include "wasm_export.h"
#include "../interpreter/wasm.h"

#define get_module_inst(exec_env) \
    wasm_runtime_get_module_inst(exec_env)

#define validate_native_addr(addr, size) \
    wasm_runtime_validate_native_addr(module_inst, addr, size)

#define module_malloc(size, p_native_addr) \
    wasm_runtime_module_malloc(module_inst, size, p_native_addr)

#define module_free(offset) \
    wasm_runtime_module_free(module_inst, offset)

#define REG_NATIVE_FUNC(func_name, signature)  \
    { #func_name, func_name##_wrapper, signature, NULL }

struct timespec_emcc {
    int tv_sec;
    int tv_nsec;
};

struct stat_emcc {
    unsigned st_dev;
    int __st_dev_padding;
    unsigned __st_ino_truncated;
    unsigned st_mode;
    unsigned st_nlink;
    unsigned st_uid;
    unsigned st_gid;
    unsigned st_rdev;
    int __st_rdev_padding;
    int64 st_size;
    int st_blksize;
    int st_blocks;
    struct timespec_emcc st_atim;
    struct timespec_emcc st_mtim;
    struct timespec_emcc st_ctim;
    int64 st_ino;
};

static int
open_wrapper(wasm_exec_env_t exec_env, const char *pathname,
             int flags, int mode)
{
    if (pathname == NULL)
        return -1;
    return open(pathname, flags, mode);
}

static int
__sys_read_wrapper(wasm_exec_env_t exec_env,
                   int fd, void *buf, uint32 count)
{
    return read(fd, buf, count);
}

static void
statbuf_native2app(const struct stat *statbuf_native,
                   struct stat_emcc *statbuf_app)
{
    statbuf_app->st_dev = (unsigned)statbuf_native->st_dev;
    statbuf_app->__st_ino_truncated = (unsigned)statbuf_native->st_ino;
    statbuf_app->st_mode = (unsigned)statbuf_native->st_mode;
    statbuf_app->st_nlink = (unsigned)statbuf_native->st_nlink;
    statbuf_app->st_uid = (unsigned)statbuf_native->st_uid;
    statbuf_app->st_gid = (unsigned)statbuf_native->st_gid;
    statbuf_app->st_rdev = (unsigned)statbuf_native->st_rdev;
    statbuf_app->st_size = (int64)statbuf_native->st_size;
    statbuf_app->st_blksize = (unsigned)statbuf_native->st_blksize;
    statbuf_app->st_blocks = (unsigned)statbuf_native->st_blocks;
    statbuf_app->st_ino = (int64)statbuf_native->st_ino;
    statbuf_app->st_atim.tv_sec = (int)statbuf_native->st_atim.tv_sec;
    statbuf_app->st_atim.tv_nsec = (int)statbuf_native->st_atim.tv_nsec;
    statbuf_app->st_mtim.tv_sec = (int)statbuf_native->st_mtim.tv_sec;
    statbuf_app->st_mtim.tv_nsec = (int)statbuf_native->st_mtim.tv_nsec;
    statbuf_app->st_ctim.tv_sec = (int)statbuf_native->st_ctim.tv_sec;
    statbuf_app->st_ctim.tv_nsec = (int)statbuf_native->st_ctim.tv_nsec;
}

static int
__sys_stat64_wrapper(wasm_exec_env_t exec_env,
                     const char *pathname,
                     struct stat_emcc *statbuf_app)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    int ret;
    struct stat statbuf;

    if (!validate_native_addr((void*)statbuf_app, sizeof(struct stat_emcc)))
        return -1;

    if (pathname == NULL)
        return -1;

    ret = stat(pathname, &statbuf);
    if (ret == 0)
        statbuf_native2app(&statbuf, statbuf_app);
    return ret;
}

static int
__sys_fstat64_wrapper(wasm_exec_env_t exec_env,
                      int fd, struct stat_emcc *statbuf_app)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    int ret;
    struct stat statbuf;

    if (!validate_native_addr((void*)statbuf_app, sizeof(struct stat_emcc)))
        return -1;

    if (fd <= 0)
        return -1;

    ret = fstat(fd, &statbuf);
    if (ret == 0)
        statbuf_native2app(&statbuf, statbuf_app);
    return ret;
}

static int
mmap_wrapper(wasm_exec_env_t exec_env,
             void *addr, int length, int prot, int flags,
             int fd, int64 offset)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    uint32 buf_offset;
    char *buf;
    int size_read;

    buf_offset = module_malloc(length, (void**)&buf);
    if (buf_offset == 0)
        return -1;

    if (fd <= 0)
        return -1;

    if (lseek(fd, offset, SEEK_SET) == -1)
        return -1;

    size_read = read(fd, buf, length);
    (void)size_read;
    return buf_offset;
}

static int
munmap_wrapper(wasm_exec_env_t exec_env, uint32 buf_offset, int length)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    module_free(buf_offset);
    return 0;
}

static int
__munmap_wrapper(wasm_exec_env_t exec_env, uint32 buf_offset, int length)
{
    return munmap_wrapper(exec_env, buf_offset, length);
}

static int
getentropy_wrapper(wasm_exec_env_t exec_env, void *buffer, uint32 length)
{
    if (buffer == NULL)
        return -1;
    return getentropy(buffer, length);
}

#define REG_NATIVE_FUNC(func_name, signature)  \
    { #func_name, func_name##_wrapper, signature, NULL }

static NativeSymbol native_symbols_libc_emcc[] = {
    REG_NATIVE_FUNC(open, "($ii)i"),
    REG_NATIVE_FUNC(__sys_read, "(i*~)i"),
    REG_NATIVE_FUNC(__sys_stat64, "($*)i"),
    REG_NATIVE_FUNC(__sys_fstat64, "(i*)i"),
    REG_NATIVE_FUNC(mmap, "(*iiiiI)i"),
    REG_NATIVE_FUNC(munmap, "(ii)i"),
    REG_NATIVE_FUNC(__munmap, "(ii)i"),
    REG_NATIVE_FUNC(getentropy, "(*~)i"),
};

uint32
get_libc_emcc_export_apis(NativeSymbol **p_libc_emcc_apis)
{
    *p_libc_emcc_apis = native_symbols_libc_emcc;
    return sizeof(native_symbols_libc_emcc) / sizeof(NativeSymbol);
}
