/*
 * Copyright (C) 2024 Amazon Inc.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#include "wasm_loader_common.h"
#include "bh_log.h"
#if WASM_ENABLE_GC != 0
#include "../common/gc/gc_type.h"
#endif

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

/*
 * compare with a bigger type set in `wasm_value_type_size_internal()`,
 * this function will only cover global value type, function's param
 * value type and function's result value type.
 *
 * please feel free to add more if there are more requirements
 */
bool
is_valid_value_type(uint8 type)
{
    if (/* I32/I64/F32/F64, 0x7C to 0x7F */
        (type >= VALUE_TYPE_F64 && type <= VALUE_TYPE_I32)
#if WASM_ENABLE_GC != 0
        /* reference types, 0x65 to 0x70 */
        || wasm_is_type_reftype(type)
#elif WASM_ENABLE_REF_TYPES != 0
        || (type == VALUE_TYPE_FUNCREF || type == VALUE_TYPE_EXTERNREF)
#endif
#if WASM_ENABLE_SIMD != 0
        || type == VALUE_TYPE_V128 /* 0x7B */
#endif
    )
        return true;
    return false;
}

bool
is_valid_func_type(const WASMFuncType *func_type)
{
    unsigned i;
    for (i = 0; i < func_type->param_count + func_type->result_count; i++) {
        if (!is_valid_value_type(func_type->types[i]))
            return false;
    }

    return true;
}

/*
 * Indices are represented as a u32.
 */
bool
is_indices_overflow(uint32 import, uint32 other, char *error_buf,
                    uint32 error_buf_size)
{
    if (import > UINT32_MAX - other) {
        snprintf(error_buf, error_buf_size,
                 "too many items in the index space(%" PRIu32 "+%" PRIu32 ").",
                 import, other);
        return true;
    }

    return false;
}
