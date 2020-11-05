/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "simd_construct_values.h"
#include "../aot_emit_exception.h"
#include "../interpreter/wasm_opcode.h"
#include "../../aot/aot_runtime.h"

bool
aot_compile_simd_v128_const(AOTCompContext *comp_ctx,
                            AOTFuncContext *func_ctx,
                            const uint8 *imm_bytes)
{
    uint64 imm1, imm2;
    LLVMValueRef undef, first_long, agg1, second_long, agg2;

    wasm_runtime_read_v128(imm_bytes, &imm1, &imm2);

    if (!(undef = LLVMGetUndef(V128_i64x2_TYPE))) {
        HANDLE_FAILURE("LLVMGetUndef");
        goto fail;
    }

    /* %agg1 = insertelement <2 x i64> undef, i16 0, i64 ${*imm} */
    if (!(first_long = I64_CONST(imm1))) {
        HANDLE_FAILURE("LLVMConstInt");
        goto fail;
    }

    if (!(agg1 = LLVMBuildInsertElement(comp_ctx->builder, undef, first_long,
                                        I32_ZERO, "agg1"))) {
        HANDLE_FAILURE("LLVMBuildInsertElement");
        goto fail;
    }

    /* %agg2 = insertelement <2 x i64> %agg1, i16 1, i64 ${*(imm + 1)} */
    if (!(second_long = I64_CONST(imm2))) {
        HANDLE_FAILURE("LLVMGetUndef");
        goto fail;
    }

    if (!(agg2 = LLVMBuildInsertElement(comp_ctx->builder, agg1, second_long,
                                        I32_ONE, "agg2"))) {
        HANDLE_FAILURE("LLVMBuildInsertElement");
        goto fail;
    }

    PUSH_V128(agg2);

    return true;
fail:
    return false;
}

bool
aot_compile_simd_splat(AOTCompContext *comp_ctx,
                       AOTFuncContext *func_ctx,
                       uint8 splat_opcode)
{
    LLVMValueRef value, undef, base, mask, new_vector, result;
    LLVMTypeRef all_zero_ty;

    switch (splat_opcode) {
        case SIMD_i8x16_splat:
        {
            LLVMValueRef input;
            POP_I32(input);

            /* trunc i32 %input to i8 */
            if (!(value = LLVMBuildTrunc(comp_ctx->builder, input, INT8_TYPE,
                                         "trunc"))) {
                HANDLE_FAILURE("LLVMBuildTrunc");
                goto fail;
            }
            undef = LLVMGetUndef(V128_i8x16_TYPE);
            if (!(all_zero_ty = LLVMVectorType(I32_TYPE, 16))) {
                HANDLE_FAILURE("LLVMVectorType");
                goto fail;
            }
            break;
        }
        case SIMD_i16x8_splat:
        {
            LLVMValueRef input;
            POP_I32(input);

            /* trunc i32 %input to i16 */
            if (!(value = LLVMBuildTrunc(comp_ctx->builder, input, INT16_TYPE,
                                         "trunc"))) {
                HANDLE_FAILURE("LLVMBuildTrunc");
                goto fail;
            }
            undef = LLVMGetUndef(V128_i16x8_TYPE);
            if (!(all_zero_ty = LLVMVectorType(I32_TYPE, 8))) {
                HANDLE_FAILURE("LLVMVectorType");
                goto fail;
            }
            break;
        }
        case SIMD_i32x4_splat:
        {
            POP_I32(value);
            undef = LLVMGetUndef(V128_i32x4_TYPE);

            if (!(all_zero_ty = LLVMVectorType(I32_TYPE, 4))) {
                HANDLE_FAILURE("LLVMVectorType");
                goto fail;
            }
            break;
        }
        case SIMD_i64x2_splat:
        {
            POP(value, VALUE_TYPE_I64);
            undef = LLVMGetUndef(V128_i64x2_TYPE);

            if (!(all_zero_ty = LLVMVectorType(I32_TYPE, 2))) {
                HANDLE_FAILURE("LLVMVectorType");
                goto fail;
            }
            break;
        }
        case SIMD_f32x4_splat:
        {
            POP(value, VALUE_TYPE_F32);
            undef = LLVMGetUndef(V128_f32x4_TYPE);

            if (!(all_zero_ty = LLVMVectorType(I32_TYPE, 4))) {
                HANDLE_FAILURE("LLVMVectorType");
                goto fail;
            }
            break;
        }
        case SIMD_f64x2_splat:
        {
            POP(value, VALUE_TYPE_F64);
            undef = LLVMGetUndef(V128_f64x2_TYPE);

            if (!(all_zero_ty = LLVMVectorType(I32_TYPE, 2))) {
                HANDLE_FAILURE("LLVMVectorType");
                goto fail;
            }
            break;
        }
        default:
        {
            bh_assert(0);
            goto fail;
        }
    }
    if (!undef) {
        HANDLE_FAILURE("LVMGetUndef");
        goto fail;
    }

    /* insertelement <n x ty> undef, ty %value, i32 0 */
    if (!(base = LLVMBuildInsertElement(comp_ctx->builder, undef, value,
                                        I32_ZERO, "base"))) {
        HANDLE_FAILURE("LLVMBuildInsertElement");
        goto fail;
    }

    /* <n x i32> zeroinitializer */
    if (!(mask = LLVMConstNull(all_zero_ty))) {
        HANDLE_FAILURE("LLVMConstNull");
        goto fail;
    }

    /* shufflevector <ty1> %base, <ty2> undef, <n x i32> zeroinitializer */
    if (!(new_vector = LLVMBuildShuffleVector(comp_ctx->builder, base, undef,
                                              mask, "new_vector"))) {
        HANDLE_FAILURE("LLVMBuildShuffleVector");
        goto fail;
    }

    /* bitcast <ty> <value> to <2 x i64> */
    if (!(result = LLVMBuildBitCast(comp_ctx->builder, new_vector,
                                    V128_i64x2_TYPE, "ret"))) {
        HANDLE_FAILURE("LLVMBuidlCast");
        goto fail;
    }

    /* push result into the stack */
    PUSH_V128(result);

    return true;
fail:
    return false;
}
