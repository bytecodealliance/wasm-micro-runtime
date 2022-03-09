/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "jit_emit_memory.h"
#include "../jit_frontend.h"

bool
jit_compile_op_i32_load(JitCompContext *cc, uint32 align, uint32 offset,
                        uint32 bytes, bool sign, bool atomic)
{
    return false;
}

bool
jit_compile_op_i64_load(JitCompContext *cc, uint32 align, uint32 offset,
                        uint32 bytes, bool sign, bool atomic)
{
    return false;
}

bool
jit_compile_op_f32_load(JitCompContext *cc, uint32 align, uint32 offset)
{
    return false;
}

bool
jit_compile_op_f64_load(JitCompContext *cc, uint32 align, uint32 offset)
{
    return false;
}

bool
jit_compile_op_i32_store(JitCompContext *cc, uint32 align, uint32 offset,
                         uint32 bytes, bool atomic)
{
    return false;
}

bool
jit_compile_op_i64_store(JitCompContext *cc, uint32 align, uint32 offset,
                         uint32 bytes, bool atomic)
{
    return false;
}

bool
jit_compile_op_f32_store(JitCompContext *cc, uint32 align, uint32 offset)
{
    return false;
}

bool
jit_compile_op_f64_store(JitCompContext *cc, uint32 align, uint32 offset)
{
    return false;
}

bool
jit_compile_op_memory_size(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_op_memory_grow(JitCompContext *cc)
{
    return false;
}

#if WASM_ENABLE_BULK_MEMORY != 0
bool
jit_compile_op_memory_init(JitCompContext *cc, uint32 seg_index)
{
    return false;
}

bool
jit_compile_op_data_drop(JitCompContext *cc, uint32 seg_index)
{
    return false;
}

bool
jit_compile_op_memory_copy(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_op_memory_fill(JitCompContext *cc)
{
    return false;
}
#endif

#if WASM_ENABLE_SHARED_MEMORY != 0
bool
jit_compile_op_atomic_rmw(JitCompContext *cc, uint8 atomic_op, uint8 op_type,
                          uint32 align, uint32 offset, uint32 bytes)
{
    return false;
}

bool
jit_compile_op_atomic_cmpxchg(JitCompContext *cc, uint8 op_type, uint32 align,
                              uint32 offset, uint32 bytes)
{
    return false;
}

bool
jit_compile_op_atomic_wait(JitCompContext *cc, uint8 op_type, uint32 align,
                           uint32 offset, uint32 bytes)
{
    return false;
}

bool
jit_compiler_op_atomic_notify(JitCompContext *cc, uint32 align, uint32 offset,
                              uint32 bytes)
{
    return false;
}
#endif
