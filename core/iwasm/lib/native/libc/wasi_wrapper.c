/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_native.h"
#include "wasm_export.h"
#include "wasm_log.h"
#include "wasm_platform_log.h"
#if WASM_ENABLE_WASI != 0
#include "wasi_wrapper.h"
#endif
#include "bh_common.h"

void
wasm_runtime_set_exception(wasm_module_inst_t module, const char *exception);

#define get_wasi_ctx(module_inst) \
    wasm_runtime_get_wasi_ctx(module_inst)

#define validate_app_addr(offset, size) \
    wasm_runtime_validate_app_addr(module_inst, offset, size)

#define addr_app_to_native(offset) \
    wasm_runtime_addr_app_to_native(module_inst, offset)

#define addr_native_to_app(ptr) \
    wasm_runtime_addr_native_to_app(module_inst, ptr)

#define module_malloc(size) \
    wasm_runtime_module_malloc(module_inst, size)

#define module_free(offset) \
    wasm_runtime_module_free(module_inst, offset)

#if WASM_ENABLE_WASI != 0

#define WASI_CHECK_ERR() do {   \
        if (err) {              \
            return err;         \
        }                       \
    } while (0)

typedef struct wasi_prestat_app {
    wasi_preopentype_t pr_type;
    uint32 pr_name_len;
} wasi_prestat_app_t;

typedef struct iovec_app {
    int32 buf_offset;
    uint32 buf_len;
} iovec_app_t;

typedef struct WASIContext {
    void *curfds;
    void *prestats;
    void *argv_environ;
} *wasi_ctx_t;

wasi_ctx_t
wasm_runtime_get_wasi_ctx(wasm_module_inst_t module_inst);

static wasi_errno_t
wasi_args_get(wasm_module_inst_t module_inst,
              int32 argv_offset /* char ** */,
              int32 argv_buf_offset /* char * */)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    size_t argc, argv_buf_size, i;
    uint64 total_size1, total_size2;
    int32 *argv_app;
    char *argv_buf_app;
    wasi_errno_t err;
    char **argv;

    err = wasmtime_ssp_args_sizes_get(wasi_ctx->argv_environ,
                                      &argc, &argv_buf_size);
    WASI_CHECK_ERR();

    total_size1 = sizeof(char *) * ((uint64)argc + 1);
    total_size2 = sizeof(char) * (uint64)argv_buf_size;
    if (total_size1 >= UINT32_MAX
        || !validate_app_addr(argv_offset, (uint32)total_size1)
        || total_size2 >= UINT32_MAX
        || !validate_app_addr(argv_buf_offset, (uint32)total_size2))
        return (wasi_errno_t)-1;

    argv = bh_malloc((uint32)total_size1);
    if (!argv)
        return (wasi_errno_t)-1;

    argv_app = (int32*)addr_app_to_native(argv_offset);
    argv_buf_app= (char*)addr_app_to_native(argv_buf_offset);

    err = wasmtime_ssp_args_get(wasi_ctx->argv_environ,
                                argv, argv_buf_app);
    if (err)
        goto fail;

    for (i = 0; i < argc; i++)
        argv_app[i] = addr_native_to_app(argv[i]);
    argv_app[argc] = 0;

    /* success */
    err = 0;

fail:
    bh_free(argv);
    return err;
}

static wasi_errno_t
wasi_args_sizes_get(wasm_module_inst_t module_inst,
                    int32 argc_offset /* size_t * */,
                    int32 argv_buf_size_offset /* size_t * */)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    size_t argc, argv_buf_size;
    uint32 *argc_app, *argv_buf_size_app;
    wasi_errno_t err;

    if (!validate_app_addr(argc_offset, sizeof(uint32))
        || !validate_app_addr(argv_buf_size_offset, sizeof(uint32)))
        return (wasi_errno_t)-1;

    argc_app = (uint32*)addr_app_to_native(argc_offset);
    argv_buf_size_app = (uint32*)addr_app_to_native(argv_buf_size_offset);

    err = wasmtime_ssp_args_sizes_get(wasi_ctx->argv_environ,
                                      &argc, &argv_buf_size);
    WASI_CHECK_ERR();

    *(uint32*)argc_app = (uint32)argc;
    *(uint32*)argv_buf_size_app = (uint32)argv_buf_size;

    return 0;
}

static wasi_errno_t
wasi_clock_res_get(wasm_module_inst_t module_inst,
                   wasi_clockid_t clock_id,
                   int32 resolution_offset /* wasi_timestamp_t * */)
{
    wasi_timestamp_t resolution;
    uint32 *resolution_app;
    wasi_errno_t err;

    if (!validate_app_addr(resolution_offset, sizeof(wasi_timestamp_t)))
        return (wasi_errno_t)-1;

    err = wasmtime_ssp_clock_res_get(clock_id, &resolution);
    WASI_CHECK_ERR();

    resolution_app = addr_app_to_native(resolution_offset);

    memcpy(resolution_app, &resolution, sizeof(wasi_timestamp_t));

    return 0;
}

static wasi_errno_t
wasi_clock_time_get(wasm_module_inst_t module_inst,
                    wasi_clockid_t clock_id,
                    wasi_timestamp_t precision,
                    int32 time_offset /*wasi_timestamp_t * */)
{
    wasi_timestamp_t time;
    uint32 *time_app;
    wasi_errno_t err;

    if (!validate_app_addr(time_offset, sizeof(wasi_timestamp_t)))
        return (wasi_errno_t)-1;

    err = wasmtime_ssp_clock_time_get(clock_id, precision, &time);
    WASI_CHECK_ERR();

    time_app = addr_app_to_native(time_offset);

    memcpy(time_app, &time, sizeof(wasi_timestamp_t));

    return 0;
}

static wasi_errno_t
wasi_environ_get(wasm_module_inst_t module_inst,
                 int32 environ_offset /* char ** */,
                 int32 environ_buf_offset /* char */)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    size_t environ_count, environ_buf_size, i;
    uint64 total_size;
    int32 *environ_app;
    char *environ_buff_app;
    char **environ;
    wasi_errno_t err;

    err = wasmtime_ssp_environ_sizes_get(wasi_ctx->argv_environ,
                                         &environ_count, &environ_buf_size);
    WASI_CHECK_ERR();

    total_size = sizeof(uint32) * ((uint64)environ_count + 1);
    if (total_size >= UINT32_MAX
        || !validate_app_addr(environ_offset, (uint32)total_size)
        || environ_buf_size >= UINT32_MAX
        || !validate_app_addr(environ_buf_offset, (uint32)environ_buf_size))
        return (wasi_errno_t)-1;

    environ_app = (int32*)addr_app_to_native(environ_offset);
    environ_buff_app = (char*)addr_app_to_native(environ_buf_offset);

    environ = bh_malloc((uint32)total_size);
    if (!environ)
        return (wasi_errno_t)-1;

    err = wasmtime_ssp_environ_get(wasi_ctx->argv_environ,
                                   environ, environ_buff_app);
    if (err)
        goto fail;

    for (i = 0; i < environ_count; i++)
        environ_app[i] = addr_native_to_app(environ[i]);
    environ_app[environ_count] = 0;

    /* success */
    err = 0;

fail:
    bh_free(environ);
    return err;
}

static wasi_errno_t
wasi_environ_sizes_get(wasm_module_inst_t module_inst,
                       int32 environ_count_offset /* size_t * */,
                       int32 environ_buf_size_offset /* size_t * */)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    size_t environ_count, environ_buf_size;
    uint32 *environ_count_app, *environ_buf_size_app;
    wasi_errno_t err;

    if (!validate_app_addr(environ_count_offset, sizeof(uint32))
        || !validate_app_addr(environ_buf_size_offset, sizeof(uint32)))
        return (wasi_errno_t)-1;

    err = wasmtime_ssp_environ_sizes_get(wasi_ctx->argv_environ,
                                         &environ_count, &environ_buf_size);
    WASI_CHECK_ERR();

    environ_count_app = (uint32*)addr_app_to_native(environ_count_offset);
    environ_buf_size_app = (uint32*)addr_app_to_native(environ_buf_size_offset);

    *(uint32*)environ_count_app = (uint32)environ_count;
    *(uint32*)environ_buf_size_app = (uint32)environ_buf_size;

    return 0;
}

static wasi_errno_t
wasi_fd_prestat_get(wasm_module_inst_t module_inst,
                    wasi_fd_t fd,
                    int32 buf_offset /* wasi_prestat_t * */)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    wasi_prestat_app_t *prestat_app;
    wasi_prestat_t prestat;
    wasi_errno_t err;

    if (!validate_app_addr(buf_offset, sizeof(wasi_prestat_app_t)))
        return (wasi_errno_t)-1;

    err = wasmtime_ssp_fd_prestat_get(wasi_ctx->prestats, fd, &prestat);
    WASI_CHECK_ERR();

    prestat_app = (wasi_prestat_app_t*)addr_app_to_native(buf_offset);
    prestat_app->pr_type = prestat.pr_type;
    prestat_app->pr_name_len = (uint32)prestat.u.dir.pr_name_len;

    return 0;
}

static wasi_errno_t
wasi_fd_prestat_dir_name(wasm_module_inst_t module_inst,
                         wasi_fd_t fd,
                         int32 path_offset /* char * */,
                         uint32 path_len)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    char *path_app;

    if (!validate_app_addr(path_offset, path_len))
        return (wasi_errno_t)-1;

    path_app = (char*)addr_app_to_native(path_offset);
    return wasmtime_ssp_fd_prestat_dir_name(wasi_ctx->prestats, fd,
                                            path_app, path_len);
}

static wasi_errno_t
wasi_fd_close(wasm_module_inst_t module_inst, wasi_fd_t fd)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);

    return wasmtime_ssp_fd_close(wasi_ctx->curfds, wasi_ctx->prestats, fd);
}

static wasi_errno_t
wasi_fd_datasync(wasm_module_inst_t module_inst, wasi_fd_t fd)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);

    return wasmtime_ssp_fd_datasync(wasi_ctx->curfds, fd);
}

static wasi_errno_t
wasi_fd_pread(wasm_module_inst_t module_inst,
              wasi_fd_t fd,
              int32 iovs_offset /* const wasi_iovec_t * */,
              uint32 iovs_len,
              wasi_filesize_t offset,
              int32 nread_offset /* size_t * */)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    int32 mem;
    wasi_iovec_t *iovec, *iovec_begin;
    iovec_app_t *iovec_app;
    uint32 i;
    size_t nread;
    uint32 *nread_app;
    uint64 total_size;
    wasi_errno_t err;

    total_size = sizeof(iovec_app_t) * (uint64)iovs_len;
    if (!validate_app_addr(nread_offset, (uint32)sizeof(uint32))
        || total_size >= UINT32_MAX
        || !validate_app_addr(iovs_offset, (uint32)total_size))
        return (wasi_errno_t)-1;

    iovec_app = (iovec_app_t*)addr_app_to_native(iovs_offset);
    if (!iovec_app)
        return (wasi_errno_t)-1;

    total_size = sizeof(wasi_iovec_t) * (uint64)iovs_len;
    if (total_size >= UINT32_MAX
        || !(mem = module_malloc((uint32)total_size)))
        return (wasi_errno_t)-1;

    iovec = iovec_begin = (wasi_iovec_t*)addr_app_to_native(mem);

    for (i = 0; i < iovs_len; i++, iovec_app++, iovec++) {
        if (!validate_app_addr(iovec_app->buf_offset, iovec_app->buf_len)) {
            err = (wasi_errno_t)-1;
            goto fail;
        }
        iovec->buf = (void*)addr_app_to_native(iovec_app->buf_offset);
        iovec->buf_len = iovec_app->buf_len;
    }

    err = wasmtime_ssp_fd_pread(wasi_ctx->curfds, fd, iovec_begin,
                                iovs_len, offset, &nread);
    if (err)
        goto fail;

    nread_app = (uint32*)addr_app_to_native(nread_offset);
    *(uint32*)nread_app = (uint32)nread;

    /* success */
    err = 0;

fail:
    module_free(mem);
    return err;
}

static wasi_errno_t
wasi_fd_pwrite(wasm_module_inst_t module_inst,
               wasi_fd_t fd,
               int32 iovs_offset /* const wasi_ciovec_t * */,
               uint32 iovs_len,
               wasi_filesize_t offset,
               int32 nwritten_offset /* size_t * */)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    int32 mem;
    wasi_ciovec_t *ciovec, *ciovec_begin;
    iovec_app_t *ciovec_app;
    uint32 i;
    size_t nwritten;
    uint32 *nwritten_app;
    uint64 total_size;
    wasi_errno_t err;

    total_size = sizeof(iovec_app_t) * (uint64)iovs_len;
    if (!validate_app_addr(nwritten_offset, (uint32)sizeof(uint32))
        || total_size >= UINT32_MAX
        || !validate_app_addr(iovs_offset, (uint32)total_size))
        return (wasi_errno_t)-1;

    ciovec_app = (iovec_app_t*)addr_app_to_native(iovs_offset);

    total_size = sizeof(wasi_ciovec_t) * (uint64)iovs_len;
    if (total_size >= UINT32_MAX
        || !(mem = module_malloc((uint32)total_size)))
        return (wasi_errno_t)-1;

    ciovec_begin = ciovec = (wasi_ciovec_t*)addr_app_to_native(mem);
    for (i = 0; i < iovs_len; i++, ciovec_app++, ciovec++) {
        if (!validate_app_addr(ciovec_app->buf_offset, ciovec_app->buf_len)) {
            err = (wasi_errno_t)-1;
            goto fail;
        }
        ciovec->buf = (char*)addr_app_to_native(ciovec_app->buf_offset);
        ciovec->buf_len = ciovec_app->buf_len;
    }

    err = wasmtime_ssp_fd_pwrite(wasi_ctx->curfds, fd, ciovec_begin,
                                 iovs_len, offset, &nwritten);
    if (err)
        goto fail;

    nwritten_app = (uint32*)addr_app_to_native(nwritten_offset);
    *(uint32*)nwritten_app = (uint32)nwritten;

    /* success */
    err = 0;

fail:
    module_free(mem);
    return err;
}

static wasi_errno_t
wasi_fd_read(wasm_module_inst_t module_inst,
             wasi_fd_t fd,
             int32 iovs_offset /* const wasi_iovec_t * */,
             uint32 iovs_len,
             int32 nread_offset /* size_t * */)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    int32 mem;
    wasi_iovec_t *iovec, *iovec_begin;
    iovec_app_t *iovec_app;
    uint32 i;
    size_t nread;
    uint32 *nread_app;
    uint64 total_size;
    wasi_errno_t err;

    total_size = sizeof(iovec_app_t) * (uint64)iovs_len;
    if (!validate_app_addr(nread_offset, (uint32)sizeof(uint32))
        || total_size >= UINT32_MAX
        || !validate_app_addr(iovs_offset, (uint32)total_size))
        return (wasi_errno_t)-1;

    iovec_app = (iovec_app_t*)addr_app_to_native(iovs_offset);

    total_size = sizeof(wasi_iovec_t) * (uint64)iovs_len;
    if (total_size >= UINT32_MAX
        || !(mem = module_malloc((uint32)total_size)))
        return (wasi_errno_t)-1;

    iovec = iovec_begin = (wasi_iovec_t*)addr_app_to_native(mem);

    for (i = 0; i < iovs_len; i++, iovec_app++, iovec++) {
        if (!validate_app_addr(iovec_app->buf_offset, iovec_app->buf_len)) {
            err = (wasi_errno_t)-1;
            goto fail;
        }
        iovec->buf = (void*)addr_app_to_native(iovec_app->buf_offset);
        iovec->buf_len = iovec_app->buf_len;
    }

    err = wasmtime_ssp_fd_read(wasi_ctx->curfds, fd,
                               iovec_begin, iovs_len, &nread);
    if (err)
        goto fail;

    nread_app = (uint32*)addr_app_to_native(nread_offset);
    *(uint32*)nread_app = (uint32)nread;

    /* success */
    err = 0;

fail:
    module_free(mem);
    return err;
}

static wasi_errno_t
wasi_fd_renumber(wasm_module_inst_t module_inst,
                 wasi_fd_t from, wasi_fd_t to)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);

    return wasmtime_ssp_fd_renumber(wasi_ctx->curfds,
                                    wasi_ctx->prestats, from, to);
}

static wasi_errno_t
wasi_fd_seek(wasm_module_inst_t module_inst,
             wasi_fd_t fd,
             wasi_filedelta_t offset,
             wasi_whence_t whence,
             int32 newoffset_offset /* wasi_filesize_t * */)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    size_t newoffset;
    uint64 *newoffset_app;
    wasi_errno_t err;

    if (!validate_app_addr(newoffset_offset, sizeof(uint64)))
        return (wasi_errno_t)-1;

    err = wasmtime_ssp_fd_seek(wasi_ctx->curfds, fd,
                               offset, whence, &newoffset);
    WASI_CHECK_ERR();

    newoffset_app = (uint64*)addr_app_to_native(newoffset_offset);
    *(uint64*)newoffset_app = (uint64)newoffset;

    return 0;
}

static wasi_errno_t
wasi_fd_tell(wasm_module_inst_t module_inst,
             wasi_fd_t fd,
             int32 newoffset_offset /* wasi_filesize_t * */)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    size_t newoffset;
    uint64 *newoffset_app;
    wasi_errno_t err;

    if (!validate_app_addr(newoffset_offset, sizeof(uint64)))
        return (wasi_errno_t)-1;

    err = wasmtime_ssp_fd_tell(wasi_ctx->curfds, fd, &newoffset);
    WASI_CHECK_ERR();

    newoffset_app = (uint64*)addr_app_to_native(newoffset_offset);
    *(uint64*)newoffset_app = (uint64)newoffset;

    return 0;
}

static wasi_errno_t
wasi_fd_fdstat_get(wasm_module_inst_t module_inst,
                   wasi_fd_t fd,
                   int32 buf_offset /* wasi_fdstat_t * */)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    wasi_fdstat_t fdstat, *fdstat_app;
    wasi_errno_t err;

    if (!validate_app_addr(buf_offset, sizeof(wasi_fdstat_t)))
        return (wasi_errno_t)-1;

    err = wasmtime_ssp_fd_fdstat_get(wasi_ctx->curfds, fd, &fdstat);
    WASI_CHECK_ERR();

    fdstat_app = (wasi_fdstat_t*)addr_app_to_native(buf_offset);
    memcpy(fdstat_app, &fdstat, sizeof(wasi_fdstat_t));

    return 0;
}

static wasi_errno_t
wasi_fd_fdstat_set_flags(wasm_module_inst_t module_inst,
                         wasi_fd_t fd,
                         wasi_fdflags_t flags)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);

    return wasmtime_ssp_fd_fdstat_set_flags(wasi_ctx->curfds, fd, flags);

}

static wasi_errno_t
wasi_fd_fdstat_set_rights(wasm_module_inst_t module_inst,
                          wasi_fd_t fd,
                          wasi_rights_t fs_rights_base,
                          wasi_rights_t fs_rights_inheriting)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);

    return wasmtime_ssp_fd_fdstat_set_rights(wasi_ctx->curfds, fd,
                                             fs_rights_base, fs_rights_inheriting);
}

static wasi_errno_t
wasi_fd_sync(wasm_module_inst_t module_inst, wasi_fd_t fd)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);

    return wasmtime_ssp_fd_sync(wasi_ctx->curfds, fd);
}

static wasi_errno_t
wasi_fd_write(wasm_module_inst_t module_inst,
              wasi_fd_t fd,
              int32 iovs_offset /* const wasi_ciovec_t * */,
              uint32 iovs_len,
              int32 nwritten_offset /* size_t * */)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    int32 mem;
    wasi_ciovec_t *ciovec, *ciovec_begin;
    iovec_app_t *ciovec_app;
    uint32 i;
    size_t nwritten;
    uint32 *nwritten_app;
    uint64 total_size;
    wasi_errno_t err;

    total_size = sizeof(iovec_app_t) * (uint64)iovs_len;
    if (!validate_app_addr(nwritten_offset, (uint32)sizeof(uint32))
        || total_size >= UINT32_MAX
        || !validate_app_addr(iovs_offset, (uint32)total_size))
        return (wasi_errno_t)-1;

    ciovec_app = (iovec_app_t*)addr_app_to_native(iovs_offset);

    total_size = sizeof(wasi_ciovec_t) * (uint64)iovs_len;
    if (total_size >= UINT32_MAX
        || !(mem = module_malloc((uint32)total_size)))
        return (wasi_errno_t)-1;

    ciovec_begin = ciovec = (wasi_ciovec_t*)addr_app_to_native(mem);
    for (i = 0; i < iovs_len; i++, ciovec_app++, ciovec++) {
        if (!validate_app_addr(ciovec_app->buf_offset, ciovec_app->buf_len)) {
            err = (wasi_errno_t)-1;
            goto fail;
        }
        ciovec->buf = (char*)addr_app_to_native(ciovec_app->buf_offset);
        ciovec->buf_len = ciovec_app->buf_len;
    }

    err = wasmtime_ssp_fd_write(wasi_ctx->curfds, fd,
                                ciovec_begin, iovs_len, &nwritten);
    if (err)
        goto fail;

    nwritten_app = (uint32*)addr_app_to_native(nwritten_offset);
    *(uint32*)nwritten_app = (uint32)nwritten;

    /* success */
    err = 0;

fail:
    module_free(mem);
    return err;
}

static wasi_errno_t
wasi_fd_advise(wasm_module_inst_t module_inst,
               wasi_fd_t fd,
               wasi_filesize_t offset,
               wasi_filesize_t len,
               wasi_advice_t advice)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);

    return wasmtime_ssp_fd_advise(wasi_ctx->curfds, fd, offset, len, advice);
}

static wasi_errno_t
wasi_fd_allocate(wasm_module_inst_t module_inst,
                 wasi_fd_t fd,
                 wasi_filesize_t offset,
                 wasi_filesize_t len)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);

    return wasmtime_ssp_fd_allocate(wasi_ctx->curfds, fd, offset, len);
}

static wasi_errno_t
wasi_path_create_directory(wasm_module_inst_t module_inst,
                           wasi_fd_t fd,
                           int32 path_offset /* const char * */,
                           uint32 path_len)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    char *path;

    if (!validate_app_addr(path_offset, path_len))
        return (wasi_errno_t)-1;

    path = (char*)addr_app_to_native(path_offset);

    return wasmtime_ssp_path_create_directory(wasi_ctx->curfds, fd,
                                              path, path_len);
}

static wasi_errno_t
wasi_path_link(wasm_module_inst_t module_inst,
               wasi_fd_t old_fd,
               wasi_lookupflags_t old_flags,
               int32 old_path_offset /* const char * */,
               uint32 old_path_len,
               wasi_fd_t new_fd,
               int32 new_path_offset /* const char * */,
               uint32 new_path_len)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    char *old_path, *new_path;

    if (!validate_app_addr(old_path_offset, old_path_len)
        || !validate_app_addr(new_path_offset, new_path_len))
        return (wasi_errno_t)-1;

    old_path = (char*)addr_app_to_native(old_path_offset);
    new_path = (char*)addr_app_to_native(new_path_offset);

    return wasmtime_ssp_path_link(wasi_ctx->curfds, wasi_ctx->prestats,
                                  old_fd, old_flags, old_path, old_path_len,
                                  new_fd, new_path, new_path_len);
}

static wasi_errno_t
wasi_path_open(wasm_module_inst_t module_inst,
               wasi_fd_t dirfd,
               wasi_lookupflags_t dirflags,
               int32 path_offset /* const char * */,
               uint32 path_len,
               wasi_oflags_t oflags,
               wasi_rights_t fs_rights_base,
               wasi_rights_t fs_rights_inheriting,
               wasi_fdflags_t fs_flags,
               int32 fd_offset /* wasi_fd_t * */)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    char *path;
    wasi_fd_t fd = (wasi_fd_t)-1;
    uint32 *fd_app;
    wasi_errno_t err;

    if (!validate_app_addr(path_offset, path_len)
        || !validate_app_addr(fd_offset, 1))
        return (wasi_errno_t)-1;

    path = (char*)addr_app_to_native(path_offset);

    err = wasmtime_ssp_path_open(wasi_ctx->curfds,
                                 dirfd, dirflags,
                                 path, path_len,
                                 oflags,
                                 fs_rights_base,
                                 fs_rights_inheriting,
                                 fs_flags,
                                 &fd);

    fd_app = (wasi_fd_t*)addr_app_to_native(fd_offset);
    *fd_app = (uint32)fd;

    WASI_CHECK_ERR();

    return 0;
}

static wasi_errno_t
wasi_fd_readdir(wasm_module_inst_t module_inst,
                wasi_fd_t fd,
                int32 buf_offset /* void *buf */,
                uint32 buf_len,
                wasi_dircookie_t cookie,
                int32 bufused_offset /* size_t * */)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    void *buf;
    size_t bufused;
    uint32 *bufused_app;
    wasi_errno_t err;

    if (!validate_app_addr(buf_offset, buf_len)
        || !validate_app_addr(bufused_offset, sizeof(uint32)))
        return (wasi_errno_t)-1;

    buf = (void*)addr_app_to_native(buf_offset);

    err = wasmtime_ssp_fd_readdir(wasi_ctx->curfds, fd,
                                  buf, buf_len, cookie, &bufused);
    WASI_CHECK_ERR();

    bufused_app = (uint32*)addr_app_to_native(bufused_offset);
    *(uint32*)bufused_app = (uint32)bufused;

    return 0;
}

static wasi_errno_t
wasi_path_readlink(wasm_module_inst_t module_inst,
                   wasi_fd_t fd,
                   int32 path_offset /* const char * */,
                   uint32 path_len,
                   int32 buf_offset /* char * */,
                   uint32 buf_len,
                   int32 bufused_offset /* size_t * */)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    char *path, *buf;
    size_t bufused;
    uint32 *bufused_app;
    wasi_errno_t err;

    if (!validate_app_addr(path_offset, path_len)
        || !validate_app_addr(buf_offset, buf_len)
        || !validate_app_addr(bufused_offset, sizeof(uint32)))
        return (wasi_errno_t)-1;

    path = (char*)addr_app_to_native(path_offset);
    buf = (char*)addr_app_to_native(buf_offset);

    err = wasmtime_ssp_path_readlink(wasi_ctx->curfds, fd,
                                     path, path_len, buf,
                                     buf_len, &bufused);
    WASI_CHECK_ERR();

    bufused_app = (uint32*)addr_app_to_native(bufused_offset);
    *(uint32*)bufused_app = (uint32)bufused;

    return 0;
}

static wasi_errno_t
wasi_path_rename(wasm_module_inst_t module_inst,
                 wasi_fd_t old_fd,
                 int32 old_path_offset /* const char * */,
                 uint32 old_path_len,
                 wasi_fd_t new_fd,
                 int32 new_path_offset /* const char * */,
                 uint32 new_path_len)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    char *old_path, *new_path;

    if (!validate_app_addr(old_path_offset, old_path_len)
        || !validate_app_addr(new_path_offset, new_path_len))
        return (wasi_errno_t)-1;

    old_path = (char*)addr_app_to_native(old_path_offset);
    new_path = (char*)addr_app_to_native(new_path_offset);

    return wasmtime_ssp_path_rename(wasi_ctx->curfds, old_fd,
                                    old_path, old_path_len,
                                    new_fd, new_path,
                                    new_path_len);
}

static wasi_errno_t
wasi_fd_filestat_get(wasm_module_inst_t module_inst,
                     wasi_fd_t fd,
                     int32 buf_offset /* wasi_filestat_t * */)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    wasi_filestat_t filestat, *filestat_app;
    wasi_errno_t err;

    if (!validate_app_addr(buf_offset, sizeof(wasi_filestat_t)))
        return (wasi_errno_t)-1;

    err = wasmtime_ssp_fd_filestat_get(wasi_ctx->curfds, fd, &filestat);
    WASI_CHECK_ERR();

    filestat_app = (wasi_filestat_t*)addr_app_to_native(buf_offset);
    memcpy(filestat_app, &filestat, sizeof(wasi_filestat_t));

    return 0;
}

static wasi_errno_t
wasi_fd_filestat_set_times(wasm_module_inst_t module_inst,
                           wasi_fd_t fd,
                           wasi_timestamp_t st_atim,
                           wasi_timestamp_t st_mtim,
                           wasi_fstflags_t fstflags)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);

    return wasmtime_ssp_fd_filestat_set_times(wasi_ctx->curfds, fd,
                                              st_atim, st_mtim, fstflags);
}

static wasi_errno_t
wasi_fd_filestat_set_size(wasm_module_inst_t module_inst,
                          wasi_fd_t fd,
                          wasi_filesize_t st_size)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);

    return wasmtime_ssp_fd_filestat_set_size(wasi_ctx->curfds, fd, st_size);
}

static wasi_errno_t
wasi_path_filestat_get(wasm_module_inst_t module_inst,
                       wasi_fd_t fd,
                       wasi_lookupflags_t flags,
                       int32 path_offset /* const char * */,
                       uint32 path_len,
                       int32 buf_offset /* wasi_filestat_t * */)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    char *path;
    wasi_filestat_t filestat, *filestat_app;
    wasi_errno_t err;

    if (!validate_app_addr(path_offset, path_len)
        || !validate_app_addr(buf_offset, sizeof(wasi_filestat_t)))
        return (wasi_errno_t)-1;

    path = (char*)addr_app_to_native(path_offset);

    err = wasmtime_ssp_path_filestat_get(wasi_ctx->curfds, fd,
                                         flags, path, path_len, &filestat);
    WASI_CHECK_ERR();

    filestat_app = (wasi_filestat_t*)addr_app_to_native(buf_offset);
    memcpy(filestat_app, &filestat, sizeof(wasi_filestat_t));

    return 0;
}

static wasi_errno_t
wasi_path_filestat_set_times(wasm_module_inst_t module_inst,
                             wasi_fd_t fd,
                             wasi_lookupflags_t flags,
                             int32 path_offset /* const char * */,
                             uint32 path_len,
                             wasi_timestamp_t st_atim,
                             wasi_timestamp_t st_mtim,
                             wasi_fstflags_t fstflags)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    char *path;

    if (!validate_app_addr(path_offset, path_len))
        return (wasi_errno_t)-1;

    path = (char*)addr_app_to_native(path_offset);

    return wasmtime_ssp_path_filestat_set_times(wasi_ctx->curfds, fd,
                                                flags, path, path_len,
                                                st_atim, st_mtim, fstflags);
}

static wasi_errno_t
wasi_path_symlink(wasm_module_inst_t module_inst,
                  int32 old_path_offset /* const char * */,
                  uint32 old_path_len,
                  wasi_fd_t fd,
                  int32 new_path_offset /* const char * */,
                  uint32 new_path_len)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    char *old_path, *new_path;

    if (!validate_app_addr(old_path_offset, old_path_len)
        || !validate_app_addr(new_path_offset, new_path_len))
        return (wasi_errno_t)-1;

    old_path = (char*)addr_app_to_native(old_path_offset);
    new_path = (char*)addr_app_to_native(new_path_offset);

    return wasmtime_ssp_path_symlink(wasi_ctx->curfds, wasi_ctx->prestats,
                                     old_path, old_path_len, fd, new_path,
                                     new_path_len);
}

static wasi_errno_t
wasi_path_unlink_file(wasm_module_inst_t module_inst,
                      wasi_fd_t fd,
                      int32 path_offset /* const char * */,
                      uint32 path_len)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    char *path;

    if (!validate_app_addr(path_offset, path_len))
        return (wasi_errno_t)-1;

    path = (char*)addr_app_to_native(path_offset);

    return wasmtime_ssp_path_unlink_file(wasi_ctx->curfds, fd, path, path_len);
}

static wasi_errno_t
wasi_path_remove_directory(wasm_module_inst_t module_inst,
                           wasi_fd_t fd,
                           int32 path_offset /* const char * */,
                           uint32 path_len)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    char *path;

    if (!validate_app_addr(path_offset, path_len))
        return (wasi_errno_t)-1;

    path = (char*)addr_app_to_native(path_offset);

    return wasmtime_ssp_path_remove_directory(wasi_ctx->curfds, fd, path, path_len);
}

static wasi_errno_t
wasi_poll_oneoff(wasm_module_inst_t module_inst,
                 int32 in_offset /* const wasi_subscription_t * */,
                 int32 out_offset /* wasi_event_t * */,
                 uint32 nsubscriptions,
                 int32 nevents_offset /* size_t * */)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    wasi_subscription_t *in;
    wasi_event_t *out;
    size_t nevents;
    uint32 *nevents_app;
    wasi_errno_t err;

    if (!validate_app_addr(in_offset, sizeof(wasi_subscription_t))
        || !validate_app_addr(out_offset, sizeof(wasi_event_t))
        || !validate_app_addr(nevents_offset, sizeof(uint32)))
        return (wasi_errno_t)-1;

    in = (wasi_subscription_t*)addr_app_to_native(in_offset);
    out = (wasi_event_t*)addr_app_to_native(out_offset);

    err = wasmtime_ssp_poll_oneoff(wasi_ctx->curfds, in, out, nsubscriptions, &nevents);
    WASI_CHECK_ERR();

    nevents_app = (uint32*)addr_app_to_native(nevents_offset);
    *(uint32*)nevents_app = (uint32)nevents;

    return 0;
}

void wasi_proc_exit(wasm_module_inst_t module_inst,
                              wasi_exitcode_t rval)
{
    wasm_runtime_set_exception(module_inst, "wasi proc exit");
}

static wasi_errno_t
wasi_proc_raise(wasm_module_inst_t module_inst,
                wasi_signal_t sig)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%s%d", "wasi proc raise ", sig);
    wasm_runtime_set_exception(module_inst, buf);

    return 0;
}

static wasi_errno_t
wasi_random_get(wasm_module_inst_t module_inst,
                int32 buf_offset /* void * */,
                uint32 buf_len)
{
    void *buf;

    if (!validate_app_addr(buf_offset, buf_len))
        return (wasi_errno_t)-1;

    buf = (void*)addr_app_to_native(buf_offset);

    return wasmtime_ssp_random_get(buf, buf_len);
}

static wasi_errno_t
wasi_sock_recv(wasm_module_inst_t module_inst,
               wasi_fd_t sock,
               int32 ri_data_offset /* const wasi_iovec_t * */,
               uint32 ri_data_len,
               wasi_riflags_t ri_flags,
               int32 ro_datalen_offset /* size_t * */,
               int32 ro_flags_offset /* wasi_roflags_t * */)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    wasi_iovec_t *iovec, *iovec_begin;
    int32 mem;
    iovec_app_t *ri_data_app;
    uint32 i;
    size_t ro_datalen;
    wasi_roflags_t ro_flags;
    uint32 *ro_datalen_app, *ro_flags_app;
    uint64 total_size;
    wasi_errno_t err;

    total_size = sizeof(iovec_app_t) * (uint64)ri_data_len;
    if (!validate_app_addr(ro_datalen_offset, (uint32)sizeof(uint32))
        || !validate_app_addr(ro_flags_offset, (uint32)sizeof(uint32))
        || total_size >= UINT32_MAX
        || !validate_app_addr(ri_data_offset, (uint32)total_size))
        return (wasi_errno_t)-1;

    ri_data_app = (iovec_app_t*)addr_app_to_native(ri_data_offset);

    total_size = sizeof(wasi_iovec_t) * (uint64)ri_data_len;
    if (total_size >= UINT32_MAX
        || !(mem = module_malloc((uint32)total_size)))
        return (wasi_errno_t)-1;

    iovec = iovec_begin = (wasi_iovec_t*)addr_app_to_native(mem);

    for (i = 0; i < ri_data_len; i++, ri_data_app++, iovec++) {
        if (!validate_app_addr(ri_data_app->buf_offset, ri_data_app->buf_len)) {
            err = (wasi_errno_t)-1;
            goto fail;
        }
        iovec->buf = (void*)addr_app_to_native(ri_data_app->buf_offset);
        iovec->buf_len = ri_data_app->buf_len;
    }

    err = wasmtime_ssp_sock_recv(wasi_ctx->curfds, sock,
                                 iovec_begin, ri_data_len,
                                 ri_flags, &ro_datalen,
                                 &ro_flags);
    if (err)
        goto fail;

    ro_datalen_app = (uint32*)addr_app_to_native(ro_datalen_offset);
    ro_flags_app = (uint32*)addr_app_to_native(ro_flags_offset);

    *(uint32*)ro_datalen_app = (uint32)ro_datalen;
    *(uint32*)ro_flags_app = (uint32)ro_flags;

    /* success */
    err = 0;

fail:
    module_free(mem);
    return err;
}

static wasi_errno_t
wasi_sock_send(wasm_module_inst_t module_inst,
               wasi_fd_t sock,
               int32 si_data_offset /* const wasi_ciovec_t * */,
               uint32 si_data_len,
               wasi_siflags_t si_flags,
               int32 so_datalen_offset /* size_t * */)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);
    int32 mem;
    wasi_ciovec_t *ciovec, *ciovec_begin;
    iovec_app_t *si_data_app;
    uint32 i;
    size_t so_datalen;
    uint32 *so_datalen_app;
    uint64 total_size;
    wasi_errno_t err;

    total_size = sizeof(iovec_app_t) * (uint64)si_data_len;
    if (!validate_app_addr(so_datalen_offset, sizeof(uint32))
        || total_size >= UINT32_MAX
        || !validate_app_addr(si_data_offset, (uint32)total_size))
        return (wasi_errno_t)-1;

    si_data_app = (iovec_app_t*)addr_app_to_native(si_data_offset);

    total_size = sizeof(wasi_ciovec_t) * (uint64)si_data_len;
    if (total_size >= UINT32_MAX
        || !(mem = module_malloc((uint32)total_size)))
        return (wasi_errno_t)-1;

    ciovec_begin = ciovec = (wasi_ciovec_t*)addr_app_to_native(mem);

    for (i = 0; i < si_data_len; i++, si_data_app++, ciovec++) {
        if (!validate_app_addr(si_data_app->buf_offset, si_data_app->buf_len)) {
            err = (wasi_errno_t)-1;
            goto fail;
        }
        ciovec->buf = (char*)addr_app_to_native(si_data_app->buf_offset);
        ciovec->buf_len = si_data_app->buf_len;
    }

    err = wasmtime_ssp_sock_send(wasi_ctx->curfds, sock,
                                 ciovec_begin, si_data_len,
                                 si_flags, &so_datalen);
    if (err)
        goto fail;

    so_datalen_app = (uint32*)addr_app_to_native(so_datalen_offset);
    *(uint32*)so_datalen_app = (uint32)so_datalen;

    /* success */
    err = 0;

fail:
    module_free(mem);
    return err;
}

static wasi_errno_t
wasi_sock_shutdown(wasm_module_inst_t module_inst,
                   wasi_fd_t sock, wasi_sdflags_t how)
{
    wasi_ctx_t wasi_ctx = get_wasi_ctx(module_inst);

    return wasmtime_ssp_sock_shutdown(wasi_ctx->curfds, sock, how);
}

static wasi_errno_t
wasi_sched_yield(wasm_module_inst_t module_inst)
{
    return wasmtime_ssp_sched_yield();
}

#define REG_NATIVE_FUNC(func_name)     \
    { "wasi_unstable", #func_name, wasi_##func_name }

typedef struct WASMNativeFuncDef {
    const char *module_name;
    const char *func_name;
    void *func_ptr;
} WASMNativeFuncDef;

static WASMNativeFuncDef native_func_defs[] = {
    REG_NATIVE_FUNC(args_get),
    REG_NATIVE_FUNC(args_sizes_get),
    REG_NATIVE_FUNC(clock_res_get),
    REG_NATIVE_FUNC(clock_time_get),
    REG_NATIVE_FUNC(environ_get),
    REG_NATIVE_FUNC(environ_sizes_get),
    REG_NATIVE_FUNC(fd_prestat_get),
    REG_NATIVE_FUNC(fd_prestat_dir_name),
    REG_NATIVE_FUNC(fd_close),
    REG_NATIVE_FUNC(fd_datasync),
    REG_NATIVE_FUNC(fd_pread),
    REG_NATIVE_FUNC(fd_pwrite),
    REG_NATIVE_FUNC(fd_read),
    REG_NATIVE_FUNC(fd_renumber),
    REG_NATIVE_FUNC(fd_seek),
    REG_NATIVE_FUNC(fd_tell),
    REG_NATIVE_FUNC(fd_fdstat_get),
    REG_NATIVE_FUNC(fd_fdstat_set_flags),
    REG_NATIVE_FUNC(fd_fdstat_set_rights),
    REG_NATIVE_FUNC(fd_sync),
    REG_NATIVE_FUNC(fd_write),
    REG_NATIVE_FUNC(fd_advise),
    REG_NATIVE_FUNC(fd_allocate),
    REG_NATIVE_FUNC(path_create_directory),
    REG_NATIVE_FUNC(path_link),
    REG_NATIVE_FUNC(path_open),
    REG_NATIVE_FUNC(fd_readdir),
    REG_NATIVE_FUNC(path_readlink),
    REG_NATIVE_FUNC(path_rename),
    REG_NATIVE_FUNC(fd_filestat_get),
    REG_NATIVE_FUNC(fd_filestat_set_times),
    REG_NATIVE_FUNC(fd_filestat_set_size),
    REG_NATIVE_FUNC(path_filestat_get),
    REG_NATIVE_FUNC(path_filestat_set_times),
    REG_NATIVE_FUNC(path_symlink),
    REG_NATIVE_FUNC(path_unlink_file),
    REG_NATIVE_FUNC(path_remove_directory),
    REG_NATIVE_FUNC(poll_oneoff),
    REG_NATIVE_FUNC(proc_exit),
    REG_NATIVE_FUNC(proc_raise),
    REG_NATIVE_FUNC(random_get),
    REG_NATIVE_FUNC(sock_recv),
    REG_NATIVE_FUNC(sock_send),
    REG_NATIVE_FUNC(sock_shutdown),
    REG_NATIVE_FUNC(sched_yield),
};

void*
wasi_native_func_lookup(const char *module_name, const char *func_name)
{
    uint32 size = sizeof(native_func_defs) / sizeof(WASMNativeFuncDef);
    WASMNativeFuncDef *func_def = native_func_defs;
    WASMNativeFuncDef *func_def_end = func_def + size;

    if (!module_name || !func_name)
        return NULL;

    while (func_def < func_def_end) {
        if (!strcmp(func_def->module_name, module_name)
            && !strcmp(func_def->func_name, func_name))
            return (void*) (uintptr_t) func_def->func_ptr;
        func_def++;
    }

    return NULL;
}
#else
void*
wasi_native_func_lookup(const char *module_name, const char *func_name)
{
    return NULL;
}
#endif

