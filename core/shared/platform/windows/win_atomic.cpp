/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_vmcore.h"
#include "platform_api_extension.h"

#if WASM_ENABLE_SHARED_MEMORY != 0

#include <atomic>

void
bh_atomic_thread_fence(int mem_order)
{
#if defined(__MINGW64__) || defined(__MINGW32__)
    std::memory_order order =
        (std::memory_order)(int(std::memory_order::relaxed) + mem_order
                            - os_memory_order_relaxed);
#else
    std::memory_order order =
        (std::memory_order)(int(std::memory_order::memory_order_relaxed) + mem_order
                            - os_memory_order_relaxed);
#endif
    std::atomic_thread_fence(order);
}

#endif
