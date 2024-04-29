/*
 * Copyright (C) 2024 Amazon Inc.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WASM_LOADER_COMMON_H
#define _WASM_LOADER_COMMON_H

#include "platform_common.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
wasm_memory_check_flags(const uint8 mem_flag, char *error_buf,
                        uint32 error_buf_size, bool is_aot);

#ifdef __cplusplus
}
#endif

#endif /* end of _WASM_LOADER_COMMON_H */