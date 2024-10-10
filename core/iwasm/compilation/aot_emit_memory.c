/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_emit_memory.h"
#include "aot_compiler.h"
#include "aot_emit_exception.h"
#include "../aot/aot_runtime.h"
#include "aot_intrinsic.h"
#include "aot_emit_control.h"

#define BUILD_ICMP(op, left, right, res, name)                                \
    do {                                                                      \
        if (!(res =                                                           \
                  LLVMBuildICmp(comp_ctx->builder, op, left, right, name))) { \
            aot_set_last_error("llvm build icmp failed.");                    \
            goto fail;                                                        \
        }                                                                     \
    } while (0)

#define BUILD_OP(Op, left, right, res, name)                                \
    do {                                                                    \
        if (!(res = LLVMBuild##Op(comp_ctx->builder, left, right, name))) { \
            aot_set_last_error("llvm build " #Op " fail.");                 \
            goto fail;                                                      \
        }                                                                   \
    } while (0)

#define ADD_BASIC_BLOCK(block, name)                                          \
    do {                                                                      \
        if (!(block = LLVMAppendBasicBlockInContext(comp_ctx->context,        \
                                                    func_ctx->func, name))) { \
            aot_set_last_error("llvm add basic block failed.");               \
            goto fail;                                                        \
        }                                                                     \
    } while (0)

#define SET_BUILD_POS(block) LLVMPositionBuilderAtEnd(comp_ctx->builder, block)

static bool
zero_extend_u64(AOTCompContext *comp_ctx, LLVMValueRef *value, const char *name)
{
    if (comp_ctx->pointer_size == sizeof(uint64)) {
        /* zero extend to uint64 if the target is 64-bit */
        *value = LLVMBuildZExt(comp_ctx->builder, *value, I64_TYPE, name);
        if (!*value) {
            aot_set_last_error("llvm build zero extend failed.");
            return false;
        }
    }
    return true;
}

static LLVMValueRef
get_memory_check_bound(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                       uint32 bytes)
{
    LLVMValueRef mem_check_bound = NULL;
    switch (bytes) {
        case 1:
            mem_check_bound = func_ctx->mem_info[0].mem_bound_check_1byte;
            break;
        case 2:
            mem_check_bound = func_ctx->mem_info[0].mem_bound_check_2bytes;
            break;
        case 4:
            mem_check_bound = func_ctx->mem_info[0].mem_bound_check_4bytes;
            break;
        case 8:
            mem_check_bound = func_ctx->mem_info[0].mem_bound_check_8bytes;
            break;
        case 16:
            mem_check_bound = func_ctx->mem_info[0].mem_bound_check_16bytes;
            break;
        default:
            bh_assert(0);
            return NULL;
    }

    if (func_ctx->mem_space_unchanged)
        return mem_check_bound;

    if (!(mem_check_bound = LLVMBuildLoad2(
              comp_ctx->builder,
              (comp_ctx->pointer_size == sizeof(uint64)) ? I64_TYPE : I32_TYPE,
              mem_check_bound, "mem_check_bound"))) {
        aot_set_last_error("llvm build load failed.");
        return NULL;
    }
    return mem_check_bound;
}

#if defined(_WIN32) || defined(_WIN32_)
static inline int
ffs(int n)
{
    int pos = 0;

    if (n == 0)
        return 0;

    while (!(n & 1)) {
        pos++;
        n >>= 1;
    }
    return pos + 1;
}
#endif

static LLVMValueRef
get_memory_curr_page_count(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx);

LLVMValueRef
aot_check_memory_overflow(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          mem_offset_t offset, uint32 bytes, bool enable_segue,
                          unsigned int *alignp)
{
    LLVMValueRef offset_const =
        MEMORY64_COND_VALUE(I64_CONST(offset), I32_CONST(offset));
    LLVMValueRef addr, maddr, maddr_phi = NULL, offset1, cmp1, cmp2, cmp;
    LLVMValueRef mem_base_addr, mem_check_bound;
    LLVMBasicBlockRef block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    LLVMBasicBlockRef check_succ, block_maddr_phi = NULL;
    AOTValue *aot_value_top;
    uint32 local_idx_of_aot_value = 0;
    uint64 const_value;
    bool is_target_64bit, is_local_of_aot_value = false;
    bool is_const = false;
#if WASM_ENABLE_SHARED_MEMORY != 0
    bool is_shared_memory =
        comp_ctx->comp_data->memories[0].flags & SHARED_MEMORY_FLAG;
#endif
#if WASM_ENABLE_MEMORY64 == 0
    bool is_memory64 = false;
#else
    bool is_memory64 = IS_MEMORY64;
#endif

    is_target_64bit = (comp_ctx->pointer_size == sizeof(uint64)) ? true : false;

    if (comp_ctx->is_indirect_mode
        && aot_intrinsic_check_capability(
            comp_ctx, MEMORY64_COND_VALUE("i64.const", "i32.const"))) {
        WASMValue wasm_value;
#if WASM_ENABLE_MEMORY64 != 0
        if (IS_MEMORY64) {
            wasm_value.i64 = offset;
        }
        else
#endif
        {
            wasm_value.i32 = (int32)offset;
        }
        offset_const = aot_load_const_from_table(
            comp_ctx, func_ctx->native_symbol, &wasm_value,
            MEMORY64_COND_VALUE(VALUE_TYPE_I64, VALUE_TYPE_I32));
        if (!offset_const) {
            return NULL;
        }
    }
    else {
        CHECK_LLVM_CONST(offset_const);
    }

    /* Get memory base address and memory data size */
    if (func_ctx->mem_space_unchanged
#if WASM_ENABLE_SHARED_MEMORY != 0
        || is_shared_memory
#endif
    ) {
        mem_base_addr = func_ctx->mem_info[0].mem_base_addr;
    }
    else {
        if (!(mem_base_addr = LLVMBuildLoad2(
                  comp_ctx->builder, OPQ_PTR_TYPE,
                  func_ctx->mem_info[0].mem_base_addr, "mem_base"))) {
            aot_set_last_error("llvm build load failed.");
            goto fail;
        }
    }

    aot_value_top =
        func_ctx->block_stack.block_list_end->value_stack.value_list_end;
    if (aot_value_top) {
        /* aot_value_top is freed in the following POP_I32(addr),
           so save its fields here for further use */
        is_local_of_aot_value = aot_value_top->is_local;
        is_const = aot_value_top->is_const;
        local_idx_of_aot_value = aot_value_top->local_idx;
        const_value = aot_value_top->const_value;
    }

    POP_MEM_OFFSET(addr);

    /*
     * Note: not throw the integer-overflow-exception here since it must
     * have been thrown when converting float to integer before
     */
    /* return address directly if constant offset and inside memory space */
    if (LLVMIsEfficientConstInt(addr) || is_const) {
        uint64 value;
        if (LLVMIsEfficientConstInt(addr)) {
            value = (uint64)LLVMConstIntGetZExtValue(addr);
        }
        else {
            value = const_value;
        }
        uint64 mem_offset = value + (uint64)offset;
        uint32 num_bytes_per_page =
            comp_ctx->comp_data->memories[0].num_bytes_per_page;
        uint32 init_page_count =
            comp_ctx->comp_data->memories[0].init_page_count;
        uint64 mem_data_size = (uint64)num_bytes_per_page * init_page_count;

        if (alignp != NULL) {
            /*
             * A note about max_align below:
             * the assumption here is the base address of a linear memory
             * has the natural alignment. for platforms using mmap, it can
             * be even larger. for now, use a conservative value.
             */
            const unsigned int max_align = 8;
            int shift = ffs((int)(unsigned int)mem_offset);
            if (shift == 0) {
                *alignp = max_align;
            }
            else {
                unsigned int align = 1 << (shift - 1);
                if (align > max_align) {
                    align = max_align;
                }
                *alignp = align;
            }
        }
        if (mem_offset + bytes <= mem_data_size) {
            /* inside memory space */
            if (comp_ctx->pointer_size == sizeof(uint64))
                offset1 = I64_CONST(mem_offset);
            else
                offset1 = I32_CONST((uint32)mem_offset);
            CHECK_LLVM_CONST(offset1);
            if (!enable_segue) {
                if (!(maddr = LLVMBuildInBoundsGEP2(comp_ctx->builder,
                                                    INT8_TYPE, mem_base_addr,
                                                    &offset1, 1, "maddr"))) {
                    aot_set_last_error("llvm build add failed.");
                    goto fail;
                }
            }
            else {
                if (!(maddr = LLVMBuildIntToPtr(comp_ctx->builder, offset1,
                                                INT8_PTR_TYPE_GS, "maddr"))) {
                    aot_set_last_error("llvm build IntToPtr failed.");
                    goto fail;
                }
            }
            return maddr;
        }
    }
    else if (alignp != NULL) {
        *alignp = 1;
    }

    if (is_target_64bit) {
        if (!(offset_const = LLVMBuildZExt(comp_ctx->builder, offset_const,
                                           I64_TYPE, "offset_i64"))
            || !(addr = LLVMBuildZExt(comp_ctx->builder, addr, I64_TYPE,
                                      "addr_i64"))) {
            aot_set_last_error("llvm build zero extend failed.");
            goto fail;
        }
    }

    /* offset1 = offset + addr; */
    /* TODO: check whether integer overflow occurs when memory is 64-bit
             and boundary check is enabled */
    BUILD_OP(Add, offset_const, addr, offset1, "offset1");

    if (comp_ctx->enable_shared_heap /* TODO: && mem_idx == 0 */) {
        LLVMBasicBlockRef app_addr_in_shared_heap, app_addr_in_linear_mem;
        LLVMValueRef is_in_shared_heap, shared_heap_check_bound = NULL;

        /* Add basic blocks */
        ADD_BASIC_BLOCK(app_addr_in_shared_heap, "app_addr_in_shared_heap");
        ADD_BASIC_BLOCK(app_addr_in_linear_mem, "app_addr_in_linear_mem");
        ADD_BASIC_BLOCK(block_maddr_phi, "maddr_phi");

        LLVMMoveBasicBlockAfter(app_addr_in_shared_heap, block_curr);
        LLVMMoveBasicBlockAfter(app_addr_in_linear_mem,
                                app_addr_in_shared_heap);
        LLVMMoveBasicBlockAfter(block_maddr_phi, app_addr_in_linear_mem);

        LLVMPositionBuilderAtEnd(comp_ctx->builder, block_maddr_phi);
        if (!(maddr_phi =
                  LLVMBuildPhi(comp_ctx->builder,
                               enable_segue ? INT8_PTR_TYPE_GS : INT8_PTR_TYPE,
                               "maddr_phi"))) {
            aot_set_last_error("llvm build phi failed");
            goto fail;
        }

        LLVMPositionBuilderAtEnd(comp_ctx->builder, block_curr);

        if (!is_target_64bit) {
            /* Check whether interger overflow occurs in addr + offset */
            LLVMBasicBlockRef check_integer_overflow_end;
            ADD_BASIC_BLOCK(check_integer_overflow_end,
                            "check_integer_overflow_end");
            LLVMMoveBasicBlockAfter(check_integer_overflow_end, block_curr);

            BUILD_ICMP(LLVMIntULT, offset1, addr, cmp1, "cmp1");
            if (!aot_emit_exception(comp_ctx, func_ctx,
                                    EXCE_OUT_OF_BOUNDS_MEMORY_ACCESS, true,
                                    cmp1, check_integer_overflow_end)) {
                goto fail;
            }
            SET_BUILD_POS(check_integer_overflow_end);
        }

        shared_heap_check_bound =
            is_memory64 ? I64_CONST(UINT64_MAX - bytes + 1)
                        : (comp_ctx->pointer_size == sizeof(uint64)
                               ? I64_CONST(UINT32_MAX - bytes + 1)
                               : I32_CONST(UINT32_MAX - bytes + 1));
        CHECK_LLVM_CONST(shared_heap_check_bound);

        /* Check whether the bytes to access are in shared heap */
        if (!comp_ctx->enable_bound_check) {
            /* Use IntUGT but not IntUGE to compare, since (1) in the ems
               memory allocator, the hmu node includes hmu header and hmu
               memory, only the latter is returned to the caller as the
               allocated memory, the hmu header isn't returned so the
               first byte of the shared heap won't be accesed, (2) using
               IntUGT gets better performance than IntUGE in some cases */
            BUILD_ICMP(LLVMIntUGT, offset1, func_ctx->shared_heap_start_off,
                       is_in_shared_heap, "is_in_shared_heap");
            /* We don't check the shared heap's upper boundary if boundary
               check isn't enabled, the runtime may also use the guard pages
               of shared heap to check the boundary if hardware boundary
               check feature is enabled. */
        }
        else {
            /* Use IntUGT but not IntUGE to compare, same as above */
            BUILD_ICMP(LLVMIntUGT, offset1, func_ctx->shared_heap_start_off,
                       cmp1, "cmp1");
            /* Check the shared heap's upper boundary if boundary check is
               enabled */
            BUILD_ICMP(LLVMIntULE, offset1, shared_heap_check_bound, cmp2,
                       "cmp2");
            BUILD_OP(And, cmp1, cmp2, is_in_shared_heap, "is_in_shared_heap");
        }

        if (!LLVMBuildCondBr(comp_ctx->builder, is_in_shared_heap,
                             app_addr_in_shared_heap, app_addr_in_linear_mem)) {
            aot_set_last_error("llvm build cond br failed");
            goto fail;
        }

        LLVMPositionBuilderAtEnd(comp_ctx->builder, app_addr_in_shared_heap);

        /* Get native address inside shared heap */
        if (!(maddr =
                  LLVMBuildInBoundsGEP2(comp_ctx->builder, INT8_TYPE,
                                        func_ctx->shared_heap_base_addr_adj,
                                        &offset1, 1, "maddr_shared_heap"))) {
            aot_set_last_error("llvm build inbounds gep failed");
            goto fail;
        }

        if (enable_segue) {
            LLVMValueRef mem_base_addr_u64, maddr_u64, offset_to_mem_base;

            if (!(maddr_u64 = LLVMBuildPtrToInt(comp_ctx->builder, maddr,
                                                I64_TYPE, "maddr_u64"))
                || !(mem_base_addr_u64 =
                         LLVMBuildPtrToInt(comp_ctx->builder, mem_base_addr,
                                           I64_TYPE, "mem_base_addr_u64"))) {
                aot_set_last_error("llvm build ptr to int failed");
                goto fail;
            }
            if (!(offset_to_mem_base =
                      LLVMBuildSub(comp_ctx->builder, maddr_u64,
                                   mem_base_addr_u64, "offset_to_mem_base"))) {
                aot_set_last_error("llvm build sub failed");
                goto fail;
            }
            if (!(maddr = LLVMBuildIntToPtr(
                      comp_ctx->builder, offset_to_mem_base, INT8_PTR_TYPE_GS,
                      "maddr_shared_heap_segue"))) {
                aot_set_last_error("llvm build int to ptr failed.");
                goto fail;
            }
        }

        LLVMAddIncoming(maddr_phi, &maddr, &app_addr_in_shared_heap, 1);

        if (!LLVMBuildBr(comp_ctx->builder, block_maddr_phi)) {
            aot_set_last_error("llvm build br failed");
            goto fail;
        }

        LLVMPositionBuilderAtEnd(comp_ctx->builder, app_addr_in_linear_mem);
        block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    }

    if (comp_ctx->enable_bound_check
        && !(is_local_of_aot_value
             && aot_checked_addr_list_find(func_ctx, local_idx_of_aot_value,
                                           offset, bytes))) {
        uint32 init_page_count =
            comp_ctx->comp_data->memories[0].init_page_count;
        if (init_page_count == 0) {
            LLVMValueRef mem_size;

            if (!(mem_size = get_memory_curr_page_count(comp_ctx, func_ctx))) {
                goto fail;
            }
            BUILD_ICMP(LLVMIntEQ, mem_size,
                       MEMORY64_COND_VALUE(I64_ZERO, I32_ZERO), cmp, "is_zero");
            ADD_BASIC_BLOCK(check_succ, "check_mem_size_succ");
            LLVMMoveBasicBlockAfter(check_succ, block_curr);
            if (!aot_emit_exception(comp_ctx, func_ctx,
                                    EXCE_OUT_OF_BOUNDS_MEMORY_ACCESS, true, cmp,
                                    check_succ)) {
                goto fail;
            }

            SET_BUILD_POS(check_succ);
            block_curr = check_succ;
        }

        if (!(mem_check_bound =
                  get_memory_check_bound(comp_ctx, func_ctx, bytes))) {
            goto fail;
        }

        if (is_target_64bit) {
            BUILD_ICMP(LLVMIntUGT, offset1, mem_check_bound, cmp, "cmp");
        }
        else {
            if (comp_ctx->enable_shared_heap /* TODO: && mem_idx == 0 */) {
                /* Check integer overflow has been checked above */
                BUILD_ICMP(LLVMIntUGT, offset1, mem_check_bound, cmp, "cmp");
            }
            else {
                /* Check integer overflow */
                BUILD_ICMP(LLVMIntULT, offset1, addr, cmp1, "cmp1");
                BUILD_ICMP(LLVMIntUGT, offset1, mem_check_bound, cmp2, "cmp2");
                BUILD_OP(Or, cmp1, cmp2, cmp, "cmp");
            }
        }

        /* Add basic blocks */
        ADD_BASIC_BLOCK(check_succ, "check_succ");
        LLVMMoveBasicBlockAfter(check_succ, block_curr);

        if (!aot_emit_exception(comp_ctx, func_ctx,
                                EXCE_OUT_OF_BOUNDS_MEMORY_ACCESS, true, cmp,
                                check_succ)) {
            goto fail;
        }

        SET_BUILD_POS(check_succ);

        if (is_local_of_aot_value) {
            if (!aot_checked_addr_list_add(func_ctx, local_idx_of_aot_value,
                                           offset, bytes))
                goto fail;
        }
    }

    if (!enable_segue) {
        /* maddr = mem_base_addr + offset1 */
        if (!(maddr =
                  LLVMBuildInBoundsGEP2(comp_ctx->builder, INT8_TYPE,
                                        mem_base_addr, &offset1, 1, "maddr"))) {
            aot_set_last_error("llvm build add failed.");
            goto fail;
        }
    }
    else {
        LLVMValueRef maddr_base;

        if (!(maddr_base = LLVMBuildIntToPtr(comp_ctx->builder, addr,
                                             INT8_PTR_TYPE_GS, "maddr_base"))) {
            aot_set_last_error("llvm build int to ptr failed.");
            goto fail;
        }
        if (!(maddr = LLVMBuildInBoundsGEP2(comp_ctx->builder, INT8_TYPE,
                                            maddr_base, &offset_const, 1,
                                            "maddr"))) {
            aot_set_last_error("llvm build inboundgep failed.");
            goto fail;
        }
    }

    if (comp_ctx->enable_shared_heap /* TODO: && mem_idx == 0 */) {
        block_curr = LLVMGetInsertBlock(comp_ctx->builder);
        LLVMAddIncoming(maddr_phi, &maddr, &block_curr, 1);
        if (!LLVMBuildBr(comp_ctx->builder, block_maddr_phi)) {
            aot_set_last_error("llvm build br failed");
            goto fail;
        }
        LLVMPositionBuilderAtEnd(comp_ctx->builder, block_maddr_phi);
        return maddr_phi;
    }
    else
        return maddr;
fail:
    return NULL;
}

#define BUILD_PTR_CAST(ptr_type)                                           \
    do {                                                                   \
        if (!(maddr = LLVMBuildBitCast(comp_ctx->builder, maddr, ptr_type, \
                                       "data_ptr"))) {                     \
            aot_set_last_error("llvm build bit cast failed.");             \
            goto fail;                                                     \
        }                                                                  \
    } while (0)

#define BUILD_LOAD(data_type)                                             \
    do {                                                                  \
        if (!(value = LLVMBuildLoad2(comp_ctx->builder, data_type, maddr, \
                                     "data"))) {                          \
            aot_set_last_error("llvm build load failed.");                \
            goto fail;                                                    \
        }                                                                 \
        LLVMSetAlignment(value, known_align);                             \
    } while (0)

#define BUILD_TRUNC(value, data_type)                                     \
    do {                                                                  \
        if (!(value = LLVMBuildTrunc(comp_ctx->builder, value, data_type, \
                                     "val_trunc"))) {                     \
            aot_set_last_error("llvm build trunc failed.");               \
            goto fail;                                                    \
        }                                                                 \
    } while (0)

#define BUILD_STORE()                                                   \
    do {                                                                \
        LLVMValueRef res;                                               \
        if (!(res = LLVMBuildStore(comp_ctx->builder, value, maddr))) { \
            aot_set_last_error("llvm build store failed.");             \
            goto fail;                                                  \
        }                                                               \
        LLVMSetAlignment(res, known_align);                             \
    } while (0)

#define BUILD_SIGN_EXT(dst_type)                                        \
    do {                                                                \
        if (!(value = LLVMBuildSExt(comp_ctx->builder, value, dst_type, \
                                    "data_s_ext"))) {                   \
            aot_set_last_error("llvm build sign ext failed.");          \
            goto fail;                                                  \
        }                                                               \
    } while (0)

#define BUILD_ZERO_EXT(dst_type)                                        \
    do {                                                                \
        if (!(value = LLVMBuildZExt(comp_ctx->builder, value, dst_type, \
                                    "data_z_ext"))) {                   \
            aot_set_last_error("llvm build zero ext failed.");          \
            goto fail;                                                  \
        }                                                               \
    } while (0)

#if WASM_ENABLE_SHARED_MEMORY != 0 || WASM_ENABLE_STRINGREF != 0
bool
check_memory_alignment(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                       LLVMValueRef addr, uint32 align)
{
    LLVMBasicBlockRef block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    LLVMBasicBlockRef check_align_succ;
    LLVMValueRef align_mask = I32_CONST(((uint32)1 << align) - 1);
    LLVMValueRef res;

    CHECK_LLVM_CONST(align_mask);

    /* Convert pointer to int */
    if (!(addr = LLVMBuildPtrToInt(comp_ctx->builder, addr, I32_TYPE,
                                   "address"))) {
        aot_set_last_error("llvm build ptr to int failed.");
        goto fail;
    }

    /* The memory address should be aligned */
    BUILD_OP(And, addr, align_mask, res, "and");
    BUILD_ICMP(LLVMIntNE, res, I32_ZERO, res, "cmp");

    /* Add basic blocks */
    ADD_BASIC_BLOCK(check_align_succ, "check_align_succ");
    LLVMMoveBasicBlockAfter(check_align_succ, block_curr);

    if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_UNALIGNED_ATOMIC, true,
                            res, check_align_succ)) {
        goto fail;
    }

    SET_BUILD_POS(check_align_succ);

    return true;
fail:
    return false;
}
#endif /* WASM_ENABLE_SHARED_MEMORY != 0 || WASM_ENABLE_STRINGREF != 0 */

#if WASM_ENABLE_SHARED_MEMORY != 0
#define BUILD_ATOMIC_LOAD(align, data_type)                                \
    do {                                                                   \
        if (!(check_memory_alignment(comp_ctx, func_ctx, maddr, align))) { \
            goto fail;                                                     \
        }                                                                  \
        if (!(value = LLVMBuildLoad2(comp_ctx->builder, data_type, maddr,  \
                                     "data"))) {                           \
            aot_set_last_error("llvm build load failed.");                 \
            goto fail;                                                     \
        }                                                                  \
        LLVMSetAlignment(value, 1 << align);                               \
        LLVMSetVolatile(value, true);                                      \
        LLVMSetOrdering(value, LLVMAtomicOrderingSequentiallyConsistent);  \
    } while (0)

#define BUILD_ATOMIC_STORE(align)                                          \
    do {                                                                   \
        LLVMValueRef res;                                                  \
        if (!(check_memory_alignment(comp_ctx, func_ctx, maddr, align))) { \
            goto fail;                                                     \
        }                                                                  \
        if (!(res = LLVMBuildStore(comp_ctx->builder, value, maddr))) {    \
            aot_set_last_error("llvm build store failed.");                \
            goto fail;                                                     \
        }                                                                  \
        LLVMSetAlignment(res, 1 << align);                                 \
        LLVMSetVolatile(res, true);                                        \
        LLVMSetOrdering(res, LLVMAtomicOrderingSequentiallyConsistent);    \
    } while (0)
#endif

bool
aot_compile_op_i32_load(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                        uint32 align, mem_offset_t offset, uint32 bytes,
                        bool sign, bool atomic)
{
    LLVMValueRef maddr, value = NULL;
    LLVMTypeRef data_type;
    bool enable_segue = comp_ctx->enable_segue_i32_load;

    unsigned int known_align;
    if (!(maddr = aot_check_memory_overflow(comp_ctx, func_ctx, offset, bytes,
                                            enable_segue, &known_align)))
        return false;

    switch (bytes) {
        case 4:
            if (!enable_segue)
                BUILD_PTR_CAST(INT32_PTR_TYPE);
            else
                BUILD_PTR_CAST(INT32_PTR_TYPE_GS);
#if WASM_ENABLE_SHARED_MEMORY != 0
            if (atomic)
                BUILD_ATOMIC_LOAD(align, I32_TYPE);
            else
#endif
                BUILD_LOAD(I32_TYPE);
            break;
        case 2:
        case 1:
            if (bytes == 2) {
                if (!enable_segue)
                    BUILD_PTR_CAST(INT16_PTR_TYPE);
                else
                    BUILD_PTR_CAST(INT16_PTR_TYPE_GS);
                data_type = INT16_TYPE;
            }
            else {
                if (!enable_segue)
                    BUILD_PTR_CAST(INT8_PTR_TYPE);
                else
                    BUILD_PTR_CAST(INT8_PTR_TYPE_GS);
                data_type = INT8_TYPE;
            }

#if WASM_ENABLE_SHARED_MEMORY != 0
            if (atomic) {
                BUILD_ATOMIC_LOAD(align, data_type);
                BUILD_ZERO_EXT(I32_TYPE);
            }
            else
#endif
            {
                BUILD_LOAD(data_type);
                if (sign)
                    BUILD_SIGN_EXT(I32_TYPE);
                else
                    BUILD_ZERO_EXT(I32_TYPE);
            }
            break;
        default:
            bh_assert(0);
            break;
    }

    PUSH_I32(value);
    (void)data_type;
    return true;
fail:
    return false;
}

bool
aot_compile_op_i64_load(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                        uint32 align, mem_offset_t offset, uint32 bytes,
                        bool sign, bool atomic)
{
    LLVMValueRef maddr, value = NULL;
    LLVMTypeRef data_type;
    bool enable_segue = comp_ctx->enable_segue_i64_load;

    unsigned int known_align;
    if (!(maddr = aot_check_memory_overflow(comp_ctx, func_ctx, offset, bytes,
                                            enable_segue, &known_align)))
        return false;

    switch (bytes) {
        case 8:
            if (!enable_segue)
                BUILD_PTR_CAST(INT64_PTR_TYPE);
            else
                BUILD_PTR_CAST(INT64_PTR_TYPE_GS);
#if WASM_ENABLE_SHARED_MEMORY != 0
            if (atomic)
                BUILD_ATOMIC_LOAD(align, I64_TYPE);
            else
#endif
                BUILD_LOAD(I64_TYPE);
            break;
        case 4:
        case 2:
        case 1:
            if (bytes == 4) {
                if (!enable_segue)
                    BUILD_PTR_CAST(INT32_PTR_TYPE);
                else
                    BUILD_PTR_CAST(INT32_PTR_TYPE_GS);
                data_type = I32_TYPE;
            }
            else if (bytes == 2) {
                if (!enable_segue)
                    BUILD_PTR_CAST(INT16_PTR_TYPE);
                else
                    BUILD_PTR_CAST(INT16_PTR_TYPE_GS);
                data_type = INT16_TYPE;
            }
            else {
                if (!enable_segue)
                    BUILD_PTR_CAST(INT8_PTR_TYPE);
                else
                    BUILD_PTR_CAST(INT8_PTR_TYPE_GS);
                data_type = INT8_TYPE;
            }

#if WASM_ENABLE_SHARED_MEMORY != 0
            if (atomic) {
                BUILD_ATOMIC_LOAD(align, data_type);
                BUILD_ZERO_EXT(I64_TYPE);
            }
            else
#endif
            {
                BUILD_LOAD(data_type);
                if (sign)
                    BUILD_SIGN_EXT(I64_TYPE);
                else
                    BUILD_ZERO_EXT(I64_TYPE);
            }
            break;
        default:
            bh_assert(0);
            break;
    }

    PUSH_I64(value);
    (void)data_type;
    return true;
fail:
    return false;
}

bool
aot_compile_op_f32_load(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                        uint32 align, mem_offset_t offset)
{
    LLVMValueRef maddr, value;
    bool enable_segue = comp_ctx->enable_segue_f32_load;

    unsigned int known_align;
    if (!(maddr = aot_check_memory_overflow(comp_ctx, func_ctx, offset, 4,
                                            enable_segue, &known_align)))
        return false;

    if (!enable_segue)
        BUILD_PTR_CAST(F32_PTR_TYPE);
    else
        BUILD_PTR_CAST(F32_PTR_TYPE_GS);
    BUILD_LOAD(F32_TYPE);

    PUSH_F32(value);
    return true;
fail:
    return false;
}

bool
aot_compile_op_f64_load(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                        uint32 align, mem_offset_t offset)
{
    LLVMValueRef maddr, value;
    bool enable_segue = comp_ctx->enable_segue_f64_load;

    unsigned int known_align;
    if (!(maddr = aot_check_memory_overflow(comp_ctx, func_ctx, offset, 8,
                                            enable_segue, &known_align)))
        return false;

    if (!enable_segue)
        BUILD_PTR_CAST(F64_PTR_TYPE);
    else
        BUILD_PTR_CAST(F64_PTR_TYPE_GS);
    BUILD_LOAD(F64_TYPE);

    PUSH_F64(value);
    return true;
fail:
    return false;
}

bool
aot_compile_op_i32_store(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 align, mem_offset_t offset, uint32 bytes,
                         bool atomic)
{
    LLVMValueRef maddr, value;
    bool enable_segue = comp_ctx->enable_segue_i32_store;

    POP_I32(value);

    unsigned int known_align;
    if (!(maddr = aot_check_memory_overflow(comp_ctx, func_ctx, offset, bytes,
                                            enable_segue, &known_align)))
        return false;

    switch (bytes) {
        case 4:
            if (!enable_segue)
                BUILD_PTR_CAST(INT32_PTR_TYPE);
            else
                BUILD_PTR_CAST(INT32_PTR_TYPE_GS);
            break;
        case 2:
            if (!enable_segue)
                BUILD_PTR_CAST(INT16_PTR_TYPE);
            else
                BUILD_PTR_CAST(INT16_PTR_TYPE_GS);
            BUILD_TRUNC(value, INT16_TYPE);
            break;
        case 1:
            if (!enable_segue)
                BUILD_PTR_CAST(INT8_PTR_TYPE);
            else
                BUILD_PTR_CAST(INT8_PTR_TYPE_GS);
            BUILD_TRUNC(value, INT8_TYPE);
            break;
        default:
            bh_assert(0);
            break;
    }

#if WASM_ENABLE_SHARED_MEMORY != 0
    if (atomic)
        BUILD_ATOMIC_STORE(align);
    else
#endif
        BUILD_STORE();
    return true;
fail:
    return false;
}

bool
aot_compile_op_i64_store(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 align, mem_offset_t offset, uint32 bytes,
                         bool atomic)
{
    LLVMValueRef maddr, value;
    bool enable_segue = comp_ctx->enable_segue_i64_store;

    POP_I64(value);

    unsigned int known_align;
    if (!(maddr = aot_check_memory_overflow(comp_ctx, func_ctx, offset, bytes,
                                            enable_segue, &known_align)))
        return false;

    switch (bytes) {
        case 8:
            if (!enable_segue)
                BUILD_PTR_CAST(INT64_PTR_TYPE);
            else
                BUILD_PTR_CAST(INT64_PTR_TYPE_GS);
            break;
        case 4:
            if (!enable_segue)
                BUILD_PTR_CAST(INT32_PTR_TYPE);
            else
                BUILD_PTR_CAST(INT32_PTR_TYPE_GS);
            BUILD_TRUNC(value, I32_TYPE);
            break;
        case 2:
            if (!enable_segue)
                BUILD_PTR_CAST(INT16_PTR_TYPE);
            else
                BUILD_PTR_CAST(INT16_PTR_TYPE_GS);
            BUILD_TRUNC(value, INT16_TYPE);
            break;
        case 1:
            if (!enable_segue)
                BUILD_PTR_CAST(INT8_PTR_TYPE);
            else
                BUILD_PTR_CAST(INT8_PTR_TYPE_GS);
            BUILD_TRUNC(value, INT8_TYPE);
            break;
        default:
            bh_assert(0);
            break;
    }

#if WASM_ENABLE_SHARED_MEMORY != 0
    if (atomic)
        BUILD_ATOMIC_STORE(align);
    else
#endif
        BUILD_STORE();
    return true;
fail:
    return false;
}

bool
aot_compile_op_f32_store(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 align, mem_offset_t offset)
{
    LLVMValueRef maddr, value;
    bool enable_segue = comp_ctx->enable_segue_f32_store;

    POP_F32(value);

    unsigned int known_align;
    if (!(maddr = aot_check_memory_overflow(comp_ctx, func_ctx, offset, 4,
                                            enable_segue, &known_align)))
        return false;

    if (!enable_segue)
        BUILD_PTR_CAST(F32_PTR_TYPE);
    else
        BUILD_PTR_CAST(F32_PTR_TYPE_GS);
    BUILD_STORE();
    return true;
fail:
    return false;
}

bool
aot_compile_op_f64_store(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 align, mem_offset_t offset)
{
    LLVMValueRef maddr, value;
    bool enable_segue = comp_ctx->enable_segue_f64_store;

    POP_F64(value);

    unsigned int known_align;
    if (!(maddr = aot_check_memory_overflow(comp_ctx, func_ctx, offset, 8,
                                            enable_segue, &known_align)))
        return false;

    if (!enable_segue)
        BUILD_PTR_CAST(F64_PTR_TYPE);
    else
        BUILD_PTR_CAST(F64_PTR_TYPE_GS);
    BUILD_STORE();
    return true;
fail:
    return false;
}

static LLVMValueRef
get_memory_curr_page_count(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef mem_size;

    if (func_ctx->mem_space_unchanged) {
        mem_size = func_ctx->mem_info[0].mem_cur_page_count_addr;
    }
    else {
        if (!(mem_size = LLVMBuildLoad2(
                  comp_ctx->builder, I32_TYPE,
                  func_ctx->mem_info[0].mem_cur_page_count_addr, "mem_size"))) {
            aot_set_last_error("llvm build load failed.");
            goto fail;
        }
    }

    return LLVMBuildIntCast(comp_ctx->builder, mem_size,
                            MEMORY64_COND_VALUE(I64_TYPE, I32_TYPE), "");
fail:
    return NULL;
}

bool
aot_compile_op_memory_size(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef mem_size = get_memory_curr_page_count(comp_ctx, func_ctx);

    if (mem_size)
        PUSH_PAGE_COUNT(mem_size);
    return mem_size ? true : false;
fail:
    return false;
}

bool
aot_compile_op_memory_grow(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef mem_size = get_memory_curr_page_count(comp_ctx, func_ctx);
    LLVMValueRef delta, param_values[2], ret_value, func, value;
    LLVMTypeRef param_types[2], ret_type, func_type, func_ptr_type;
    int32 func_index;
#if WASM_ENABLE_MEMORY64 != 0
    LLVMValueRef u32_max, u32_cmp_result;
#endif

    if (!mem_size)
        return false;

    POP_PAGE_COUNT(delta);

    /* TODO: multi-memory aot_enlarge_memory_with_idx() */
    /* Function type of aot_enlarge_memory() */
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
        if (!(value = I64_CONST((uint64)(uintptr_t)wasm_enlarge_memory))
            || !(func = LLVMConstIntToPtr(value, func_ptr_type))) {
            aot_set_last_error("create LLVM value failed.");
            return false;
        }
    }
    else if (comp_ctx->is_indirect_mode) {
        if (!(func_ptr_type = LLVMPointerType(func_type, 0))) {
            aot_set_last_error("create LLVM function type failed.");
            return false;
        }
        func_index =
            aot_get_native_symbol_index(comp_ctx, "aot_enlarge_memory");
        if (func_index < 0) {
            return false;
        }
        if (!(func = aot_get_func_from_table(comp_ctx, func_ctx->native_symbol,
                                             func_ptr_type, func_index))) {
            return false;
        }
    }
    else {
        char *func_name = "aot_enlarge_memory";
        /* AOT mode, delcare the function */
        if (!(func = LLVMGetNamedFunction(func_ctx->module, func_name))
            && !(func =
                     LLVMAddFunction(func_ctx->module, func_name, func_type))) {
            aot_set_last_error("llvm add function failed.");
            return false;
        }
    }

    /* Call function aot_enlarge_memory() */
    param_values[0] = func_ctx->aot_inst;
    param_values[1] = LLVMBuildTrunc(comp_ctx->builder, delta, I32_TYPE, "");
    if (!(ret_value = LLVMBuildCall2(comp_ctx->builder, func_type, func,
                                     param_values, 2, "call"))) {
        aot_set_last_error("llvm build call failed.");
        return false;
    }

    BUILD_ICMP(LLVMIntUGT, ret_value, I8_ZERO, ret_value, "mem_grow_ret");
#if WASM_ENABLE_MEMORY64 != 0
    if (IS_MEMORY64) {
        if (!(u32_max = I64_CONST(UINT32_MAX))) {
            aot_set_last_error("llvm build const failed");
            return false;
        }
        BUILD_ICMP(LLVMIntULE, delta, u32_max, u32_cmp_result, "page_size_cmp");
        BUILD_OP(And, ret_value, u32_cmp_result, ret_value, "and");
    }
#endif

    /* ret_value = ret_value == true ? pre_page_count : -1 */
    if (!(ret_value = LLVMBuildSelect(
              comp_ctx->builder, ret_value, mem_size,
              MEMORY64_COND_VALUE(I64_NEG_ONE, I32_NEG_ONE), "mem_grow_ret"))) {
        aot_set_last_error("llvm build select failed.");
        return false;
    }

    PUSH_PAGE_COUNT(ret_value);
    return true;
fail:
    return false;
}

#if WASM_ENABLE_BULK_MEMORY != 0 || WASM_ENABLE_STRINGREF != 0
LLVMValueRef
check_bulk_memory_overflow(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                           LLVMValueRef offset, LLVMValueRef bytes)
{
    LLVMValueRef maddr, max_addr, cmp;
    LLVMValueRef mem_base_addr, maddr_phi = NULL;
    LLVMBasicBlockRef block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    LLVMBasicBlockRef check_succ, block_maddr_phi = NULL;
    LLVMValueRef mem_size;
#if WASM_ENABLE_MEMORY64 == 0
    bool is_memory64 = false;
#else
    bool is_memory64 = IS_MEMORY64;
#endif

    /* Get memory base address and memory data size */
#if WASM_ENABLE_SHARED_MEMORY != 0
    bool is_shared_memory = comp_ctx->comp_data->memories[0].flags & 0x02;

    if (func_ctx->mem_space_unchanged || is_shared_memory) {
#else
    if (func_ctx->mem_space_unchanged) {
#endif
        mem_base_addr = func_ctx->mem_info[0].mem_base_addr;
    }
    else {
        if (!(mem_base_addr = LLVMBuildLoad2(
                  comp_ctx->builder, OPQ_PTR_TYPE,
                  func_ctx->mem_info[0].mem_base_addr, "mem_base"))) {
            aot_set_last_error("llvm build load failed.");
            goto fail;
        }
    }

    /*
     * Note: not throw the integer-overflow-exception here since it must
     * have been thrown when converting float to integer before
     */
    /* return addres directly if constant offset and inside memory space */
    if (LLVMIsEfficientConstInt(offset) && LLVMIsEfficientConstInt(bytes)) {
        uint64 mem_offset = (uint64)LLVMConstIntGetZExtValue(offset);
        uint64 mem_len = (uint64)LLVMConstIntGetZExtValue(bytes);
        uint32 num_bytes_per_page =
            comp_ctx->comp_data->memories[0].num_bytes_per_page;
        uint32 init_page_count =
            comp_ctx->comp_data->memories[0].init_page_count;
        uint64 mem_data_size = (uint64)num_bytes_per_page * init_page_count;
        if (mem_data_size > 0 && mem_offset + mem_len <= mem_data_size) {
            /* inside memory space */
            /* maddr = mem_base_addr + moffset */
            if (!(maddr = LLVMBuildInBoundsGEP2(comp_ctx->builder, INT8_TYPE,
                                                mem_base_addr, &offset, 1,
                                                "maddr"))) {
                aot_set_last_error("llvm build add failed.");
                goto fail;
            }
            return maddr;
        }
    }

    if (func_ctx->mem_space_unchanged) {
        mem_size = func_ctx->mem_info[0].mem_data_size_addr;
    }
    else {
        if (!(mem_size = LLVMBuildLoad2(
                  comp_ctx->builder, I64_TYPE,
                  func_ctx->mem_info[0].mem_data_size_addr, "mem_size"))) {
            aot_set_last_error("llvm build load failed.");
            goto fail;
        }
    }

    ADD_BASIC_BLOCK(check_succ, "check_succ");
    LLVMMoveBasicBlockAfter(check_succ, block_curr);

    offset =
        LLVMBuildZExt(comp_ctx->builder, offset, I64_TYPE, "extend_offset");
    bytes = LLVMBuildZExt(comp_ctx->builder, bytes, I64_TYPE, "extend_len");
    if (!offset || !bytes) {
        aot_set_last_error("llvm build zext failed.");
        goto fail;
    }

    /* TODO: check whether integer overflow occurs when memory is 64-bit
             and boundary check is enabled */
    BUILD_OP(Add, offset, bytes, max_addr, "max_addr");

    if (comp_ctx->enable_shared_heap /* TODO: && mem_idx == 0 */) {
        LLVMBasicBlockRef app_addr_in_shared_heap, app_addr_in_linear_mem;
        LLVMValueRef shared_heap_start_off, shared_heap_check_bound;
        LLVMValueRef max_offset, cmp1, cmp2, is_in_shared_heap;

        /* Add basic blocks */
        ADD_BASIC_BLOCK(app_addr_in_shared_heap, "app_addr_in_shared_heap");
        ADD_BASIC_BLOCK(app_addr_in_linear_mem, "app_addr_in_linear_mem");
        ADD_BASIC_BLOCK(block_maddr_phi, "maddr_phi");

        LLVMMoveBasicBlockAfter(app_addr_in_shared_heap, block_curr);
        LLVMMoveBasicBlockAfter(app_addr_in_linear_mem,
                                app_addr_in_shared_heap);
        LLVMMoveBasicBlockAfter(block_maddr_phi, check_succ);

        LLVMPositionBuilderAtEnd(comp_ctx->builder, block_maddr_phi);
        if (!(maddr_phi = LLVMBuildPhi(comp_ctx->builder, INT8_PTR_TYPE,
                                       "maddr_phi"))) {
            aot_set_last_error("llvm build phi failed");
            goto fail;
        }

        LLVMPositionBuilderAtEnd(comp_ctx->builder, block_curr);

        shared_heap_start_off = func_ctx->shared_heap_start_off;
        if (comp_ctx->pointer_size == sizeof(uint32)) {
            if (!(shared_heap_start_off =
                      LLVMBuildZExt(comp_ctx->builder, shared_heap_start_off,
                                    I64_TYPE, "shared_heap_start_off_u64"))) {
                aot_set_last_error("llvm build zext failed");
                goto fail;
            }
        }
        shared_heap_check_bound =
            is_memory64 ? I64_CONST(UINT64_MAX) : I64_CONST(UINT32_MAX);
        CHECK_LLVM_CONST(shared_heap_check_bound);

        /* Check whether the bytes to access are in shared heap */
        if (!comp_ctx->enable_bound_check) {
            /* Use IntUGT but not IntUGE to compare, same as the check
               in aot_check_memory_overflow */
            BUILD_ICMP(LLVMIntUGT, offset, func_ctx->shared_heap_start_off,
                       is_in_shared_heap, "is_in_shared_heap");
        }
        else {
            BUILD_ICMP(LLVMIntUGT, offset, func_ctx->shared_heap_start_off,
                       cmp1, "cmp1");
            BUILD_OP(Add, max_addr, I64_NEG_ONE, max_offset, "max_offset");
            BUILD_ICMP(LLVMIntULE, max_offset, shared_heap_check_bound, cmp2,
                       "cmp2");
            BUILD_OP(And, cmp1, cmp2, is_in_shared_heap, "is_in_shared_heap");
        }

        if (!LLVMBuildCondBr(comp_ctx->builder, is_in_shared_heap,
                             app_addr_in_shared_heap, app_addr_in_linear_mem)) {
            aot_set_last_error("llvm build cond br failed");
            goto fail;
        }

        LLVMPositionBuilderAtEnd(comp_ctx->builder, app_addr_in_shared_heap);

        /* Get native address inside shared heap */
        if (!(maddr = LLVMBuildInBoundsGEP2(comp_ctx->builder, INT8_TYPE,
                                            func_ctx->shared_heap_base_addr_adj,
                                            &offset, 1, "maddr_shared_heap"))) {
            aot_set_last_error("llvm build inbounds gep failed");
            goto fail;
        }
        LLVMAddIncoming(maddr_phi, &maddr, &app_addr_in_shared_heap, 1);

        if (!LLVMBuildBr(comp_ctx->builder, block_maddr_phi)) {
            aot_set_last_error("llvm build br failed");
            goto fail;
        }

        LLVMPositionBuilderAtEnd(comp_ctx->builder, app_addr_in_linear_mem);
        block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    }

    BUILD_ICMP(LLVMIntUGT, max_addr, mem_size, cmp, "cmp_max_mem_addr");

    if (!aot_emit_exception(comp_ctx, func_ctx,
                            EXCE_OUT_OF_BOUNDS_MEMORY_ACCESS, true, cmp,
                            check_succ)) {
        goto fail;
    }

    /* maddr = mem_base_addr + offset */
    if (!(maddr = LLVMBuildInBoundsGEP2(comp_ctx->builder, INT8_TYPE,
                                        mem_base_addr, &offset, 1, "maddr"))) {
        aot_set_last_error("llvm build add failed.");
        goto fail;
    }

    if (comp_ctx->enable_shared_heap /* TODO: && mem_idx == 0 */) {
        block_curr = LLVMGetInsertBlock(comp_ctx->builder);
        LLVMAddIncoming(maddr_phi, &maddr, &block_curr, 1);
        if (!LLVMBuildBr(comp_ctx->builder, block_maddr_phi)) {
            aot_set_last_error("llvm build br failed");
            goto fail;
        }
        LLVMPositionBuilderAtEnd(comp_ctx->builder, block_maddr_phi);
        return maddr_phi;
    }
    else
        return maddr;
fail:
    return NULL;
}
#endif /* end of WASM_ENABLE_BULK_MEMORY != 0 || WASM_ENABLE_STRINGREF != 0 */

#if WASM_ENABLE_BULK_MEMORY != 0
bool
aot_compile_op_memory_init(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                           uint32 seg_index)
{
    LLVMValueRef seg, offset, dst, len, param_values[5], ret_value, func, value;
    LLVMTypeRef param_types[5], ret_type, func_type, func_ptr_type;
    AOTFuncType *aot_func_type = func_ctx->aot_func->func_type;
    LLVMBasicBlockRef block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    LLVMBasicBlockRef mem_init_fail, init_success;

    seg = I32_CONST(seg_index);

    POP_I32(len);
    POP_I32(offset);
    POP_MEM_OFFSET(dst);

    if (!zero_extend_u64(comp_ctx, &dst, "dst64")) {
        return false;
    }

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    param_types[2] = I32_TYPE;
    param_types[3] = I32_TYPE;
    param_types[4] = SIZE_T_TYPE;
    ret_type = INT8_TYPE;

    if (comp_ctx->is_jit_mode)
        GET_AOT_FUNCTION(llvm_jit_memory_init, 5);
    else
        GET_AOT_FUNCTION(aot_memory_init, 5);

    /* Call function aot_memory_init() */
    param_values[0] = func_ctx->aot_inst;
    param_values[1] = seg;
    param_values[2] = offset;
    param_values[3] = len;
    param_values[4] = dst;
    if (!(ret_value = LLVMBuildCall2(comp_ctx->builder, func_type, func,
                                     param_values, 5, "call"))) {
        aot_set_last_error("llvm build call failed.");
        return false;
    }

    BUILD_ICMP(LLVMIntUGT, ret_value, I8_ZERO, ret_value, "mem_init_ret");

    ADD_BASIC_BLOCK(mem_init_fail, "mem_init_fail");
    ADD_BASIC_BLOCK(init_success, "init_success");

    LLVMMoveBasicBlockAfter(mem_init_fail, block_curr);
    LLVMMoveBasicBlockAfter(init_success, block_curr);

    if (!LLVMBuildCondBr(comp_ctx->builder, ret_value, init_success,
                         mem_init_fail)) {
        aot_set_last_error("llvm build cond br failed.");
        goto fail;
    }

    /* If memory.init failed, return this function
       so the runtime can catch the exception */
    LLVMPositionBuilderAtEnd(comp_ctx->builder, mem_init_fail);
    if (!aot_build_zero_function_ret(comp_ctx, func_ctx, aot_func_type)) {
        goto fail;
    }

    LLVMPositionBuilderAtEnd(comp_ctx->builder, init_success);

    return true;
fail:
    return false;
}

bool
aot_compile_op_data_drop(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 seg_index)
{
    LLVMValueRef seg, param_values[2], ret_value, func, value;
    LLVMTypeRef param_types[2], ret_type, func_type, func_ptr_type;

    seg = I32_CONST(seg_index);
    CHECK_LLVM_CONST(seg);

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    ret_type = INT8_TYPE;

    if (comp_ctx->is_jit_mode)
        GET_AOT_FUNCTION(llvm_jit_data_drop, 2);
    else
        GET_AOT_FUNCTION(aot_data_drop, 2);

    /* Call function aot_data_drop() */
    param_values[0] = func_ctx->aot_inst;
    param_values[1] = seg;
    if (!(ret_value = LLVMBuildCall2(comp_ctx->builder, func_type, func,
                                     param_values, 2, "call"))) {
        aot_set_last_error("llvm build call failed.");
        return false;
    }

    return true;
fail:
    return false;
}

bool
aot_compile_op_memory_copy(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef src, dst, src_addr, dst_addr, len, res;
    bool call_aot_memmove = false;

    POP_MEM_OFFSET(len);
    POP_MEM_OFFSET(src);
    POP_MEM_OFFSET(dst);

    if (!(src_addr = check_bulk_memory_overflow(comp_ctx, func_ctx, src, len)))
        return false;

    if (!(dst_addr = check_bulk_memory_overflow(comp_ctx, func_ctx, dst, len)))
        return false;

    if (!zero_extend_u64(comp_ctx, &len, "len64")) {
        return false;
    }

    call_aot_memmove = comp_ctx->is_indirect_mode || comp_ctx->is_jit_mode;
    if (call_aot_memmove) {
        LLVMTypeRef param_types[3], ret_type, func_type, func_ptr_type;
        LLVMValueRef func, params[3];

        param_types[0] = INT8_PTR_TYPE;
        param_types[1] = INT8_PTR_TYPE;
        param_types[2] = SIZE_T_TYPE;
        ret_type = INT8_PTR_TYPE;

        if (!(func_type = LLVMFunctionType(ret_type, param_types, 3, false))) {
            aot_set_last_error("create LLVM function type failed.");
            return false;
        }

        if (!(func_ptr_type = LLVMPointerType(func_type, 0))) {
            aot_set_last_error("create LLVM function pointer type failed.");
            return false;
        }

        if (comp_ctx->is_jit_mode) {
            if (!(func = I64_CONST((uint64)(uintptr_t)aot_memmove))
                || !(func = LLVMConstIntToPtr(func, func_ptr_type))) {
                aot_set_last_error("create LLVM value failed.");
                return false;
            }
        }
        else {
            int32 func_index;
            func_index = aot_get_native_symbol_index(comp_ctx, "memmove");
            if (func_index < 0) {
                return false;
            }
            if (!(func =
                      aot_get_func_from_table(comp_ctx, func_ctx->native_symbol,
                                              func_ptr_type, func_index))) {
                return false;
            }
        }

        params[0] = dst_addr;
        params[1] = src_addr;
        params[2] = len;
        if (!(res = LLVMBuildCall2(comp_ctx->builder, func_type, func, params,
                                   3, "call_memmove"))) {
            aot_set_last_error("llvm build memmove failed.");
            return false;
        }
    }
    else {
        if (!(res = LLVMBuildMemMove(comp_ctx->builder, dst_addr, 1, src_addr,
                                     1, len))) {
            aot_set_last_error("llvm build memmove failed.");
            return false;
        }
    }

    return true;
fail:
    return false;
}

static void *
jit_memset(void *s, int c, size_t n)
{
    return memset(s, c, n);
}

bool
aot_compile_op_memory_fill(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef val, dst, dst_addr, len, res;
    LLVMTypeRef param_types[3], ret_type, func_type, func_ptr_type;
    LLVMValueRef func, params[3];

    POP_MEM_OFFSET(len);
    POP_I32(val);
    POP_MEM_OFFSET(dst);

    if (!(dst_addr = check_bulk_memory_overflow(comp_ctx, func_ctx, dst, len)))
        return false;

    if (!zero_extend_u64(comp_ctx, &len, "len64")) {
        return false;
    }

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = I32_TYPE;
    param_types[2] = SIZE_T_TYPE;
    ret_type = INT8_PTR_TYPE;

    if (!(func_type = LLVMFunctionType(ret_type, param_types, 3, false))) {
        aot_set_last_error("create LLVM function type failed.");
        return false;
    }

    if (!(func_ptr_type = LLVMPointerType(func_type, 0))) {
        aot_set_last_error("create LLVM function pointer type failed.");
        return false;
    }

    if (comp_ctx->is_jit_mode) {
        if (!(func = I64_CONST((uint64)(uintptr_t)jit_memset))
            || !(func = LLVMConstIntToPtr(func, func_ptr_type))) {
            aot_set_last_error("create LLVM value failed.");
            return false;
        }
    }
    else if (comp_ctx->is_indirect_mode) {
        int32 func_index;
        func_index = aot_get_native_symbol_index(comp_ctx, "memset");
        if (func_index < 0) {
            return false;
        }
        if (!(func = aot_get_func_from_table(comp_ctx, func_ctx->native_symbol,
                                             func_ptr_type, func_index))) {
            return false;
        }
    }
    else {
        if (!(func = LLVMGetNamedFunction(func_ctx->module, "memset"))
            && !(func =
                     LLVMAddFunction(func_ctx->module, "memset", func_type))) {
            aot_set_last_error("llvm add function failed.");
            return false;
        }
    }

    params[0] = dst_addr;
    params[1] = val;
    params[2] = len;
    if (!(res = LLVMBuildCall2(comp_ctx->builder, func_type, func, params, 3,
                               "call_memset"))) {
        aot_set_last_error("llvm build memset failed.");
        return false;
    }

    return true;
fail:
    return false;
}
#endif /* end of WASM_ENABLE_BULK_MEMORY */

#if WASM_ENABLE_SHARED_MEMORY != 0
bool
aot_compile_op_atomic_rmw(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          uint8 atomic_op, uint8 op_type, uint32 align,
                          mem_offset_t offset, uint32 bytes)
{
    LLVMValueRef maddr, value, result;
    bool enable_segue = (op_type == VALUE_TYPE_I32)
                            ? comp_ctx->enable_segue_i32_load
                                  && comp_ctx->enable_segue_i32_store
                            : comp_ctx->enable_segue_i64_load
                                  && comp_ctx->enable_segue_i64_store;

    if (op_type == VALUE_TYPE_I32)
        POP_I32(value);
    else
        POP_I64(value);

    if (!(maddr = aot_check_memory_overflow(comp_ctx, func_ctx, offset, bytes,
                                            enable_segue, NULL)))
        return false;

    if (!check_memory_alignment(comp_ctx, func_ctx, maddr, align))
        return false;

    switch (bytes) {
        case 8:
            if (!enable_segue)
                BUILD_PTR_CAST(INT64_PTR_TYPE);
            else
                BUILD_PTR_CAST(INT64_PTR_TYPE_GS);
            break;
        case 4:
            if (!enable_segue)
                BUILD_PTR_CAST(INT32_PTR_TYPE);
            else
                BUILD_PTR_CAST(INT32_PTR_TYPE_GS);
            if (op_type == VALUE_TYPE_I64)
                BUILD_TRUNC(value, I32_TYPE);
            break;
        case 2:
            if (!enable_segue)
                BUILD_PTR_CAST(INT16_PTR_TYPE);
            else
                BUILD_PTR_CAST(INT16_PTR_TYPE_GS);
            BUILD_TRUNC(value, INT16_TYPE);
            break;
        case 1:
            if (!enable_segue)
                BUILD_PTR_CAST(INT8_PTR_TYPE);
            else
                BUILD_PTR_CAST(INT8_PTR_TYPE_GS);
            BUILD_TRUNC(value, INT8_TYPE);
            break;
        default:
            bh_assert(0);
            break;
    }

    if (!(result = LLVMBuildAtomicRMW(
              comp_ctx->builder, atomic_op, maddr, value,
              LLVMAtomicOrderingSequentiallyConsistent, false))) {
        goto fail;
    }

    LLVMSetVolatile(result, true);

    if (op_type == VALUE_TYPE_I32) {
        if (!(result = LLVMBuildZExt(comp_ctx->builder, result, I32_TYPE,
                                     "result_i32"))) {
            goto fail;
        }
        PUSH_I32(result);
    }
    else {
        if (!(result = LLVMBuildZExt(comp_ctx->builder, result, I64_TYPE,
                                     "result_i64"))) {
            goto fail;
        }
        PUSH_I64(result);
    }

    return true;
fail:
    return false;
}

bool
aot_compile_op_atomic_cmpxchg(AOTCompContext *comp_ctx,
                              AOTFuncContext *func_ctx, uint8 op_type,
                              uint32 align, mem_offset_t offset, uint32 bytes)
{
    LLVMValueRef maddr, value, expect, result;
    bool enable_segue = (op_type == VALUE_TYPE_I32)
                            ? comp_ctx->enable_segue_i32_load
                                  && comp_ctx->enable_segue_i32_store
                            : comp_ctx->enable_segue_i64_load
                                  && comp_ctx->enable_segue_i64_store;

    if (op_type == VALUE_TYPE_I32) {
        POP_I32(value);
        POP_I32(expect);
    }
    else {
        POP_I64(value);
        POP_I64(expect);
    }

    if (!(maddr = aot_check_memory_overflow(comp_ctx, func_ctx, offset, bytes,
                                            enable_segue, NULL)))
        return false;

    if (!check_memory_alignment(comp_ctx, func_ctx, maddr, align))
        return false;

    switch (bytes) {
        case 8:
            if (!enable_segue)
                BUILD_PTR_CAST(INT64_PTR_TYPE);
            else
                BUILD_PTR_CAST(INT64_PTR_TYPE_GS);
            break;
        case 4:
            if (!enable_segue)
                BUILD_PTR_CAST(INT32_PTR_TYPE);
            else
                BUILD_PTR_CAST(INT32_PTR_TYPE_GS);
            if (op_type == VALUE_TYPE_I64) {
                BUILD_TRUNC(value, I32_TYPE);
                BUILD_TRUNC(expect, I32_TYPE);
            }
            break;
        case 2:
            if (!enable_segue)
                BUILD_PTR_CAST(INT16_PTR_TYPE);
            else
                BUILD_PTR_CAST(INT16_PTR_TYPE_GS);
            BUILD_TRUNC(value, INT16_TYPE);
            BUILD_TRUNC(expect, INT16_TYPE);
            break;
        case 1:
            if (!enable_segue)
                BUILD_PTR_CAST(INT8_PTR_TYPE);
            else
                BUILD_PTR_CAST(INT8_PTR_TYPE_GS);
            BUILD_TRUNC(value, INT8_TYPE);
            BUILD_TRUNC(expect, INT8_TYPE);
            break;
        default:
            bh_assert(0);
            break;
    }

    if (!(result = LLVMBuildAtomicCmpXchg(
              comp_ctx->builder, maddr, expect, value,
              LLVMAtomicOrderingSequentiallyConsistent,
              LLVMAtomicOrderingSequentiallyConsistent, false))) {
        goto fail;
    }

    LLVMSetVolatile(result, true);

    /* CmpXchg return {i32, i1} structure,
       we need to extract the previous_value from the structure */
    if (!(result = LLVMBuildExtractValue(comp_ctx->builder, result, 0,
                                         "previous_value"))) {
        goto fail;
    }

    if (op_type == VALUE_TYPE_I32) {
        if (LLVMTypeOf(result) != I32_TYPE) {
            if (!(result = LLVMBuildZExt(comp_ctx->builder, result, I32_TYPE,
                                         "result_i32"))) {
                goto fail;
            }
        }
        PUSH_I32(result);
    }
    else {
        if (LLVMTypeOf(result) != I64_TYPE) {
            if (!(result = LLVMBuildZExt(comp_ctx->builder, result, I64_TYPE,
                                         "result_i64"))) {
                goto fail;
            }
        }
        PUSH_I64(result);
    }

    return true;
fail:
    return false;
}

bool
aot_compile_op_atomic_wait(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                           uint8 op_type, uint32 align, mem_offset_t offset,
                           uint32 bytes)
{
    LLVMValueRef maddr, value, timeout, expect, cmp;
    LLVMValueRef param_values[5], ret_value, func, is_wait64;
    LLVMTypeRef param_types[5], ret_type, func_type, func_ptr_type;
    LLVMBasicBlockRef wait_fail, wait_success;
    LLVMBasicBlockRef block_curr = LLVMGetInsertBlock(comp_ctx->builder);
    AOTFuncType *aot_func_type = func_ctx->aot_func->func_type;

    POP_I64(timeout);
    if (op_type == VALUE_TYPE_I32) {
        POP_I32(expect);
        is_wait64 = I8_CONST(false);
        if (!(expect = LLVMBuildZExt(comp_ctx->builder, expect, I64_TYPE,
                                     "expect_i64"))) {
            goto fail;
        }
    }
    else {
        POP_I64(expect);
        is_wait64 = I8_CONST(true);
    }

    CHECK_LLVM_CONST(is_wait64);

    if (!(maddr = aot_check_memory_overflow(comp_ctx, func_ctx, offset, bytes,
                                            false, NULL)))
        return false;

    if (!check_memory_alignment(comp_ctx, func_ctx, maddr, align))
        return false;

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = INT8_PTR_TYPE;
    param_types[2] = I64_TYPE;
    param_types[3] = I64_TYPE;
    param_types[4] = INT8_TYPE;
    ret_type = I32_TYPE;

    GET_AOT_FUNCTION(wasm_runtime_atomic_wait, 5);

    /* Call function wasm_runtime_atomic_wait() */
    param_values[0] = func_ctx->aot_inst;
    param_values[1] = maddr;
    param_values[2] = expect;
    param_values[3] = timeout;
    param_values[4] = is_wait64;
    if (!(ret_value = LLVMBuildCall2(comp_ctx->builder, func_type, func,
                                     param_values, 5, "call"))) {
        aot_set_last_error("llvm build call failed.");
        return false;
    }

    BUILD_ICMP(LLVMIntNE, ret_value, I32_NEG_ONE, cmp, "atomic_wait_ret");

    ADD_BASIC_BLOCK(wait_fail, "atomic_wait_fail");
    ADD_BASIC_BLOCK(wait_success, "wait_success");

    LLVMMoveBasicBlockAfter(wait_fail, block_curr);
    LLVMMoveBasicBlockAfter(wait_success, block_curr);

    if (!LLVMBuildCondBr(comp_ctx->builder, cmp, wait_success, wait_fail)) {
        aot_set_last_error("llvm build cond br failed.");
        goto fail;
    }

    /* If atomic wait failed, return this function
       so the runtime can catch the exception */
    LLVMPositionBuilderAtEnd(comp_ctx->builder, wait_fail);
    if (!aot_build_zero_function_ret(comp_ctx, func_ctx, aot_func_type)) {
        goto fail;
    }

    LLVMPositionBuilderAtEnd(comp_ctx->builder, wait_success);

    PUSH_I32(ret_value);

    /* Insert suspend check point */
    if (comp_ctx->enable_thread_mgr) {
        if (!check_suspend_flags(comp_ctx, func_ctx, false))
            return false;
    }

    return true;
fail:
    return false;
}

bool
aot_compiler_op_atomic_notify(AOTCompContext *comp_ctx,
                              AOTFuncContext *func_ctx, uint32 align,
                              mem_offset_t offset, uint32 bytes)
{
    LLVMValueRef maddr, value, count;
    LLVMValueRef param_values[3], ret_value, func;
    LLVMTypeRef param_types[3], ret_type, func_type, func_ptr_type;

    POP_I32(count);

    if (!(maddr = aot_check_memory_overflow(comp_ctx, func_ctx, offset, bytes,
                                            false, NULL)))
        return false;

    if (!check_memory_alignment(comp_ctx, func_ctx, maddr, align))
        return false;

    param_types[0] = INT8_PTR_TYPE;
    param_types[1] = INT8_PTR_TYPE;
    param_types[2] = I32_TYPE;
    ret_type = I32_TYPE;

    GET_AOT_FUNCTION(wasm_runtime_atomic_notify, 3);

    /* Call function wasm_runtime_atomic_notify() */
    param_values[0] = func_ctx->aot_inst;
    param_values[1] = maddr;
    param_values[2] = count;
    if (!(ret_value = LLVMBuildCall2(comp_ctx->builder, func_type, func,
                                     param_values, 3, "call"))) {
        aot_set_last_error("llvm build call failed.");
        return false;
    }

    PUSH_I32(ret_value);

    return true;
fail:
    return false;
}

bool
aot_compiler_op_atomic_fence(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    return LLVMBuildFence(comp_ctx->builder,
                          LLVMAtomicOrderingSequentiallyConsistent, false, "")
               ? true
               : false;
}

#endif /* end of WASM_ENABLE_SHARED_MEMORY */
