/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "jit_emit_conversion.h"
#include "../jit_frontend.h"

bool
jit_compile_op_i32_wrap_i64(JitCompContext *cc)
{
    JitReg num, res;

    POP_I64(num);

    res = jit_cc_new_reg_I32(cc);
    GEN_INSN(I64TOI32, res, num);
    PUSH_I32(res);

    return true;
fail:
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
jit_compile_op_i64_extend_i32(JitCompContext *cc, bool sign)
{
    JitReg num, res;

    POP_I32(num);

    res = jit_cc_new_reg_I64(cc);
    if (sign) {
        GEN_INSN(I32TOI64, res, num);
    }
    else {
        GEN_INSN(U32TOI64, res, num);
    }
    PUSH_I64(res);

    return true;
fail:
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
