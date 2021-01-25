/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "simd_bool_reductions.h"
#include "simd_common.h"
#include "../aot_emit_exception.h"
#include "../../aot/aot_runtime.h"

static bool
simd_any_true(AOTCompContext *comp_ctx,
              AOTFuncContext *func_ctx,
              LLVMTypeRef vector_type,
              LLVMTypeRef element_type,
              const char *intrinsic)
{
    LLVMValueRef vector, zeros, non_zero, result;

    if (!(vector = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, vector_type,
                                             "vec"))) {
        goto fail;
    }

    if (!(zeros = LLVMConstNull(vector_type))) {
        HANDLE_FAILURE("LLVMConstNull");
        goto fail;
    }

    /* icmp eq <N x iX> %vector, zeroinitialize */
    if (!(non_zero = LLVMBuildICmp(comp_ctx->builder, LLVMIntNE, vector, zeros,
                                   "non_zero"))) {
        HANDLE_FAILURE("LLVMBuildICmp");
        goto fail;
    }

    /* zext <N x i1> to <N x iX> */
    if (!(non_zero = LLVMBuildZExt(comp_ctx->builder, non_zero, vector_type,
                                   "non_zero_ex"))) {
        HANDLE_FAILURE("LLVMBuildZExt");
        goto fail;
    }

    if (!(result = aot_call_llvm_intrinsic(comp_ctx, intrinsic, element_type,
                                           &vector_type, 1, non_zero))) {
        HANDLE_FAILURE("LLVMBuildCall");
        goto fail;
    }

    if (!(zeros = LLVMConstNull(element_type))) {
        HANDLE_FAILURE("LLVMConstNull");
        goto fail;
    }

    if (!(result = LLVMBuildICmp(comp_ctx->builder, LLVMIntNE, result, zeros,
                                 "gt_zero"))) {
        HANDLE_FAILURE("LLVMBuildICmp");
        goto fail;
    }

    if (!(result =
            LLVMBuildZExt(comp_ctx->builder, result, I32_TYPE, "ret"))) {
        HANDLE_FAILURE("LLVMBuildZExt");
        goto fail;
    }

    PUSH_I32(result);

    return true;
fail:
    return false;
}

bool
aot_compile_simd_i8x16_any_true(AOTCompContext *comp_ctx,
                                AOTFuncContext *func_ctx)
{
    return simd_any_true(comp_ctx, func_ctx, V128_i8x16_TYPE, INT8_TYPE,
                         "llvm.experimental.vector.reduce.add.v16i8");
}

bool
aot_compile_simd_i16x8_any_true(AOTCompContext *comp_ctx,
                                AOTFuncContext *func_ctx)
{
    return simd_any_true(comp_ctx, func_ctx, V128_i16x8_TYPE, INT16_TYPE,
                         "llvm.experimental.vector.reduce.add.v8i16");
}

bool
aot_compile_simd_i32x4_any_true(AOTCompContext *comp_ctx,
                                AOTFuncContext *func_ctx)
{
    return simd_any_true(comp_ctx, func_ctx, V128_i32x4_TYPE, I32_TYPE,
                         "llvm.experimental.vector.reduce.add.v4i32");
}

static bool
simd_all_true(AOTCompContext *comp_ctx,
              AOTFuncContext *func_ctx,
              LLVMTypeRef vector_type,
              LLVMTypeRef element_type,
              const char *intrinsic)
{
    LLVMValueRef vector, zeros, is_zero, result;

    if (!(vector = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, vector_type,
                                             "vec"))) {
        goto fail;
    }

    if (!(zeros = LLVMConstNull(vector_type))) {
        HANDLE_FAILURE("LLVMConstNull");
        goto fail;
    }

    /* icmp eq <N x iX> %vector, zeroinitialize */
    if (!(is_zero = LLVMBuildICmp(comp_ctx->builder, LLVMIntEQ, vector, zeros,
                                  "is_zero"))) {
        HANDLE_FAILURE("LLVMBuildICmp");
        goto fail;
    }

    /* zext <N x i1> to <N x iX> */
    if (!(is_zero = LLVMBuildZExt(comp_ctx->builder, is_zero, vector_type,
                                  "is_zero_ex"))) {
        HANDLE_FAILURE("LLVMBuildZExt");
        goto fail;
    }

    if (!(result = aot_call_llvm_intrinsic(comp_ctx, intrinsic, element_type,
                                           &vector_type, 1, is_zero))) {
        HANDLE_FAILURE("LLVMBuildCall");
        goto fail;
    }

    if (!(zeros = LLVMConstNull(element_type))) {
        HANDLE_FAILURE("LLVMConstNull");
        goto fail;
    }

    if (!(result = LLVMBuildICmp(comp_ctx->builder, LLVMIntEQ, result, zeros,
                                 "none"))) {
        HANDLE_FAILURE("LLVMBuildICmp");
        goto fail;
    }

    if (!(result =
            LLVMBuildZExt(comp_ctx->builder, result, I32_TYPE, "ret"))) {
        HANDLE_FAILURE("LLVMBuildZExt");
        goto fail;
    }

    PUSH_I32(result);

    return true;
fail:
    return false;
}

bool
aot_compile_simd_i8x16_all_true(AOTCompContext *comp_ctx,
                                AOTFuncContext *func_ctx)
{
    return simd_all_true(comp_ctx, func_ctx, V128_i8x16_TYPE, INT8_TYPE,
                         "llvm.experimental.vector.reduce.add.v16i8");
}

bool
aot_compile_simd_i16x8_all_true(AOTCompContext *comp_ctx,
                                AOTFuncContext *func_ctx)
{
    return simd_all_true(comp_ctx, func_ctx, V128_i16x8_TYPE, INT16_TYPE,
                         "llvm.experimental.vector.reduce.add.v8i16");
}

bool
aot_compile_simd_i32x4_all_true(AOTCompContext *comp_ctx,
                                AOTFuncContext *func_ctx)
{
    return simd_all_true(comp_ctx, func_ctx, V128_i32x4_TYPE, I32_TYPE,
                         "llvm.experimental.vector.reduce.add.v4i32");
}
