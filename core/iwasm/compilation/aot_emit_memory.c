/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_emit_memory.h"
#include "aot_emit_exception.h"
#include "../aot/aot_runtime.h"

#define BUILD_ICMP(op, left, right, res, name) do {     \
    if (!(res = LLVMBuildICmp(comp_ctx->builder, op,    \
                              left, right, name))) {    \
        aot_set_last_error("llvm build icmp failed.");  \
        goto fail;                                      \
    }                                                   \
  } while (0)

#define BUILD_OP(Op, left, right, res, name) do {       \
    if (!(res = LLVMBuild##Op(comp_ctx->builder,        \
                              left, right, name))) {    \
        aot_set_last_error("llvm build " #Op " fail."); \
        goto fail;                                      \
    }                                                   \
  } while (0)

#define ADD_BASIC_BLOCK(block, name) do {                           \
    if (!(block = LLVMAppendBasicBlockInContext(comp_ctx->context,  \
                                                func_ctx->func,     \
                                                name))) {           \
        aot_set_last_error("llvm add basic block failed.");         \
        goto fail;                                                  \
    }                                                               \
  } while (0)

#define SET_BUILD_POS(block)    \
    LLVMPositionBuilderAtEnd(comp_ctx->builder, block)

static LLVMValueRef
get_memory_check_bound(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                       uint32 bytes)
{
    LLVMValueRef mem_check_bound = NULL;
    switch (bytes) {
        case 1:
            mem_check_bound = func_ctx->mem_bound_check_1byte;
            break;
        case 2:
            mem_check_bound = func_ctx->mem_bound_check_2bytes;
            break;
        case 4:
            mem_check_bound = func_ctx->mem_bound_check_4bytes;
            break;
        case 8:
            mem_check_bound = func_ctx->mem_bound_check_8bytes;
            break;
        default:
            bh_assert(0);
            return NULL;
    }

    if (func_ctx->mem_space_unchanged)
        return mem_check_bound;

    if (!(mem_check_bound = LLVMBuildLoad(comp_ctx->builder,
                                          mem_check_bound,
                                          "mem_check_bound"))) {
        aot_set_last_error("llvm build load failed.");
        return NULL;
    }
    return mem_check_bound;
}

static LLVMValueRef
check_memory_overflow(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                      uint32 offset, uint32 bytes)
{
    LLVMValueRef offset_const = I32_CONST(offset);
    LLVMValueRef bytes_const = I32_CONST(bytes);
    LLVMValueRef bytes64_const = I64_CONST(bytes);
    LLVMValueRef heap_base_offset = func_ctx->heap_base_offset;
    LLVMValueRef addr, maddr, offset1, offset2, cmp;
    LLVMValueRef mem_base_addr, mem_check_bound, total_mem_size;
    LLVMBasicBlockRef block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    LLVMBasicBlockRef check_succ, check_mem_space;
    AOTValue *aot_value;

    CHECK_LLVM_CONST(offset_const);
    CHECK_LLVM_CONST(bytes_const);
    CHECK_LLVM_CONST(bytes64_const);

    /* Get memory base address and memory data size */
    if (func_ctx->mem_space_unchanged) {
        mem_base_addr = func_ctx->mem_base_addr;
    }
    else {
        if (!(mem_base_addr = LLVMBuildLoad(comp_ctx->builder,
                                            func_ctx->mem_base_addr,
                                            "mem_base"))) {
            aot_set_last_error("llvm build load failed.");
            goto fail;
        }
    }

    aot_value = func_ctx->block_stack.block_list_end->value_stack.value_list_end;

    POP_I32(addr);
    /* offset1 = offset + addr; */
    BUILD_OP(Add, offset_const, addr, offset1, "offset1");

    /* return addres directly if constant offset and inside memory space */
    if (LLVMIsConstant(offset1)) {
        uint32 mem_offset = (uint32)LLVMConstIntGetZExtValue(offset1);
        uint32 num_bytes_per_page = comp_ctx->comp_data->num_bytes_per_page;
        uint32 init_page_count = comp_ctx->comp_data->mem_init_page_count;
        uint32 mem_data_size = num_bytes_per_page * init_page_count;
        if (mem_data_size > 0
            && mem_offset <= mem_data_size - bytes) {
            /* inside memory space */
            /* maddr = mem_base_addr + moffset */
            if (!(maddr = LLVMBuildInBoundsGEP(comp_ctx->builder,
                                               mem_base_addr,
                                               &offset1, 1, "maddr"))) {
                aot_set_last_error("llvm build add failed.");
                goto fail;
            }
            return maddr;
        }
    }

    if (comp_ctx->comp_data->mem_init_page_count == 0) {
        /* Get total memory size */
        if (func_ctx->mem_space_unchanged) {
            total_mem_size = func_ctx->total_mem_size;
        }
        else {
            if (!(total_mem_size = LLVMBuildLoad(comp_ctx->builder,
                                                 func_ctx->total_mem_size,
                                                 "total_mem_size"))) {
                aot_set_last_error("llvm build load failed.");
                goto fail;
            }
        }

        ADD_BASIC_BLOCK(check_mem_space, "check_mem_space");
        LLVMMoveBasicBlockAfter(check_mem_space, block_curr);

        /* if total_mem_size is zero, boundary check fail */
        BUILD_ICMP(LLVMIntEQ, total_mem_size, I32_ZERO, cmp,
                   "cmp_total_mem_size");
        if (!aot_emit_exception(comp_ctx, func_ctx,
                                EXCE_OUT_OF_BOUNDS_MEMORY_ACCESS,
                                true, cmp, check_mem_space)) {
            goto fail;
        }
        SET_BUILD_POS(check_mem_space);
    }

    if (!(aot_value->is_local
          && aot_checked_addr_list_find(func_ctx, aot_value->local_idx,
                                        offset, bytes))) {
        /* offset2 = offset1 - heap_base_offset; */
        BUILD_OP(Sub, offset1, heap_base_offset, offset2, "offset2");

        if (!(mem_check_bound =
                    get_memory_check_bound(comp_ctx, func_ctx, bytes))) {
            goto fail;
        }

        /* Add basic blocks */
        ADD_BASIC_BLOCK(check_succ, "check_succ");
        LLVMMoveBasicBlockAfter(check_succ, block_curr);

        /* offset2 > bound ? */
        BUILD_ICMP(LLVMIntUGT, offset2, mem_check_bound, cmp, "cmp");
        if (!aot_emit_exception(comp_ctx, func_ctx,
                                EXCE_OUT_OF_BOUNDS_MEMORY_ACCESS,
                                true, cmp, check_succ)) {
            goto fail;
        }

        SET_BUILD_POS(check_succ);

        if (aot_value->is_local) {
            if (!aot_checked_addr_list_add(func_ctx, aot_value->local_idx,
                                           offset, bytes))
                goto fail;
        }
    }

    /* maddr = mem_base_addr + offset1 */
    if (!(maddr = LLVMBuildInBoundsGEP(comp_ctx->builder, mem_base_addr,
                                       &offset1, 1, "maddr"))) {
        aot_set_last_error("llvm build add failed.");
        goto fail;
    }
    return maddr;
fail:
    return NULL;
}

#define BUILD_PTR_CAST(ptr_type) do {                       \
    if (!(maddr = LLVMBuildBitCast(comp_ctx->builder, maddr,\
                                   ptr_type, "data_ptr"))) {\
        aot_set_last_error("llvm build bit cast failed.");  \
        goto fail;                                          \
    }                                                       \
  } while (0)

#define BUILD_LOAD() do {                                   \
    if (!(value = LLVMBuildLoad(comp_ctx->builder, maddr,   \
                                "data"))) {                 \
        aot_set_last_error("llvm build load failed.");      \
        goto fail;                                          \
    }                                                       \
    LLVMSetAlignment(value, 1);                             \
  } while (0)

#define BUILD_TRUNC(data_type) do {                         \
    if (!(value = LLVMBuildTrunc(comp_ctx->builder, value,  \
                                 data_type, "val_trunc"))){ \
        aot_set_last_error("llvm build trunc failed.");     \
        goto fail;                                          \
    }                                                       \
  } while (0)

#define BUILD_STORE() do {                                  \
    LLVMValueRef res;                                       \
    if (!(res = LLVMBuildStore(comp_ctx->builder, value, maddr))) { \
        aot_set_last_error("llvm build store failed.");     \
        goto fail;                                          \
    }                                                       \
    LLVMSetAlignment(res, 1);                               \
  } while (0)

#define BUILD_SIGN_EXT(dst_type) do {                       \
    if (!(value = LLVMBuildSExt(comp_ctx->builder, value,   \
                                dst_type, "data_s_ext"))) { \
        aot_set_last_error("llvm build sign ext failed.");  \
        goto fail;                                          \
    }                                                       \
  } while (0)

#define BUILD_ZERO_EXT(dst_type) do {                       \
    if (!(value = LLVMBuildZExt(comp_ctx->builder, value,   \
                                dst_type, "data_z_ext"))) { \
        aot_set_last_error("llvm build zero ext failed.");  \
        goto fail;                                          \
    }                                                       \
  } while (0)

bool
aot_compile_op_i32_load(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                        uint32 align, uint32 offset, uint32 bytes, bool sign)
{
    LLVMValueRef maddr, value = NULL;

    if (!(maddr = check_memory_overflow(comp_ctx, func_ctx, offset, bytes)))
        return false;

    switch (bytes) {
        case 4:
            BUILD_PTR_CAST(INT32_PTR_TYPE);
            BUILD_LOAD();
            break;
        case 2:
        case 1:
            if (bytes == 2)
                BUILD_PTR_CAST(INT16_PTR_TYPE);
            else
                BUILD_PTR_CAST(INT8_PTR_TYPE);
            BUILD_LOAD();
            if (sign)
                BUILD_SIGN_EXT(I32_TYPE);
            else
                BUILD_ZERO_EXT(I32_TYPE);
            break;
        default:
            bh_assert(0);
            break;
    }

    PUSH_I32(value);
    return true;
fail:
    return false;
}

bool
aot_compile_op_i64_load(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                        uint32 align, uint32 offset, uint32 bytes, bool sign)
{
    LLVMValueRef maddr, value = NULL;

    if (!(maddr = check_memory_overflow(comp_ctx, func_ctx, offset, bytes)))
        return false;

    switch (bytes) {
        case 8:
            BUILD_PTR_CAST(INT64_PTR_TYPE);
            BUILD_LOAD();
            break;
        case 4:
        case 2:
        case 1:
            if (bytes == 4)
                BUILD_PTR_CAST(INT32_PTR_TYPE);
            else if (bytes == 2)
                BUILD_PTR_CAST(INT16_PTR_TYPE);
            else
                BUILD_PTR_CAST(INT8_PTR_TYPE);
            BUILD_LOAD();
            if (sign)
                BUILD_SIGN_EXT(I64_TYPE);
            else
                BUILD_ZERO_EXT(I64_TYPE);
            break;
        default:
            bh_assert(0);
            break;
    }

    PUSH_I64(value);
    return true;
fail:
    return false;
}

bool
aot_compile_op_f32_load(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                        uint32 align, uint32 offset)
{
    LLVMValueRef maddr, value;

    if (!(maddr = check_memory_overflow(comp_ctx, func_ctx, offset, 4)))
        return false;

    BUILD_PTR_CAST(F32_PTR_TYPE);
    BUILD_LOAD();
    PUSH_F32(value);
    return true;
fail:
    return false;
}

bool
aot_compile_op_f64_load(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                        uint32 align, uint32 offset)
{
    LLVMValueRef maddr, value;

    if (!(maddr = check_memory_overflow(comp_ctx, func_ctx, offset, 8)))
        return false;

    BUILD_PTR_CAST(F64_PTR_TYPE);
    BUILD_LOAD();
    PUSH_F64(value);
    return true;
fail:
    return false;
}

bool
aot_compile_op_i32_store(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 align, uint32 offset, uint32 bytes)
{
    LLVMValueRef maddr, value;

    POP_I32(value);

    if (!(maddr = check_memory_overflow(comp_ctx, func_ctx, offset, bytes)))
        return false;

    switch (bytes) {
        case 4:
            BUILD_PTR_CAST(INT32_PTR_TYPE);
            break;
        case 2:
            BUILD_PTR_CAST(INT16_PTR_TYPE);
            BUILD_TRUNC(INT16_TYPE);
            break;
        case 1:
            BUILD_PTR_CAST(INT8_PTR_TYPE);
            BUILD_TRUNC(INT8_TYPE);
            break;
        default:
            bh_assert(0);
            break;
    }

    BUILD_STORE();
    return true;
fail:
    return false;
}

bool
aot_compile_op_i64_store(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 align, uint32 offset, uint32 bytes)
{
    LLVMValueRef maddr, value;

    POP_I64(value);

    if (!(maddr = check_memory_overflow(comp_ctx, func_ctx, offset, bytes)))
        return false;

    switch (bytes) {
        case 8:
            BUILD_PTR_CAST(INT64_PTR_TYPE);
            break;
        case 4:
            BUILD_PTR_CAST(INT32_PTR_TYPE);
            BUILD_TRUNC(I32_TYPE);
            break;
        case 2:
            BUILD_PTR_CAST(INT16_PTR_TYPE);
            BUILD_TRUNC(INT16_TYPE);
            break;
        case 1:
            BUILD_PTR_CAST(INT8_PTR_TYPE);
            BUILD_TRUNC(INT8_TYPE);
            break;
        default:
            bh_assert(0);
            break;
    }

    BUILD_STORE();
    return true;
fail:
    return false;
}

bool
aot_compile_op_f32_store(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 align, uint32 offset)
{
    LLVMValueRef maddr, value;

    POP_F32(value);

    if (!(maddr = check_memory_overflow(comp_ctx, func_ctx, offset, 4)))
        return false;

    BUILD_PTR_CAST(F32_PTR_TYPE);
    BUILD_STORE();
    return true;
fail:
    return false;
}

bool
aot_compile_op_f64_store(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 align, uint32 offset)
{
    LLVMValueRef maddr, value;

    POP_F64(value);

    if (!(maddr = check_memory_overflow(comp_ctx, func_ctx, offset, 8)))
        return false;

    BUILD_PTR_CAST(F64_PTR_TYPE);
    BUILD_STORE();
    return true;
fail:
    return false;
}

static LLVMValueRef
get_memory_size(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    uint32 offset = offsetof(AOTModuleInstance, mem_cur_page_count);
    LLVMValueRef mem_size_offset, mem_size_ptr, mem_size;

    /* mem_size_offset = aot_inst + offset */
    mem_size_offset = I32_CONST(offset);
    CHECK_LLVM_CONST(mem_size_offset);
    if (!(mem_size_ptr = LLVMBuildInBoundsGEP(comp_ctx->builder,
                                              func_ctx->aot_inst,
                                              &mem_size_offset, 1,
                                              "mem_size_ptr_tmp"))) {
        aot_set_last_error("llvm build inbounds gep failed.");
        return NULL;
    }

    /* cast to int32* */
    if (!(mem_size_ptr = LLVMBuildBitCast(comp_ctx->builder, mem_size_ptr,
                                          INT32_PTR_TYPE, "mem_size_ptr"))) {
        aot_set_last_error("llvm build bitcast failed.");
        return NULL;
    }

    /* load memory size, or current page count */
    if (!(mem_size = LLVMBuildLoad(comp_ctx->builder,
                                   mem_size_ptr, "mem_size"))) {
        aot_set_last_error("llvm build load failed.");
        return NULL;
    }

    return mem_size;
fail:
    return NULL;
}

bool
aot_compile_op_memory_size(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef mem_size = get_memory_size(comp_ctx, func_ctx);

    if (mem_size)
        PUSH_I32(mem_size);
    return mem_size ? true : false;
fail:
    return false;
}

bool
aot_compile_op_memory_grow(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef mem_size = get_memory_size(comp_ctx, func_ctx);
    LLVMValueRef delta, param_values[2], ret_value, func, value;
    LLVMTypeRef param_types[2], ret_type, func_type, func_ptr_type;

    if (!mem_size)
        return false;

    POP_I32(delta);

    /* Function type of wasm_runtime_enlarge_memory() */
    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    ret_type = INT8_TYPE;

    if (!(func_type = LLVMFunctionType(ret_type, param_types, 2, false))) {
        aot_set_last_error("llvm add function type failed.");
        return false;
    }

    if (comp_ctx->is_jit_mode) {
        /* JIT mode, call the function directly */
        if (!(func_ptr_type = LLVMPointerType(func_type, 0))) {
            aot_set_last_error("llvm add pointer type failed.");
            return false;
        }
        if (!(value = I64_CONST((uint64)(uintptr_t)wasm_runtime_enlarge_memory))
            || !(func = LLVMConstIntToPtr(value, func_ptr_type))) {
            aot_set_last_error("create LLVM value failed.");
            return false;
        }
    }
    else {
        char *func_name = "wasm_runtime_enlarge_memory";
        /* AOT mode, delcare the function */
        if (!(func = LLVMGetNamedFunction(comp_ctx->module, func_name))
            && !(func = LLVMAddFunction(comp_ctx->module,
                                        func_name, func_type))) {
            aot_set_last_error("llvm add function failed.");
            return false;
        }
    }

    /* Call function wasm_runtime_enlarge_memory() */
    param_values[0] = func_ctx->aot_inst;
    param_values[1] = delta;
    if (!(ret_value = LLVMBuildCall(comp_ctx->builder, func,
                                    param_values, 2, "call"))) {
        aot_set_last_error("llvm build call failed.");
        return false;
    }

    BUILD_ICMP(LLVMIntUGT, ret_value, I8_ZERO, ret_value, "mem_grow_ret");

    /* ret_value = ret_value == true ? delta : pre_page_count */
    if (!(ret_value = LLVMBuildSelect(comp_ctx->builder, ret_value,
                                      mem_size, I32_NEG_ONE,
                                      "mem_grow_ret"))) {
        aot_set_last_error("llvm build select failed.");
        return false;
    }

    PUSH_I32(ret_value);

    /* To be simple, call wasm_runtime_set_exception() no matter
       enlarge success or not */
    param_types[1] = INT8_PTR_TYPE;
    ret_type = VOID_TYPE;
    if (!(func_type = LLVMFunctionType(ret_type, param_types, 2, false))) {
        aot_set_last_error("llvm add function type failed.");
        return false;
    }

    if (comp_ctx->is_jit_mode) {
        /* JIT mode, call the function directly */
        if (!(func_ptr_type = LLVMPointerType(func_type, 0))) {
            aot_set_last_error("llvm add pointer type failed.");
            return false;
        }
        if (!(value = I64_CONST((uint64)(uintptr_t)wasm_runtime_set_exception))
            || !(func = LLVMConstIntToPtr(value, func_ptr_type))) {
            aot_set_last_error("create LLVM value failed.");
            return false;
        }
    }
    else {
        char *func_name = "wasm_runtime_set_exception";
        /* AOT mode, delcare the function */
        if (!(func = LLVMGetNamedFunction(comp_ctx->module, func_name))
            && !(func = LLVMAddFunction(comp_ctx->module,
                                        func_name, func_type))) {
           aot_set_last_error("llvm add function failed.");
           return false;
       }
    }

    /* Call function wasm_runtime_set_exception(aot_inst, NULL) */
    param_values[1] = LLVMConstNull(INT8_PTR_TYPE);
    CHECK_LLVM_CONST(param_values[1]);
    if (!(LLVMBuildCall(comp_ctx->builder, func, param_values, 2, ""))) {
        aot_set_last_error("llvm build call failed.");
        return false;
    }

    return true;
fail:
    return false;
}

