/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_emit_const.h"

static LLVMValueRef
get_const_from_table(const AOTCompContext *comp_ctx, LLVMValueRef base,
                     int32 index, uint8 value_type)
{
    LLVMTypeRef const_ptr_type;
    LLVMValueRef const_index, const_addr, const_value;

    if (!(const_index = I32_CONST(index))) {
        aot_set_last_error("construct const index failed.");
        goto fail;
    }

    if (!(const_addr = LLVMBuildInBoundsGEP(
              comp_ctx->builder, base, &const_index, 1, "const_addr_tmp"))) {
        aot_set_last_error("get const addr by index failed.");
        goto fail;
    }

    switch (value_type) {
        case VALUE_TYPE_I32:
            const_ptr_type = INT32_PTR_TYPE;
            break;
        case VALUE_TYPE_I64:
            const_ptr_type = INT64_PTR_TYPE;
            break;
        case VALUE_TYPE_F32:
            const_ptr_type = F32_PTR_TYPE;
            break;
        case VALUE_TYPE_F64:
            const_ptr_type = F64_PTR_TYPE;
            break;
        default:
            bh_assert(0);
            goto fail;
    }

    if (!(const_addr = LLVMBuildBitCast(comp_ctx->builder, const_addr,
                                        const_ptr_type, "const_addr"))) {
        aot_set_last_error("cast const fialed.");
        goto fail;
    }

    if (!(const_value =
              LLVMBuildLoad(comp_ctx->builder, const_addr, "const_value"))) {
        aot_set_last_error("load const failed.");
        goto fail;
    }

    return const_value;
fail:
    return NULL;
}

bool
aot_compile_op_i32_const(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         int32 i32_const)
{
    LLVMValueRef value;
    char buf[128];
    int32 const_index;

    if (!comp_ctx->is_indirect_mode) {
        value = I32_CONST((uint32)i32_const);
        CHECK_LLVM_CONST(value);
        PUSH_I32(value);
    }
    else {
        snprintf(buf, sizeof(buf), "i32_const#%08X", i32_const);
        const_index = aot_get_native_symbol_index(comp_ctx, buf);
        if (const_index < 0) {
            return false;
        }
        value = get_const_from_table(comp_ctx, func_ctx->native_symbol,
                                     const_index, VALUE_TYPE_I32);
        if (!value) {
            return false;
        }
        PUSH_I32(value);
    }
    return true;
fail:
    return false;
}

bool
aot_compile_op_i64_const(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         int64 i64_const)
{
    LLVMValueRef value;
    char buf[128];
    int32 const_index;

    if (!comp_ctx->is_indirect_mode) {
        value = I64_CONST((uint64)i64_const);
        CHECK_LLVM_CONST(value);
        PUSH_I64(value);
    }
    else {
        snprintf(buf, sizeof(buf), "i64_const#%016" PRIx64, i64_const);
        const_index = aot_get_native_symbol_index(comp_ctx, buf);
        if (const_index < 0) {
            return false;
        }
        value = get_const_from_table(comp_ctx, func_ctx->native_symbol,
                                     const_index, VALUE_TYPE_I64);
        if (!value) {
            return false;
        }
        PUSH_I64(value);
    }
    return true;
fail:
    return false;
}

bool
aot_compile_op_f32_const(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         float32 f32_const)
{
    LLVMValueRef alloca, value;
    char buf[128];
    int32 i32_const;
    int32 const_index;

    if (!comp_ctx->is_indirect_mode) {
        if (!isnan(f32_const)) {
            value = F32_CONST(f32_const);
            CHECK_LLVM_CONST(value);
            PUSH_F32(value);
        }
        else {
            int32 i32_const;
            memcpy(&i32_const, &f32_const, sizeof(int32));
            if (!(alloca = LLVMBuildAlloca(comp_ctx->builder, I32_TYPE,
                                           "i32_ptr"))) {
                aot_set_last_error("llvm build alloca failed.");
                return false;
            }
            if (!LLVMBuildStore(comp_ctx->builder, I32_CONST((uint32)i32_const),
                                alloca)) {
                aot_set_last_error("llvm build store failed.");
                return false;
            }
            if (!(alloca = LLVMBuildBitCast(comp_ctx->builder, alloca,
                                            F32_PTR_TYPE, "f32_ptr"))) {
                aot_set_last_error("llvm build bitcast failed.");
                return false;
            }
            if (!(value = LLVMBuildLoad(comp_ctx->builder, alloca, ""))) {
                aot_set_last_error("llvm build load failed.");
                return false;
            }
            PUSH_F32(value);
        }
    }
    else {
        bh_memcpy_s(&i32_const, sizeof(int32), &f32_const, sizeof(float32));
        snprintf(buf, sizeof(buf), "i32_const#%08X", i32_const);
        const_index = aot_get_native_symbol_index(comp_ctx, buf);
        if (const_index < 0) {
            return false;
        }
        value = get_const_from_table(comp_ctx, func_ctx->native_symbol,
                                     const_index, VALUE_TYPE_F32);
        if (!value) {
            return false;
        }
        PUSH_F32(value);
    }

    return true;
fail:
    return false;
}

bool
aot_compile_op_f64_const(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         float64 f64_const)
{
    LLVMValueRef alloca, value;
    char buf[128];
    int64 i64_const;
    int32 const_index;

    if (!comp_ctx->is_indirect_mode) {
        if (!isnan(f64_const)) {
            value = F64_CONST(f64_const);
            CHECK_LLVM_CONST(value);
            PUSH_F64(value);
        }
        else {
            int64 i64_const;
            memcpy(&i64_const, &f64_const, sizeof(int64));
            if (!(alloca = LLVMBuildAlloca(comp_ctx->builder, I64_TYPE,
                                           "i64_ptr"))) {
                aot_set_last_error("llvm build alloca failed.");
                return false;
            }
            value = I64_CONST((uint64)i64_const);
            CHECK_LLVM_CONST(value);
            if (!LLVMBuildStore(comp_ctx->builder, value, alloca)) {
                aot_set_last_error("llvm build store failed.");
                return false;
            }
            if (!(alloca = LLVMBuildBitCast(comp_ctx->builder, alloca,
                                            F64_PTR_TYPE, "f64_ptr"))) {
                aot_set_last_error("llvm build bitcast failed.");
                return false;
            }
            if (!(value = LLVMBuildLoad(comp_ctx->builder, alloca, ""))) {
                aot_set_last_error("llvm build load failed.");
                return false;
            }
            PUSH_F64(value);
        }
    }
    else {
        bh_memcpy_s(&i64_const, sizeof(int64), &f64_const, sizeof(float64));
        snprintf(buf, sizeof(buf), "i64_const#%016" PRIx64, i64_const);
        const_index = aot_get_native_symbol_index(comp_ctx, buf);
        if (const_index < 0) {
            return false;
        }
        value = get_const_from_table(comp_ctx, func_ctx->native_symbol,
                                     const_index, VALUE_TYPE_F64);
        if (!value) {
            return false;
        }
        PUSH_F64(value);
    }

    return true;
fail:
    return false;
}
