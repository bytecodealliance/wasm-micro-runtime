/*
 * Copyright (C) 2024 Amazon Inc.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#include "wasm_loader_common.h"
#include "bh_log.h"
#include "../interpreter/wasm.h"

static void
set_error_buf(char *error_buf, uint32 error_buf_size, const char *string,
              bool is_aot)
{
    if (error_buf != NULL) {
        snprintf(error_buf, error_buf_size, "%s module load failed: %s",
                 is_aot ? "AOT" : "WASM", string);
    }
}

bool
wasm_memory_check_flags(const uint8 mem_flag, char *error_buf,
                        uint32 error_buf_size, bool is_aot)
{
    /* Check whether certain features indicated by mem_flag are enabled in
     * runtime */
    if (mem_flag > MAX_PAGE_COUNT_FLAG) {
#if WASM_ENABLE_SHARED_MEMORY == 0
        if (mem_flag & SHARED_MEMORY_FLAG) {
            LOG_VERBOSE("shared memory flag was found, please enable shared "
                        "memory, lib-pthread or lib-wasi-threads");
            set_error_buf(error_buf, error_buf_size, "invalid limits flags",
                          is_aot);
            return false;
        }
#endif
#if WASM_ENABLE_MEMORY64 == 0
        if (mem_flag & MEMORY64_FLAG) {
            LOG_VERBOSE("memory64 flag was found, please enable memory64");
            set_error_buf(error_buf, error_buf_size, "invalid limits flags",
                          is_aot);
            return false;
        }
#endif
    }

    if (mem_flag > MAX_PAGE_COUNT_FLAG + SHARED_MEMORY_FLAG + MEMORY64_FLAG) {
        set_error_buf(error_buf, error_buf_size, "invalid limits flags",
                      is_aot);
        return false;
    }
    else if ((mem_flag & SHARED_MEMORY_FLAG)
             && !(mem_flag & MAX_PAGE_COUNT_FLAG)) {
        set_error_buf(error_buf, error_buf_size,
                      "shared memory must have maximum", is_aot);
        return false;
    }

    return true;
}
