/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_emit_variable.h"
#include "aot_emit_exception.h"
#include "../aot/aot_runtime.h"
#include "aot_emit_table.h"

#define CHECK_LOCAL(idx)                                      \
    do {                                                      \
        if (idx >= func_ctx->aot_func->func_type->param_count \
                       + func_ctx->aot_func->local_count) {   \
            aot_set_last_error("local index out of range");   \
            return false;                                     \
        }                                                     \
    } while (0)

static uint8
get_local_type(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
               uint32 local_idx)
{
    AOTFunc *aot_func = func_ctx->aot_func;
    uint32 param_count = aot_func->func_type->param_count;
    uint8 local_type;

    local_type = local_idx < param_count
                     ? aot_func->func_type->types[local_idx]
                     : aot_func->local_types_wp[local_idx - param_count];

    if (comp_ctx->enable_gc && aot_is_type_gc_reftype(local_type))
        local_type = VALUE_TYPE_GC_REF;

    return local_type;
}

bool
aot_compile_op_get_local(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 local_idx)
{
    char name[32];
    LLVMValueRef value;
    AOTValue *aot_value_top;
    uint8 local_type;

    CHECK_LOCAL(local_idx);

    local_type = get_local_type(comp_ctx, func_ctx, local_idx);

    snprintf(name, sizeof(name), "%s%d%s", "local", local_idx, "#");
    if (!(value = LLVMBuildLoad2(comp_ctx->builder, TO_LLVM_TYPE(local_type),
                                 func_ctx->locals[local_idx], name))) {
        aot_set_last_error("llvm build load fail");
        return false;
    }

    PUSH(value, local_type);

    aot_value_top =
        func_ctx->block_stack.block_list_end->value_stack.value_list_end;
    aot_value_top->is_local = true;
    aot_value_top->local_idx = local_idx;
    return true;

fail:
    return false;
}

static bool
aot_compile_op_set_or_tee_local(AOTCompContext *comp_ctx,
                                AOTFuncContext *func_ctx, uint32 local_idx,
                                bool is_tee_local)
{
    LLVMValueRef value;
    uint8 local_type;
    uint32 n;

    CHECK_LOCAL(local_idx);

    local_type = get_local_type(comp_ctx, func_ctx, local_idx);

    POP(value, local_type);

    if (comp_ctx->aot_frame) {
        /* Get the slot index */
        n = func_ctx->aot_func->local_offsets[local_idx];
        bh_assert(comp_ctx->aot_frame->lp[n].type == local_type);

        switch (local_type) {
            case VALUE_TYPE_I32:
                set_local_i32(comp_ctx->aot_frame, n, value);
                break;
            case VALUE_TYPE_I64:
                set_local_i64(comp_ctx->aot_frame, n, value);
                break;
            case VALUE_TYPE_F32:
                set_local_f32(comp_ctx->aot_frame, n, value);
                break;
            case VALUE_TYPE_F64:
                set_local_f64(comp_ctx->aot_frame, n, value);
                break;
            case VALUE_TYPE_V128:
                set_local_v128(comp_ctx->aot_frame, n, value);
                break;
            case VALUE_TYPE_FUNCREF:
            case VALUE_TYPE_EXTERNREF:
                set_local_ref(comp_ctx->aot_frame, n, value, local_type);
                break;
#if WASM_ENABLE_GC != 0
            case VALUE_TYPE_GC_REF:
                set_local_gc_ref(comp_ctx->aot_frame, n, value, local_type);
                break;
#endif
            default:
                bh_assert(0);
                break;
        }
    }

    if (!LLVMBuildStore(comp_ctx->builder, value,
                        func_ctx->locals[local_idx])) {
        aot_set_last_error("llvm build store fail");
        return false;
    }

    if (is_tee_local) {
        PUSH(value, local_type);
    }

    aot_checked_addr_list_del(func_ctx, local_idx);
    return true;

fail:
    return false;
}

bool
aot_compile_op_set_local(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 local_idx)
{
    return aot_compile_op_set_or_tee_local(comp_ctx, func_ctx, local_idx,
                                           false);
}

bool
aot_compile_op_tee_local(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 local_idx)
{
    return aot_compile_op_set_or_tee_local(comp_ctx, func_ctx, local_idx, true);
}

/*TODO: should be optimized by moving globals to WASMModuleInstance */
LLVMValueRef
get_global_from_wasm_inst(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          uint32 global_idx)
{
    /* WASMModuleInstance->e->globals */
    uint32 e_offset_val = get_module_inst_extra_offset(comp_ctx);
    uint32 globals_offset_val = e_offset_val;
#if WASM_ENABLE_JIT != 0
    if (comp_ctx->is_jit_mode)
        globals_offset_val += offsetof(WASMModuleInstanceExtra, globals);
    else
#endif
        globals_offset_val += offsetof(AOTModuleInstanceExtra, globals);

    LLVMValueRef globals_offset = I32_CONST(globals_offset_val);
    if (!globals_offset) {
        aot_set_last_error("I32_CONST failed for globals_offset");
        return NULL;
    }

    LLVMValueRef globals_ptr =
        LLVMBuildInBoundsGEP2(comp_ctx->builder, INT8_TYPE, func_ctx->aot_inst,
                              &globals_offset, 1, "globals_ptr");
    if (!globals_ptr) {
        aot_set_last_error("LLVMBuildInBoundsGEP2 failed for globals_ptr");
        return NULL;
    }

    LLVMValueRef globals =
        LLVMBuildLoad2(comp_ctx->builder, OPQ_PTR_TYPE, globals_ptr, "globals");
    if (!globals) {
        aot_set_last_error("LLVMBuildLoad2 failed for globals");
        return NULL;
    }

    /* WASMGlobalInstance[global_idx] */
    uint32 global_offset_val = global_idx * sizeof(WASMGlobalInstance);
    LLVMValueRef global_offset = I32_CONST(global_offset_val);
    if (!global_offset) {
        aot_set_last_error("I32_CONST failed for global_offset");
        return NULL;
    }

    LLVMValueRef global_ptr = LLVMBuildInBoundsGEP2(
        comp_ctx->builder, INT8_TYPE, globals, &global_offset, 1, "global_ptr");
    if (!global_ptr) {
        aot_set_last_error("LLVMBuildInBoundsGEP2 failed for global_ptr");
        return NULL;
    }

    return global_ptr;
}

static LLVMValueRef
get_global_value_addr(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                      uint32 global_idx)
{
    /* WASMGlobalInstance[global_idx] */
    LLVMValueRef global_ptr =
        get_global_from_wasm_inst(comp_ctx, func_ctx, global_idx);
    if (!global_ptr) {
        aot_set_last_error("get_global_from_wasm_inst failed");
        return NULL;
    }

    /* WASMGlobalInstance->import_module_inst */
    uint32 import_inst_offset_val =
        offsetof(WASMGlobalInstance, import_module_inst);
    LLVMValueRef import_inst_offset = I32_CONST(import_inst_offset_val);
    if (!import_inst_offset) {
        aot_set_last_error("I32_CONST failed for import_inst_offset");
        return NULL;
    }

    LLVMValueRef import_inst_ptr_u8 =
        LLVMBuildInBoundsGEP2(comp_ctx->builder, INT8_TYPE, global_ptr,
                              &import_inst_offset, 1, "import_inst_u8_ptr");
    if (!import_inst_ptr_u8) {
        aot_set_last_error(
            "LLVMBuildInBoundsGEP2 failed for import_inst_ptr_u8");
        return NULL;
    }

    LLVMValueRef import_inst_ptr = LLVMBuildBitCast(
        comp_ctx->builder, import_inst_ptr_u8, OPQ_PTR_TYPE, "import_inst_ptr");
    if (!import_inst_ptr) {
        aot_set_last_error("LLVMBuildBitCast failed for import_inst_ptr");
        return NULL;
    }

    LLVMValueRef import_inst = LLVMBuildLoad2(comp_ctx->builder, OPQ_PTR_TYPE,
                                              import_inst_ptr, "import_inst");
    if (!import_inst) {
        aot_set_last_error("LLVMBuildLoad2 failed for import_inst");
        return NULL;
    }

    /* WASMGlobalInstance->data_offset */
    uint32 data_offset_offset_val = offsetof(WASMGlobalInstance, data_offset);
    LLVMValueRef data_offset_offset = I32_CONST(data_offset_offset_val);
    if (!data_offset_offset) {
        aot_set_last_error("I32_CONST failed for data_offset_offset");
        return NULL;
    }

    LLVMValueRef data_offset_ptr =
        LLVMBuildInBoundsGEP2(comp_ctx->builder, INT8_TYPE, global_ptr,
                              &data_offset_offset, 1, "data_offset_ptr");
    if (!data_offset_ptr) {
        aot_set_last_error("LLVMBuildInBoundsGEP2 failed for data_offset_ptr");
        return NULL;
    }

    LLVMValueRef data_offset = LLVMBuildLoad2(comp_ctx->builder, I32_TYPE,
                                              data_offset_ptr, "data_offset");
    if (!data_offset) {
        aot_set_last_error("LLVMBuildLoad2 failed for data_offset");
        return NULL;
    }

    /* WASMModuleInstance->global_data preparation */
    uint32 global_data_offset_val =
        offsetof(WASMModuleInstance, global_table_data.bytes)
        + sizeof(AOTMemoryInstance)
              * (comp_ctx->comp_data->memory_count
                 + comp_ctx->comp_data->import_memory_count);
    LLVMValueRef global_data_offset = I32_CONST(global_data_offset_val);
    if (!global_data_offset) {
        aot_set_last_error("I32_CONST failed for global_data_offset");
        return NULL;
    }

    // Check if import_module_inst is NULL
    LLVMBasicBlockRef then_block = LLVMAppendBasicBlockInContext(
        comp_ctx->context, func_ctx->func, "then");
    if (!then_block) {
        aot_set_last_error(
            "LLVMAppendBasicBlockInContext failed for then_block");
        return NULL;
    }

    LLVMBasicBlockRef else_block = LLVMAppendBasicBlockInContext(
        comp_ctx->context, func_ctx->func, "else");
    if (!else_block) {
        aot_set_last_error(
            "LLVMAppendBasicBlockInContext failed for else_block");
        return NULL;
    }

    LLVMBasicBlockRef merge_block = LLVMAppendBasicBlockInContext(
        comp_ctx->context, func_ctx->func, "merge");
    if (!merge_block) {
        aot_set_last_error(
            "LLVMAppendBasicBlockInContext failed for merge_block");
        return NULL;
    }

    LLVMValueRef terminator = LLVMBuildCondBr(
        comp_ctx->builder,
        LLVMBuildIsNull(comp_ctx->builder, import_inst, "is_null"), then_block,
        else_block);
    if (!terminator) {
        aot_set_last_error("LLVMBuildCondBr failed");
        return NULL;
    }

    // If import_module_inst is NULL
    LLVMPositionBuilderAtEnd(comp_ctx->builder, then_block);
    // load global_data from local module instance
    LLVMValueRef local_global_data =
        LLVMBuildInBoundsGEP2(comp_ctx->builder, INT8_TYPE, func_ctx->aot_inst,
                              &global_data_offset, 1, "local_global_data");
    if (!local_global_data) {
        aot_set_last_error(
            "LLVMBuildInBoundsGEP2 failed for local_global_data");
        return NULL;
    }

    // global value pointer
    LLVMValueRef local_global_value_ptr =
        LLVMBuildInBoundsGEP2(comp_ctx->builder, INT8_TYPE, local_global_data,
                              &data_offset, 1, "local_global_value_ptr");
    if (!local_global_value_ptr) {
        aot_set_last_error(
            "LLVMBuildInBoundsGEP2 failed for local_global_value_ptr");
        return NULL;
    }

    terminator = LLVMBuildBr(comp_ctx->builder, merge_block);
    if (!terminator) {
        aot_set_last_error("LLVMBuildBr failed");
        return NULL;
    }

    // If import_module_inst is not NULL
    LLVMPositionBuilderAtEnd(comp_ctx->builder, else_block);
    // load global_data in import module instance
    LLVMValueRef import_global_data =
        LLVMBuildInBoundsGEP2(comp_ctx->builder, INT8_TYPE, import_inst,
                              &global_data_offset, 1, "import_global_data");
    if (!import_global_data) {
        aot_set_last_error(
            "LLVMBuildInBoundsGEP2 failed for import_global_data");
        return NULL;
    }

    LLVMValueRef import_global_value_ptr =
        LLVMBuildInBoundsGEP2(comp_ctx->builder, INT8_TYPE, import_global_data,
                              &data_offset, 1, "import_global_value_ptr");
    if (!import_global_value_ptr) {
        aot_set_last_error(
            "LLVMBuildInBoundsGEP2 failed for import_global_value_ptr");
        return NULL;
    }

    terminator = LLVMBuildBr(comp_ctx->builder, merge_block);
    if (!terminator) {
        aot_set_last_error("LLVMBuildBr failed");
        return NULL;
    }

    // Merge block
    LLVMPositionBuilderAtEnd(comp_ctx->builder, merge_block);
    LLVMValueRef phi =
        LLVMBuildPhi(comp_ctx->builder, OPQ_PTR_TYPE, "global_value_addr");
    if (!phi) {
        aot_set_last_error("LLVMBuildPhi failed for global_value_addr");
        return NULL;
    }

    LLVMAddIncoming(phi, &local_global_value_ptr, &then_block, 1);
    LLVMAddIncoming(phi, &import_global_value_ptr, &else_block, 1);

    return phi;
}

static bool
compile_global(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
               uint32 global_idx, bool is_set, bool is_aux_stack)
{
    const AOTCompData *comp_data = comp_ctx->comp_data;
    uint32 import_global_count = comp_data->import_global_count;
    uint8 global_type;
    LLVMValueRef global_ptr, global, res;
    LLVMTypeRef ptr_type = NULL;

    bh_assert(global_idx < import_global_count + comp_data->global_count);

    if (global_idx < import_global_count) {
        global_type = comp_data->import_globals[global_idx].type.val_type;
    }
    else {
        global_type =
            comp_data->globals[global_idx - import_global_count].type.val_type;
    }

    if (comp_ctx->enable_gc && aot_is_type_gc_reftype(global_type))
        global_type = VALUE_TYPE_GC_REF;

    global_ptr = get_global_value_addr(comp_ctx, func_ctx, global_idx);

    switch (global_type) {
        case VALUE_TYPE_I32:
        case VALUE_TYPE_EXTERNREF:
        case VALUE_TYPE_FUNCREF:
            ptr_type = INT32_PTR_TYPE;
            break;
        case VALUE_TYPE_I64:
            ptr_type = INT64_PTR_TYPE;
            break;
        case VALUE_TYPE_F32:
            ptr_type = F32_PTR_TYPE;
            break;
        case VALUE_TYPE_F64:
            ptr_type = F64_PTR_TYPE;
            break;
        case VALUE_TYPE_V128:
            ptr_type = V128_PTR_TYPE;
            break;
#if WASM_ENABLE_GC != 0
        case VALUE_TYPE_GC_REF:
            ptr_type = GC_REF_PTR_TYPE;
            break;
#endif
        default:
            bh_assert("unknown type");
            break;
    }

    if (!(global_ptr = LLVMBuildBitCast(comp_ctx->builder, global_ptr, ptr_type,
                                        "global_ptr"))) {
        aot_set_last_error("llvm build bit cast failed.");
        return false;
    }

    if (!is_set) {
        if (!(global =
                  LLVMBuildLoad2(comp_ctx->builder, TO_LLVM_TYPE(global_type),
                                 global_ptr, "global"))) {
            aot_set_last_error("llvm build load failed.");
            return false;
        }
        /* All globals' data is 4-byte aligned */
        LLVMSetAlignment(global, 4);
        PUSH(global, global_type);
    }
    else {
        POP(global, global_type);

        if (is_aux_stack && comp_ctx->enable_aux_stack_check) {
            LLVMBasicBlockRef block_curr =
                LLVMGetInsertBlock(comp_ctx->builder);
            LLVMBasicBlockRef check_overflow_succ, check_underflow_succ;
            LLVMValueRef cmp, global_i64;

            /* Add basic blocks */
            if (!(check_overflow_succ = LLVMAppendBasicBlockInContext(
                      comp_ctx->context, func_ctx->func,
                      "check_overflow_succ"))) {
                aot_set_last_error("llvm add basic block failed.");
                return false;
            }
            LLVMMoveBasicBlockAfter(check_overflow_succ, block_curr);

            if (!(check_underflow_succ = LLVMAppendBasicBlockInContext(
                      comp_ctx->context, func_ctx->func,
                      "check_underflow_succ"))) {
                aot_set_last_error("llvm add basic block failed.");
                return false;
            }
            LLVMMoveBasicBlockAfter(check_underflow_succ, check_overflow_succ);

            if (!(global_i64 = LLVMBuildZExt(comp_ctx->builder, global,
                                             I64_TYPE, "global_i64"))) {
                aot_set_last_error("llvm build zext failed.");
                return false;
            }

            /* Check aux stack overflow */
            if (!(cmp = LLVMBuildICmp(comp_ctx->builder, LLVMIntULE, global_i64,
                                      func_ctx->aux_stack_bound, "cmp"))) {
                aot_set_last_error("llvm build icmp failed.");
                return false;
            }
            if (!aot_emit_exception(comp_ctx, func_ctx, EXCE_AUX_STACK_OVERFLOW,
                                    true, cmp, check_overflow_succ)) {
                return false;
            }

            /* Check aux stack underflow */
            LLVMPositionBuilderAtEnd(comp_ctx->builder, check_overflow_succ);
            if (!(cmp = LLVMBuildICmp(comp_ctx->builder, LLVMIntUGT, global_i64,
                                      func_ctx->aux_stack_bottom, "cmp"))) {
                aot_set_last_error("llvm build icmp failed.");
                return false;
            }
            if (!aot_emit_exception(comp_ctx, func_ctx,
                                    EXCE_AUX_STACK_UNDERFLOW, true, cmp,
                                    check_underflow_succ)) {
                return false;
            }

            LLVMPositionBuilderAtEnd(comp_ctx->builder, check_underflow_succ);
        }

        if (!(res = LLVMBuildStore(comp_ctx->builder, global, global_ptr))) {
            aot_set_last_error("llvm build store failed.");
            return false;
        }
        /* All globals' data is 4-byte aligned */
        LLVMSetAlignment(res, 4);
    }

    return true;
fail:
    return false;
}

bool
aot_compile_op_get_global(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          uint32 global_idx)
{
    return compile_global(comp_ctx, func_ctx, global_idx, false, false);
}

bool
aot_compile_op_set_global(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          uint32 global_idx, bool is_aux_stack)
{
    return compile_global(comp_ctx, func_ctx, global_idx, true, is_aux_stack);
}
