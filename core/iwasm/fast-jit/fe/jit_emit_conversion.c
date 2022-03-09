/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "jit_emit_conversion.h"
#include "../jit_frontend.h"

bool
jit_compile_op_i32_wrap_i64(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_op_i32_trunc_f32(JitCompContext *cc, bool sign, bool saturating)
{
    return false;
}

bool
jit_compile_op_i32_trunc_f64(JitCompContext *cc, bool sign, bool saturating)
{
    return false;
}

bool
jit_compile_op_i64_extend_i32(JitCompContext *comp_ctx, bool sign)
{
    return false;
}

bool
jit_compile_op_i64_extend_i64(JitCompContext *comp_ctx, int8 bitwidth)
{
    return false;
}

bool
jit_compile_op_i32_extend_i32(JitCompContext *comp_ctx, int8 bitwidth)
{
    return false;
}

bool
jit_compile_op_i64_trunc_f32(JitCompContext *cc, bool sign, bool saturating)
{
    return false;
}

bool
jit_compile_op_i64_trunc_f64(JitCompContext *cc, bool sign, bool saturating)
{
    return false;
}

bool
jit_compile_op_f32_convert_i32(JitCompContext *comp_ctx, bool sign)
{
    return false;
}

bool
jit_compile_op_f32_convert_i64(JitCompContext *comp_ctx, bool sign)
{
    return false;
}

bool
jit_compile_op_f32_demote_f64(JitCompContext *comp_ctx)
{
    return false;
}

bool
jit_compile_op_f64_convert_i32(JitCompContext *comp_ctx, bool sign)
{
    return false;
}

bool
jit_compile_op_f64_convert_i64(JitCompContext *comp_ctx, bool sign)
{
    return false;
}

bool
jit_compile_op_f64_promote_f32(JitCompContext *comp_ctx)
{
    return false;
}

bool
jit_compile_op_i64_reinterpret_f64(JitCompContext *comp_ctx)
{
    return false;
}

bool
jit_compile_op_i32_reinterpret_f32(JitCompContext *comp_ctx)
{
    return false;
}

bool
jit_compile_op_f64_reinterpret_i64(JitCompContext *comp_ctx)
{
    return false;
}

bool
jit_compile_op_f32_reinterpret_i32(JitCompContext *comp_ctx)
{
    return false;
}
