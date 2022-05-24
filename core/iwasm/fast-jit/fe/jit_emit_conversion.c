/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "../jit_codegen.h"
#include "jit_emit_conversion.h"
#include "../jit_frontend.h"
#include "jit_emit_exception.h"

static double
uint64_to_double(uint64 u64)
{
    return (double)u64;
}

static float
uint64_to_float(uint64 u64)
{
    return (float)u64;
}

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
    JitReg value, nan_ret, max_valid_float, min_valid_float, res;
    JitInsn *insn = NULL;

    POP_F32(value);

    min_valid_float =
        sign ? NEW_CONST(F32, -2147483904.0f) : NEW_CONST(F32, -1.0f);
    max_valid_float =
        sign ? NEW_CONST(F32, 2147483648.0f) : NEW_CONST(F32, 4294967296.0f);

    if (!saturating) {
        /*if (value is Nan) goto exception*/
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
        /* Set nan_ret to x86::eax */
        nan_ret = jit_codegen_get_hreg_by_name("eax");
#else
        nan_ret = jit_cc_new_reg_I32(cc);
#endif
        insn =
            GEN_INSN(CALLNATIVE, nan_ret, NEW_CONST(PTR, (uintptr_t)isnanf), 1);
        if (!insn) {
            goto fail;
        }
        *(jit_insn_opndv(insn, 2)) = value;

        GEN_INSN(CMP, cc->cmp_reg, nan_ret, NEW_CONST(I32, 1));
        if (!jit_emit_exception(cc, EXCE_INVALID_CONVERSION_TO_INTEGER,
                                JIT_OP_BEQ, cc->cmp_reg, NULL)) {
            goto fail;
        }

        /*if (value is out of integer range) goto exception*/
        GEN_INSN(CMP, cc->cmp_reg, value, min_valid_float);
        if (!jit_emit_exception(cc, EXCE_INTEGER_OVERFLOW, JIT_OP_BLES,
                                cc->cmp_reg, NULL)) {
            goto fail;
        }

        GEN_INSN(CMP, cc->cmp_reg, value, max_valid_float);
        if (!jit_emit_exception(cc, EXCE_INTEGER_OVERFLOW, JIT_OP_BGES,
                                cc->cmp_reg, NULL)) {
            goto fail;
        }
    }

    if (saturating) {
        JitReg tmp = jit_cc_new_reg_F32(cc);
        GEN_INSN(MAX, tmp, value, min_valid_float);
        GEN_INSN(MIN, value, tmp, max_valid_float);
    }

    res = jit_cc_new_reg_I32(cc);
    if (sign) {
        GEN_INSN(F32TOI32, res, value);
    }
    else {
        GEN_INSN(F32TOU32, res, value);
    }

    PUSH_I32(res);

    return true;
fail:
    return false;
}

bool
jit_compile_op_i32_trunc_f64(JitCompContext *cc, bool sign, bool saturating)
{
    JitReg value, nan_ret, max_valid_double, min_valid_double, res;
    JitInsn *insn = NULL;

    POP_F64(value);

    min_valid_double =
        sign ? NEW_CONST(F64, -2147483649.0) : NEW_CONST(F64, -1.0);
    max_valid_double =
        sign ? NEW_CONST(F64, 2147483648.0) : NEW_CONST(F64, 4294967296.0);

    if (!saturating) {
        /*if (value is Nan) goto exception*/
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
        /* Set nan_ret to x86::eax */
        nan_ret = jit_codegen_get_hreg_by_name("eax");
#else
        nan_ret = jit_cc_new_reg_I32(cc);
#endif
        insn =
            GEN_INSN(CALLNATIVE, nan_ret, NEW_CONST(PTR, (uintptr_t)isnan), 1);
        if (!insn) {
            goto fail;
        }
        *(jit_insn_opndv(insn, 2)) = value;

        GEN_INSN(CMP, cc->cmp_reg, nan_ret, NEW_CONST(I32, 1));
        if (!jit_emit_exception(cc, EXCE_INVALID_CONVERSION_TO_INTEGER,
                                JIT_OP_BEQ, cc->cmp_reg, NULL)) {
            goto fail;
        }

        /*if (value is out of integer range) goto exception*/
        GEN_INSN(CMP, cc->cmp_reg, value, min_valid_double);
        if (!jit_emit_exception(cc, EXCE_INTEGER_OVERFLOW, JIT_OP_BLES,
                                cc->cmp_reg, NULL)) {
            goto fail;
        }

        GEN_INSN(CMP, cc->cmp_reg, value, max_valid_double);
        if (!jit_emit_exception(cc, EXCE_INTEGER_OVERFLOW, JIT_OP_BGES,
                                cc->cmp_reg, NULL)) {
            goto fail;
        }
    }

    if (saturating) {
        JitReg tmp = jit_cc_new_reg_F64(cc);
        GEN_INSN(MAX, tmp, value, min_valid_double);
        GEN_INSN(MIN, value, tmp, max_valid_double);
    }

    res = jit_cc_new_reg_I32(cc);
    if (sign) {
        GEN_INSN(F64TOI32, res, value);
    }
    else {
        GEN_INSN(F64TOU32, res, value);
    }

    PUSH_I32(res);

    return true;
fail:
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
jit_compile_op_i64_extend_i64(JitCompContext *cc, int8 bitwidth)
{
    bh_assert(0);
    return false;
}

bool
jit_compile_op_i32_extend_i32(JitCompContext *cc, int8 bitwidth)
{
    bh_assert(0);
    return false;
}

bool
jit_compile_op_i64_trunc_f32(JitCompContext *cc, bool sign, bool saturating)
{
    /* TODO: f32 -> u64 algorithm */
    bh_assert(0);
    return false;
}

bool
jit_compile_op_i64_trunc_f64(JitCompContext *cc, bool sign, bool saturating)
{
    /* TODO: f64 -> u64 algorithm */
    bh_assert(0);
    return false;
}

bool
jit_compile_op_f32_convert_i32(JitCompContext *cc, bool sign)
{
    JitReg value, res;

    POP_I32(value);

    res = jit_cc_new_reg_F32(cc);
    if (sign) {
        GEN_INSN(I32TOF32, res, value);
    }
    else {
        GEN_INSN(U32TOF32, res, value);
    }
    PUSH_F32(res);

    return true;
fail:
    return false;
}

bool
jit_compile_op_f32_convert_i64(JitCompContext *cc, bool sign)
{
    JitReg value, res;

    POP_I64(value);

    if (sign) {
        res = jit_cc_new_reg_F32(cc);
        GEN_INSN(I64TOF32, res, value);
    }
    else {
        JitInsn *insn;
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
        res = jit_codegen_get_hreg_by_name("xmm0");
#else
        res = jit_cc_new_reg_F32(cc);
#endif
        insn = GEN_INSN(CALLNATIVE, res,
                        NEW_CONST(PTR, (uintptr_t)uint64_to_float), 1);
        if (!insn) {
            goto fail;
        }
        *(jit_insn_opndv(insn, 2)) = value;
    }

    PUSH_F32(res);

    return true;
fail:
    return false;
}

bool
jit_compile_op_f32_demote_f64(JitCompContext *cc)
{
    JitReg value, res;

    POP_F64(value);

    res = jit_cc_new_reg_F32(cc);
    GEN_INSN(F64TOF32, res, value);
    PUSH_F32(res);

    return true;
fail:
    return false;
}

bool
jit_compile_op_f64_convert_i32(JitCompContext *cc, bool sign)
{
    JitReg value, res;

    POP_I32(value);

    res = jit_cc_new_reg_F64(cc);
    if (sign) {
        GEN_INSN(I32TOF64, res, value);
    }
    else {
        GEN_INSN(U32TOF64, res, value);
    }
    PUSH_F64(res);

    return true;
fail:
    return false;
}

bool
jit_compile_op_f64_convert_i64(JitCompContext *cc, bool sign)
{
    JitReg value, res;

    POP_I64(value);

    if (sign) {
        res = jit_cc_new_reg_F64(cc);
        GEN_INSN(I64TOF64, res, value);
    }
    else {
        JitInsn *insn;
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
        res = jit_codegen_get_hreg_by_name("xmm0_f64");
#else
        res = jit_cc_new_reg_F64(cc);
#endif
        insn = GEN_INSN(CALLNATIVE, res,
                        NEW_CONST(PTR, (uintptr_t)uint64_to_double), 1);
        if (!insn) {
            goto fail;
        }
        *(jit_insn_opndv(insn, 2)) = value;
    }
    PUSH_F64(res);

    return true;
fail:
    return false;
}

bool
jit_compile_op_f64_promote_f32(JitCompContext *cc)
{
    JitReg value, res;

    POP_F32(value);

    res = jit_cc_new_reg_F64(cc);
    GEN_INSN(F32TOF64, res, value);
    PUSH_F64(res);

    return true;
fail:
    return false;
}

bool
jit_compile_op_i64_reinterpret_f64(JitCompContext *cc)
{
    JitReg value, res;

    POP_F64(value);

    res = jit_cc_new_reg_I64(cc);
    GEN_INSN(F64CASTI64, res, value);
    PUSH_I64(res);

    return true;
fail:
    return false;
}

bool
jit_compile_op_i32_reinterpret_f32(JitCompContext *cc)
{
    JitReg value, res;

    POP_F32(value);

    res = jit_cc_new_reg_I32(cc);
    GEN_INSN(F32CASTI32, res, value);
    PUSH_I32(res);

    return true;
fail:
    return false;
}

bool
jit_compile_op_f64_reinterpret_i64(JitCompContext *cc)
{
    JitReg value, res;

    POP_I64(value);

    res = jit_cc_new_reg_F64(cc);
    GEN_INSN(I64CASTF64, res, value);
    PUSH_F64(res);

    return true;
fail:
    return false;
}

bool
jit_compile_op_f32_reinterpret_i32(JitCompContext *cc)
{
    JitReg value, res;

    POP_I32(value);

    res = jit_cc_new_reg_F32(cc);
    GEN_INSN(I32CASTF32, res, value);
    PUSH_F32(res);

    return true;
fail:
    return false;
}
