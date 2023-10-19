/*
 * Copyright (C) 2023 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_extension.h"
#include "platform_internal.h"

__wasi_errno_t
os_fstat(os_file_handle handle, struct __wasi_filestat_t *buf)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_fstatat(os_file_handle handle, const char *path,
           struct __wasi_filestat_t *buf, __wasi_lookupflags_t lookup_flags)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_file_get_fdflags(os_file_handle handle, __wasi_fdflags_t *flags)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_file_set_fdflags(os_file_handle handle, __wasi_fdflags_t flags)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_file_get_access_mode(os_file_handle handle,
                        wasi_libc_file_access_mode *access_mode)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_fdatasync(os_file_handle handle)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_fsync(os_file_handle handle)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_open_preopendir(const char *path, os_file_handle *out)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_openat(os_file_handle handle, const char *path, __wasi_oflags_t oflags,
          __wasi_fdflags_t fs_flags, __wasi_lookupflags_t lookup_flags,
          wasi_libc_file_access_mode read_write_mode, os_file_handle *out)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_close(os_file_handle handle, bool is_stdio)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_preadv(os_file_handle handle, const struct __wasi_iovec_t *iov, int iovcnt,
          __wasi_filesize_t offset, size_t *nread)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_pwritev(os_file_handle handle, const struct __wasi_ciovec_t *iov, int iovcnt,
           __wasi_filesize_t offset, size_t *nwritten)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_readv(os_file_handle handle, const struct __wasi_iovec_t *iov, int iovcnt,
         size_t *nread)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_writev(os_file_handle handle, const struct __wasi_ciovec_t *iov, int iovcnt,
          size_t *nwritten)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_fallocate(os_file_handle handle, __wasi_filesize_t offset,
             __wasi_filesize_t length)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_ftruncate(os_file_handle handle, __wasi_filesize_t size)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_futimens(os_file_handle handle, __wasi_timestamp_t access_time,
            __wasi_timestamp_t modification_time, __wasi_fstflags_t fstflags)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_utimensat(os_file_handle handle, const char *path,
             __wasi_timestamp_t access_time,
             __wasi_timestamp_t modification_time, __wasi_fstflags_t fstflags,
             __wasi_lookupflags_t lookup_flags)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_readlinkat(os_file_handle handle, const char *path, char *buf,
              size_t bufsize, size_t *nread)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_linkat(os_file_handle from_handle, const char *from_path,
          os_file_handle to_handle, const char *to_path,
          __wasi_lookupflags_t lookup_flags)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_symlinkat(const char *old_path, os_file_handle handle, const char *new_path)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_mkdirat(os_file_handle handle, const char *path)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_renameat(os_file_handle old_handle, const char *old_path,
            os_file_handle new_handle, const char *new_path)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_unlinkat(os_file_handle handle, const char *path, bool is_dir)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_lseek(os_file_handle handle, __wasi_filedelta_t offset,
         __wasi_whence_t whence, __wasi_filesize_t *new_offset)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_fadvise(os_file_handle handle, __wasi_filesize_t offset,
           __wasi_filesize_t length, __wasi_advice_t advice)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_isatty(os_file_handle handle)
{
    return __WASI_ENOSYS;
}

os_file_handle
os_convert_stdin_handle(os_raw_file_handle raw_stdin)
{
    return INVALID_HANDLE_VALUE;
}

os_file_handle
os_convert_stdout_handle(os_raw_file_handle raw_stdout)
{
    return INVALID_HANDLE_VALUE;
}

os_file_handle
os_convert_stderr_handle(os_raw_file_handle raw_stderr)
{
    return INVALID_HANDLE_VALUE;
}

__wasi_errno_t
os_fdopendir(os_file_handle handle, os_dir_stream *dir_stream)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_rewinddir(os_dir_stream dir_stream)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_seekdir(os_dir_stream dir_stream, __wasi_dircookie_t position)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_readdir(os_dir_stream dir_stream, __wasi_dirent_t *entry,
           const char **d_name)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_closedir(os_dir_stream dir_stream)
{
    return __WASI_ENOSYS;
}

os_dir_stream
os_get_invalid_dir_stream()
{
    return NULL;
}

bool
os_is_dir_stream_valid(os_dir_stream *dir_stream)
{
    return false;
}

os_file_handle
os_get_invalid_handle()
{
    return INVALID_HANDLE_VALUE;
}

bool
os_is_handle_valid(os_file_handle *handle)
{
    return false;
}

char *
os_realpath(const char *path, char *resolved_path)
{
    return NULL;
}