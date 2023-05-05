# Copyright (C) 2019 Intel Corporation.  All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

set (LIBC_WASI_DIR ${CMAKE_CURRENT_LIST_DIR})

add_definitions (-DWASM_ENABLE_LIBC_WASI=1)

include_directories(${LIBC_WASI_DIR}/sandboxed-system-primitives/include
                    ${LIBC_WASI_DIR}/sandboxed-system-primitives/src)

file (GLOB_RECURSE source_all ${LIBC_WASI_DIR}/*.c )

set (LIBC_WASI_SOURCE ${source_all})

file(WRITE "${PROJECT_BINARY_DIR}/try_compile_clock_nanosleep.c"
"   #include <time.h>
    void mysleep_ms(int millisec)
    {
        struct timespec res;
        res.tv_sec = millisec/1000;
        res.tv_nsec = (millisec*1000000) % 1000000000;
        clock_nanosleep(CLOCK_MONOTONIC, 0, &res, NULL);
    }
    int main () { return 0; }
")

try_compile(WAMR_HAS_CLOCK_NANOSLEEP
  ${PROJECT_BINARY_DIR}/try_compile_clock_nanosleep
  ${PROJECT_BINARY_DIR}/try_compile_clock_nanosleep.c)

if (WAMR_HAS_CLOCK_NANOSLEEP)
    add_definitions(-DCLOCK_NANOSLEEP_COMPILES)
endif()