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

#define BUILD_COND_BR(cmp_val, then_block, else_block) do { \
    if (!LLVMBuildCondBr(comp_ctx->builder, cmp_val,        \
                         then_block, else_block)) {         \
        aot_set_last_error("llvm build cond br failed.");   \
        goto fail;                                          \
    }                                                       \
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
check_memory_overflow(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                      uint32 offset, uint32 bytes)
{
    LLVMValueRef offset_const = I32_CONST(offset);
    LLVMValueRef size_const = I32_CONST(bytes);
    LLVMValueRef addr, maddr, moffset;
    LLVMValueRef cmp, phi;
    LLVMValueRef mem_base_addr, mem_data_size;
    LLVMValueRef heap_base_addr, heap_base_offset;
    LLVMValueRef mem_offset_max = NULL, heap_offset_max = NULL;
    LLVMBasicBlockRef check_mem_space, check_heap_space, check_succ;
    LLVMBasicBlockRef block_curr = LLVMGetInsertBlock(comp_ctx->builder);

    CHECK_LLVM_CONST(offset_const);
    CHECK_LLVM_CONST(size_const);

    heap_base_addr = func_ctx->heap_base_addr;
    heap_base_offset = func_ctx->heap_base_offset;

    POP_I32(addr);
    BUILD_OP(Add, offset_const, addr, moffset, "moffset");

    /* return addres directly if constant offset and inside memory space */
    if (LLVMIsConstant(moffset)) {
        uint32 memory_offset = (uint32)LLVMConstIntGetZExtValue(moffset);
        uint32 init_page_count = comp_ctx->comp_data->mem_init_page_count;
        if (init_page_count > 0
            && memory_offset <= comp_ctx->comp_data->num_bytes_per_page
                                * init_page_count - bytes) {
            /* inside memory space */
            if (!func_ctx->mem_space_unchanged) {
                if (!(mem_base_addr = LLVMBuildLoad(comp_ctx->builder,
                                func_ctx->mem_base_addr,
                                "mem_base"))) {
                    aot_set_last_error("llvm build load failed.");
                    return NULL;
                }
            }
            else {
                mem_base_addr = func_ctx->mem_base_addr;
            }

            /* maddr = mem_base_addr + moffset */
            if (!(maddr = LLVMBuildInBoundsGEP(comp_ctx->builder,
                                               mem_base_addr,
                                               &moffset, 1, "maddr"))) {
                aot_set_last_error("llvm build add failed.");
                goto fail;
            }
            return maddr;
        }
    }

    /* Add basic blocks */
    ADD_BASIC_BLOCK(check_heap_space, "check_heap_space");
    ADD_BASIC_BLOCK(check_succ, "check_succ");

    LLVMMoveBasicBlockAfter(check_heap_space, block_curr);
    LLVMMoveBasicBlockAfter(check_succ, check_heap_space);

    /* Add return maddress phi for check_succ block */
    SET_BUILD_POS(check_succ);
    if (!(phi = LLVMBuildPhi(comp_ctx->builder,
                             INT8_PTR_TYPE, "maddr_phi"))) {
        aot_set_last_error("llvm build phi failed.");
        goto fail;
    }
    SET_BUILD_POS(block_curr);

    /* Get memory data size */
    if (!func_ctx->mem_space_unchanged) {
        if (!(mem_data_size = LLVMBuildLoad(comp_ctx->builder,
                                            func_ctx->mem_data_size,
                                            "mem_data_size"))) {
            aot_set_last_error("llvm build load failed.");
            return NULL;
        }
    }
    else {
        mem_data_size = func_ctx->mem_data_size;
    }

    if (comp_ctx->comp_data->mem_init_page_count == 0) {
        ADD_BASIC_BLOCK(check_mem_space, "check_mem_space");
        LLVMMoveBasicBlockAfter(check_mem_space, block_curr);

        /* if mem_data_size is zero, check heap space */
        BUILD_ICMP(LLVMIntEQ, mem_data_size, I32_ZERO, cmp,
                   "cmp_mem_data_size");
        BUILD_COND_BR(cmp, check_heap_space, check_mem_space);
        SET_BUILD_POS(check_mem_space);
    }

    /* Get memory base address */
    if (!func_ctx->mem_space_unchanged) {
        if (!(mem_base_addr = LLVMBuildLoad(comp_ctx->builder,
                                            func_ctx->mem_base_addr,
                                            "mem_base"))) {
            aot_set_last_error("llvm build load failed.");
            return NULL;
        }
    }
    else {
        mem_base_addr = func_ctx->mem_base_addr;
    }

    /* maddr = mem_base_addr + moffset */
    if (!(maddr = LLVMBuildInBoundsGEP(comp_ctx->builder, mem_base_addr,
                                       &moffset, 1, "maddr"))) {
        aot_set_last_error("llvm build add failed.");
        goto fail;
    }
    block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    LLVMAddIncoming(phi, &maddr, &block_curr, 1);

    if (!func_ctx->mem_space_unchanged) {
        /* mem_offset_max = mem_data_size - bytes to load/read */
        if (!(mem_offset_max = LLVMBuildSub(comp_ctx->builder,
                                            mem_data_size, size_const,
                                            "mem_offset_max"))) {
            aot_set_last_error("llvm build sub failed.");
            goto fail;
        }
    }
    else {
        if (bytes == 1)
            mem_offset_max = func_ctx->mem_bound_1_byte;
        else if (bytes == 2)
            mem_offset_max = func_ctx->mem_bound_2_bytes;
        else if (bytes == 4)
            mem_offset_max = func_ctx->mem_bound_4_bytes;
        else if (bytes == 8)
            mem_offset_max = func_ctx->mem_bound_8_bytes;
    }

    /* in linear memory if (uint32)moffset <= (uint32)mem_offset_max,
       else check heap space */
    BUILD_ICMP(LLVMIntULE, moffset, mem_offset_max, cmp, "cmp_mem_offset");

    /* Create condtion br */
    BUILD_COND_BR(cmp, check_succ, check_heap_space);

    /* Start to translate the check_heap_space block */
    SET_BUILD_POS(check_heap_space);

    /* moffset -= heap_base_offset */
    if (!(moffset = LLVMBuildSub(comp_ctx->builder,
                                 moffset, heap_base_offset,
                                 "moffset_to_heap"))) {
        aot_set_last_error("llvm build sub failed.");
        goto fail;
    }

    /* maddr = heap_base_addr + moffset */
    if (!(maddr = LLVMBuildInBoundsGEP(comp_ctx->builder, heap_base_addr,
                                       &moffset, 1, "maddr"))) {
        aot_set_last_error("llvm build add failed.");
        goto fail;
    }
    block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    LLVMAddIncoming(phi, &maddr, &block_curr, 1);

    /* heap space base addr and size is unchanged,
       the heap boundary is unchanged also. */
    if (bytes == 1)
        heap_offset_max = func_ctx->heap_bound_1_byte;
    else if (bytes == 2)
        heap_offset_max = func_ctx->heap_bound_2_bytes;
    else if (bytes == 4)
        heap_offset_max = func_ctx->heap_bound_4_bytes;
    else if (bytes == 8)
        heap_offset_max = func_ctx->heap_bound_8_bytes;

    /* in heap space if (uint32)moffset <= (uint32)heap_offset_max,
       else throw exception */
    BUILD_ICMP(LLVMIntUGT, moffset, heap_offset_max, cmp, "cmp_heap_offset");
    if (!aot_emit_exception(comp_ctx, func_ctx,
                            EXCE_OUT_OF_BOUNDS_MEMORY_ACCESS,
                            true, cmp, check_succ))
        goto fail;

    SET_BUILD_POS(check_succ);
    return phi;
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
    param_types[1] = VOID_PTR_TYPE;
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
    param_values[1] = LLVMConstNull(VOID_PTR_TYPE);
    CHECK_LLVM_CONST(param_values[1]);
    if (!(LLVMBuildCall(comp_ctx->builder, func, param_values, 2, ""))) {
        aot_set_last_error("llvm build call failed.");
        return false;
    }

    return true;
fail:
    return false;
}

