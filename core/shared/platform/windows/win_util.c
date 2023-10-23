/*
 * Copyright (C) 2023 Amazon Inc.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "win_util.h"

__wasi_timestamp_t
convert_filetime_to_wasi_timestamp(LPFILETIME filetime)
{
    // From 1601-01-01 to 1970-01-01 there are 134774 days.
    static const uint64_t NT_to_UNIX_epoch =
        134774ull * 86400ull * 1000ull * 1000ull * 1000ull;

    ULARGE_INTEGER temp = { .HighPart = filetime->dwHighDateTime,
                            .LowPart = filetime->dwLowDateTime };

    // WASI timestamps are measured in nanoseconds whereas FILETIME structs are
    // represented in terms 100-nanosecond intervals.
    return (temp.QuadPart * 100ull) - NT_to_UNIX_epoch;
}

__wasi_errno_t
convert_windows_error_code(DWORD windows_error_code)
{
    switch (windows_error_code) {
        case ERROR_INVALID_PARAMETER:
        case ERROR_INVALID_HANDLE:
        case ERROR_NEGATIVE_SEEK:
            return __WASI_EINVAL;
        case ERROR_SHARING_VIOLATION:
        case ERROR_PIPE_BUSY:
            return __WASI_EBUSY;
        case ERROR_ACCESS_DENIED:
            return __WASI_EACCES;
        case ERROR_ALREADY_EXISTS:
        case ERROR_FILE_EXISTS:
            return __WASI_EEXIST;
        case ERROR_NO_MORE_FILES:
        case ERROR_FILE_NOT_FOUND:
        case ERROR_INVALID_NAME:
            return __WASI_ENOENT;
        case ERROR_PRIVILEGE_NOT_HELD:
            return __WASI_EPERM;
        case ERROR_NOT_ENOUGH_MEMORY:
            return __WASI_ENOMEM;
        case ERROR_NOACCESS:
            return __WASI_EFAULT;
        case ERROR_DIR_NOT_EMPTY:
            return __WASI_ENOTEMPTY;
        case ERROR_DIRECTORY:
            return __WASI_ENOTDIR;
        case ERROR_IO_PENDING:
        case ERROR_INSUFFICIENT_BUFFER:
        case ERROR_INVALID_FLAGS:
        case ERROR_NO_UNICODE_TRANSLATION:
        default:
            return __WASI_EINVAL;
    }
}