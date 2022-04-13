/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "jit_emit_compare.h"
#include "../jit_frontend.h"

static bool
jit_compile_op_compare(JitCompContext *cc, IntCond cond, bool is64Bit)
{
    JitReg lhs, rhs, res, const_zero, const_one;

    if (cond < INT_EQZ || cond > INT_GE_U) {
        jit_set_last_error(cc, "unsupported comparation operation");
        goto fail;
    }

    res = jit_cc_new_reg_I32(cc);
    const_zero = NEW_CONST(I32, 0);
    const_one = NEW_CONST(I32, 1);

    if (is64Bit) {
        if (INT_EQZ == cond) {
            rhs = NEW_CONST(I64, 0);
        }
        else {
            POP_I64(rhs);
        }
        POP_I64(lhs);
    }
    else {
        if (INT_EQZ == cond) {
            rhs = NEW_CONST(I32, 0);
        }
        else {
            POP_I32(rhs);
        }
        POP_I32(lhs);
    }

    GEN_INSN(CMP, cc->cmp_reg, lhs, rhs);
    switch (cond) {
        case INT_EQ:
        case INT_EQZ:
        {
            GEN_INSN(SELECTEQ, res, cc->cmp_reg, const_one, const_zero);
            break;
        }
        case INT_NE:
        {
            GEN_INSN(SELECTNE, res, cc->cmp_reg, const_one, const_zero);
            break;
        }
        case INT_LT_S:
        {
            GEN_INSN(SELECTLTS, res, cc->cmp_reg, const_one, const_zero);
            break;
        }
        case INT_LT_U:
        {
            GEN_INSN(SELECTLTU, res, cc->cmp_reg, const_one, const_zero);
            break;
        }
        case INT_GT_S:
        {
            GEN_INSN(SELECTGTS, res, cc->cmp_reg, const_one, const_zero);
            break;
        }
        case INT_GT_U:
        {
            GEN_INSN(SELECTGTU, res, cc->cmp_reg, const_one, const_zero);
            break;
        }
        case INT_LE_S:
        {
            GEN_INSN(SELECTLES, res, cc->cmp_reg, const_one, const_zero);
            break;
        }
        case INT_LE_U:
        {
            GEN_INSN(SELECTLEU, res, cc->cmp_reg, const_one, const_zero);
            break;
        }
        case INT_GE_S:
        {
            GEN_INSN(SELECTGES, res, cc->cmp_reg, const_one, const_zero);
            break;
        }
        case INT_GE_U:
        {
            GEN_INSN(SELECTGEU, res, cc->cmp_reg, const_one, const_zero);
            break;
        }
        default:
        {
            goto fail;
        }
    }

    PUSH_I32(res);
    return true;
fail:
    return false;
}

bool
jit_compile_op_i32_compare(JitCompContext *cc, IntCond cond)
{
    return jit_compile_op_compare(cc, cond, false);
}

bool
jit_compile_op_i64_compare(JitCompContext *cc, IntCond cond)
{
    return jit_compile_op_compare(cc, cond, true);
}

bool
jit_compile_op_f32_compare(JitCompContext *cc, FloatCond cond)
{
    jit_set_last_error(cc, "unimplement f32 comparison");
    return false;
}

bool
jit_compile_op_f64_compare(JitCompContext *cc, FloatCond cond)
{
    jit_set_last_error(cc, "unimplement f64 comparison");
    return false;
}
