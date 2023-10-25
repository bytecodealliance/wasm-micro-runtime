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