/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "simd_bitmask_extracts.h"
#include "simd_common.h"
#include "../aot_emit_exception.h"
#include "../../aot/aot_runtime.h"

static bool
simd_build_bitmask(const AOTCompContext *comp_ctx,
                   const AOTFuncContext *func_ctx,
                   uint8 length,
                   LLVMTypeRef vector_type,
                   LLVMTypeRef element_type,
                   const char *intrinsic)
{
    LLVMValueRef vector, zeros, mask, mask_elements[16], cond, result;
    LLVMTypeRef param_types[1], vector_ext_type;
    const uint32 numbers[16] = { 0x1,    0x2,    0x4,    0x8,   0x10,  0x20,
                                 0x40,   0x80,   0x100,  0x200, 0x400, 0x800,
                                 0x1000, 0x2000, 0x4000, 0x8000 };
    uint8 i;

    if (!(vector = simd_pop_v128_and_bitcast(comp_ctx, func_ctx, vector_type,
                                             "vec"))) {
        goto fail;
    }

    if (!(vector_ext_type = LLVMVectorType(I32_TYPE, length))) {
        HANDLE_FAILURE("LLVMVectorType");
        goto fail;
    }

    if (!(vector = LLVMBuildSExt(comp_ctx->builder, vector, vector_ext_type,
                                 "vec_ext"))) {
        HANDLE_FAILURE("LLVMBuildSExt");
        goto fail;
    }

    if (!(zeros = LLVMConstNull(vector_ext_type))) {
        HANDLE_FAILURE("LLVMConstNull");
        goto fail;
    }

    for (i = 0; i < 16; i++) {
        if (!(mask_elements[i] = LLVMConstInt(I32_TYPE, numbers[i], false))) {
            HANDLE_FAILURE("LLVMConstInt");
            goto fail;
        }
    }

    if (!(mask = LLVMConstVector(mask_elements, length))) {
        HANDLE_FAILURE("LLVMConstVector");
        goto fail;
    }

    if (!(cond = LLVMBuildICmp(comp_ctx->builder, LLVMIntSLT, vector, zeros,
                               "lt_zero"))) {
        HANDLE_FAILURE("LLVMBuildICmp");
        goto fail;
    }

    if (!(result =
            LLVMBuildSelect(comp_ctx->builder, cond, mask, zeros, "select"))) {
        HANDLE_FAILURE("LLVMBuildSelect");
        goto fail;
    }

    param_types[0] = vector_ext_type;
    if (!(result = aot_call_llvm_intrinsic(comp_ctx, func_ctx, intrinsic, I32_TYPE,
                                           param_types, 1, result))) {
        HANDLE_FAILURE("LLVMBuildCall");
        goto fail;
    }

    PUSH_I32(result);

    return true;
fail:
    return false;
}

bool
aot_compile_simd_i8x16_bitmask(AOTCompContext *comp_ctx,
                               AOTFuncContext *func_ctx)
{
    return simd_build_bitmask(comp_ctx, func_ctx, 16, V128_i8x16_TYPE,
                              INT8_TYPE,
                              "llvm.experimental.vector.reduce.or.v16i32");
}

bool
aot_compile_simd_i16x8_bitmask(AOTCompContext *comp_ctx,
                               AOTFuncContext *func_ctx)
{
    return simd_build_bitmask(comp_ctx, func_ctx, 8, V128_i16x8_TYPE,
                              INT16_TYPE,
                              "llvm.experimental.vector.reduce.or.v8i32");
}

bool
aot_compile_simd_i32x4_bitmask(AOTCompContext *comp_ctx,
                               AOTFuncContext *func_ctx)
{
    return simd_build_bitmask(comp_ctx, func_ctx, 4, V128_i32x4_TYPE, I32_TYPE,
                              "llvm.experimental.vector.reduce.or.v4i32");
}
