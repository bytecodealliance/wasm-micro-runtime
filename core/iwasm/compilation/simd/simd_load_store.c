/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "simd_load_store.h"
#include "../aot_emit_exception.h"
#include "../aot_emit_memory.h"
#include "../../aot/aot_runtime.h"
#include "../../interpreter/wasm_opcode.h"

/* data_length in bytes */
static LLVMValueRef
simd_load(AOTCompContext *comp_ctx,
          AOTFuncContext *func_ctx,
          uint32 align,
          uint32 offset,
          uint32 data_length,
          LLVMTypeRef ptr_type)
{
    LLVMValueRef maddr, data;

    if (!(maddr = aot_check_memory_overflow(comp_ctx, func_ctx, offset,
                                            data_length))) {
        HANDLE_FAILURE("aot_check_memory_overflow");
        goto fail;
    }

    if (!(maddr = LLVMBuildBitCast(comp_ctx->builder, maddr, ptr_type,
                                   "data_ptr"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        goto fail;
    }

    if (!(data = LLVMBuildLoad(comp_ctx->builder, maddr, "data"))) {
        HANDLE_FAILURE("LLVMBuildLoad");
        goto fail;
    }

    LLVMSetAlignment(data, 1);

    return data;
fail:
    return NULL;
}

/* data_length in bytes */
static LLVMValueRef
simd_splat(AOTCompContext *comp_ctx,
           AOTFuncContext *func_ctx,
           LLVMValueRef element,
           LLVMTypeRef vectory_type,
           unsigned lane_count)
{
    LLVMValueRef undef, zeros, vector;
    LLVMTypeRef zeros_type;

    if (!(undef = LLVMGetUndef(vectory_type))) {
        HANDLE_FAILURE("LLVMGetUndef");
        goto fail;
    }

    if (!(zeros_type = LLVMVectorType(I32_TYPE, lane_count))) {
        HANDLE_FAILURE("LVMVectorType");
        goto fail;
    }

    if (!(zeros = LLVMConstNull(zeros_type))) {
        HANDLE_FAILURE("LLVMConstNull");
        goto fail;
    }

    if (!(vector = LLVMBuildInsertElement(comp_ctx->builder, undef, element,
                                          I32_ZERO, "base"))) {
        HANDLE_FAILURE("LLVMBuildInsertElement");
        goto fail;
    }

    if (!(vector = LLVMBuildShuffleVector(comp_ctx->builder, vector, undef,
                                          zeros, "vector"))) {
        HANDLE_FAILURE("LLVMBuildShuffleVector");
        goto fail;
    }

    return vector;
fail:
    return NULL;
}

bool
aot_compile_simd_v128_load(AOTCompContext *comp_ctx,
                           AOTFuncContext *func_ctx,
                           uint32 align,
                           uint32 offset)
{
    LLVMValueRef result;

    if (!(result =
            simd_load(comp_ctx, func_ctx, align, offset, 16, V128_PTR_TYPE))) {
        goto fail;
    }

    PUSH_V128(result);
    return true;
fail:
    return false;
}

bool
aot_compile_simd_v128_store(AOTCompContext *comp_ctx,
                            AOTFuncContext *func_ctx,
                            uint32 align,
                            uint32 offset)
{
    LLVMValueRef maddr, value, result;

    POP_V128(value);

    if (!(maddr = aot_check_memory_overflow(comp_ctx, func_ctx, offset, 16)))
        return false;

    if (!(maddr = LLVMBuildBitCast(comp_ctx->builder, maddr, V128_PTR_TYPE,
                                   "data_ptr"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        goto fail;
    }

    if (!(result = LLVMBuildStore(comp_ctx->builder, value, maddr))) {
        HANDLE_FAILURE("LLVMBuildStore");
        goto fail;
    }

    LLVMSetAlignment(result, 1);

    return true;
fail:
    return false;
}

bool
aot_compile_simd_load_extend(AOTCompContext *comp_ctx,
                             AOTFuncContext *func_ctx,
                             uint8 load_opcode,
                             uint32 align,
                             uint32 offset)
{
    LLVMValueRef sub_vector, result;
    LLVMTypeRef sub_vector_type, vector_type;
    bool is_signed;
    uint32 data_length;

    switch (load_opcode) {
        case SIMD_i16x8_load8x8_s:
        case SIMD_i16x8_load8x8_u:
        {
            data_length = 8;
            vector_type = V128_i16x8_TYPE;
            is_signed = (load_opcode == SIMD_i16x8_load8x8_s);

            if (!(sub_vector_type = LLVMVectorType(INT8_TYPE, 8))) {
                HANDLE_FAILURE("LLVMVectorType");
                goto fail;
            }

            break;
        }
        case SIMD_i32x4_load16x4_s:
        case SIMD_i32x4_load16x4_u:
        {
            data_length = 8;
            vector_type = V128_i32x4_TYPE;
            is_signed = (load_opcode == SIMD_i32x4_load16x4_s);

            if (!(sub_vector_type = LLVMVectorType(INT16_TYPE, 4))) {
                HANDLE_FAILURE("LLVMVectorType");
                goto fail;
            }

            break;
        }
        case SIMD_i64x2_load32x2_s:
        case SIMD_i64x2_load32x2_u:
        {
            data_length = 8;
            vector_type = V128_i64x2_TYPE;
            is_signed = (load_opcode == SIMD_i64x2_load32x2_s);

            if (!(sub_vector_type = LLVMVectorType(I32_TYPE, 2))) {
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

    /* to vector ptr type */
    if (!(sub_vector_type = LLVMPointerType(sub_vector_type, 0))) {
        HANDLE_FAILURE("LLVMPointerType");
        goto fail;
    }

    if (!(sub_vector = simd_load(comp_ctx, func_ctx, align, offset,
                                 data_length, sub_vector_type))) {
        goto fail;
    }

    if (is_signed) {
        if (!(result = LLVMBuildSExt(comp_ctx->builder, sub_vector,
                                     vector_type, "vector"))) {
            HANDLE_FAILURE("LLVMBuildSExt");
            goto fail;
        }
    }
    else {
        if (!(result = LLVMBuildZExt(comp_ctx->builder, sub_vector,
                                     vector_type, "vector"))) {
            HANDLE_FAILURE("LLVMBuildZExt");
            goto fail;
        }
    }

    if (!(result = LLVMBuildBitCast(comp_ctx->builder, result, V128_i64x2_TYPE,
                                    "result"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        goto fail;
    }

    PUSH_V128(result);
    return true;
fail:
    return false;
}

bool
aot_compile_simd_load_splat(AOTCompContext *comp_ctx,
                            AOTFuncContext *func_ctx,
                            uint8 load_opcode,
                            uint32 align,
                            uint32 offset)
{
    LLVMValueRef element, result;
    LLVMTypeRef element_ptr_type, vector_type;
    unsigned data_length, lane_count;

    switch (load_opcode) {
        case SIMD_v8x16_load_splat:
            data_length = 1;
            lane_count = 16;
            element_ptr_type = INT8_PTR_TYPE;
            vector_type = V128_i8x16_TYPE;
            break;
        case SIMD_v16x8_load_splat:
            data_length = 2;
            lane_count = 8;
            element_ptr_type = INT16_PTR_TYPE;
            vector_type = V128_i16x8_TYPE;
            break;
        case SIMD_v32x4_load_splat:
            data_length = 4;
            lane_count = 4;
            element_ptr_type = INT32_PTR_TYPE;
            vector_type = V128_i32x4_TYPE;
            break;
        case SIMD_v64x2_load_splat:
            data_length = 8;
            lane_count = 2;
            element_ptr_type = INT64_PTR_TYPE;
            vector_type = V128_i64x2_TYPE;
            break;
        default:
            bh_assert(0);
            goto fail;
    }

    if (!(element = simd_load(comp_ctx, func_ctx, align, offset, data_length,
                              element_ptr_type))) {
        goto fail;
    }

    if (!(result = simd_splat(comp_ctx, func_ctx, element, vector_type,
                              lane_count))) {
        goto fail;
    }

    if (!(result = LLVMBuildBitCast(comp_ctx->builder, result, V128_i64x2_TYPE,
                                    "result"))) {
        HANDLE_FAILURE("LLVMBuildBitCast");
        goto fail;
    }

    PUSH_V128(result);
    return true;
fail:
    return false;
}
