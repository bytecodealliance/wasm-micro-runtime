/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "simd_int_arith.h"

bool
jit_compile_simd_i8x16_arith(JitCompContext *cc, V128Arithmetic arith_op)
{
    return false;
}

bool
jit_compile_simd_i16x8_arith(JitCompContext *cc, V128Arithmetic arith_op)
{
    return false;
}

bool
jit_compile_simd_i32x4_arith(JitCompContext *cc, V128Arithmetic arith_op)
{
    JitReg lhs, rhs, res;

    POP_V128(lhs);
    POP_V128(rhs);

    res = jit_cc_new_reg_V128(cc);

    switch (arith_op) {
        case V128_ADD:
            GEN_INSN(SIMD_i32x4_add, res, lhs, rhs);
            break;
        case V128_SUB:
            GEN_INSN(SIMD_i32x4_sub, res, lhs, rhs);
            break;
        case V128_MUL:
            GEN_INSN(SIMD_i32x4_mul, res, lhs, rhs);
            break;
        default:
            bh_assert(0);
            return false;
    }

    return true;
fail:
    return false;
}

bool
jit_compile_simd_i64x2_arith(JitCompContext *cc, V128Arithmetic arith_op)
{
    return false;
}

bool
jit_compile_simd_i8x16_neg(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_simd_i16x8_neg(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_simd_i32x4_neg(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_simd_i64x2_neg(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_simd_i8x16_popcnt(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_simd_i8x16_cmp(JitCompContext *cc, V128Arithmetic arith_op,
                           bool is_signed)
{
    return false;
}

bool
jit_compile_simd_i16x8_cmp(JitCompContext *cc, V128Arithmetic arith_op,
                           bool is_signed)
{
    return false;
}

bool
jit_compile_simd_i32x4_cmp(JitCompContext *cc, V128Arithmetic arith_op,
                           bool is_signed)
{
    return false;
}

bool
jit_compile_simd_i8x16_abs(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_simd_i16x8_abs(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_simd_i32x4_abs(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_simd_i64x2_abs(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_simd_i8x16_avgr_u(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_simd_i16x8_avgr_u(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_simd_i32x4_avgr_u(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_simd_i32x4_dot_i16x8(JitCompContext *cc)
{
    return false;
}
