/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_emit_variable.h"
#include "../aot/aot_runtime.h"

#define CHECK_LOCAL(idx) do {                               \
    if (idx >= func_ctx->aot_func->func_type->param_count   \
               + func_ctx->aot_func->local_count) {         \
      aot_set_last_error("local index out of range");       \
      return false;                                         \
    }                                                       \
  } while (0)

static uint8
get_local_type(AOTFuncContext *func_ctx, uint32 local_idx)
{
    AOTFunc *aot_func = func_ctx->aot_func;
    uint32 param_count = aot_func->func_type->param_count;
    return local_idx < param_count
           ? aot_func->func_type->types[local_idx]
           : aot_func->local_types[local_idx - param_count];
}

bool
aot_compile_op_get_local(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 local_idx)
{
    char name[32];
    LLVMValueRef value;
    AOTValue *aot_value;

    CHECK_LOCAL(local_idx);

    snprintf(name, sizeof(name), "%s%d%s", "local", local_idx, "#");
    if (!(value = LLVMBuildLoad(comp_ctx->builder,
                                func_ctx->locals[local_idx],
                                name))) {
        aot_set_last_error("llvm build load fail");
        return false;
    }

    PUSH(value, get_local_type(func_ctx, local_idx));

    aot_value = func_ctx->block_stack.block_list_end->value_stack.value_list_end;
    aot_value->is_local = true;
    aot_value->local_idx = local_idx;
    return true;

fail:
    return false;
}

bool
aot_compile_op_set_local(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 local_idx)
{
    LLVMValueRef value;

    CHECK_LOCAL(local_idx);

    POP(value, get_local_type(func_ctx, local_idx));

    if (!LLVMBuildStore(comp_ctx->builder,
                        value,
                        func_ctx->locals[local_idx])) {
        aot_set_last_error("llvm build store fail");
        return false;
    }

    aot_checked_addr_list_del(func_ctx, local_idx);
    return true;

fail:
    return false;
}

bool
aot_compile_op_tee_local(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 local_idx)
{
    LLVMValueRef value;
    uint8 type;

    CHECK_LOCAL(local_idx);

    type = get_local_type(func_ctx, local_idx);

    POP(value, type);

    if (!LLVMBuildStore(comp_ctx->builder,
                        value,
                        func_ctx->locals[local_idx])) {
        aot_set_last_error("llvm build store fail");
        return false;
    }

    PUSH(value, type);
    aot_checked_addr_list_del(func_ctx, local_idx);
    return true;

fail:
    return false;
}

static bool
compile_global(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
               uint32 global_idx, bool is_set)
{
    AOTCompData *comp_data = comp_ctx->comp_data;
    uint32 import_global_count = comp_data->import_global_count;
    uint32 global_base_offset =
        offsetof(AOTModuleInstance, global_table_data.bytes)
        + sizeof(AOTMemoryInstance) * comp_ctx->comp_data->memory_count;
    uint32 global_offset;
    uint8 global_type;
    LLVMValueRef offset, global_ptr, global;
    LLVMTypeRef ptr_type = NULL;

    bh_assert(global_idx < import_global_count + comp_data->global_count);

    if (global_idx < import_global_count) {
        global_offset = global_base_offset
                        + comp_data->import_globals[global_idx].data_offset;
        global_type = comp_data->import_globals[global_idx].type;
    }
    else {
        global_offset = global_base_offset
            + comp_data->globals[global_idx - import_global_count].data_offset;
        global_type =
            comp_data->globals[global_idx - import_global_count].type;
    }

    offset = I32_CONST(global_offset);
    if (!(global_ptr = LLVMBuildInBoundsGEP(comp_ctx->builder, func_ctx->aot_inst,
                                            &offset, 1, "global_ptr_tmp"))) {
        aot_set_last_error("llvm build in bounds gep failed.");
        return false;
    }

    switch (global_type) {
        case VALUE_TYPE_I32:
            ptr_type = comp_ctx->basic_types.int32_ptr_type;
            break;
        case VALUE_TYPE_I64:
            ptr_type = comp_ctx->basic_types.int64_ptr_type;
            break;
        case VALUE_TYPE_F32:
            ptr_type = comp_ctx->basic_types.float32_ptr_type;
            break;
        case VALUE_TYPE_F64:
            ptr_type = comp_ctx->basic_types.float64_ptr_type;
            break;
        default:
            bh_assert(0);
            break;
    }

    if (!(global_ptr = LLVMBuildBitCast(comp_ctx->builder, global_ptr,
                                        ptr_type, "global_ptr"))) {
        aot_set_last_error("llvm build bit cast failed.");
        return false;
    }

    if (!is_set) {
        if (!(global = LLVMBuildLoad(comp_ctx->builder,
                                     global_ptr, "global"))) {
            aot_set_last_error("llvm build load failed.");
            return false;
        }
        PUSH(global, global_type);
    }
    else {
        POP(global, global_type);
        if (!LLVMBuildStore(comp_ctx->builder, global, global_ptr)) {
            aot_set_last_error("llvm build store failed.");
            return false;
        }
    }

    return true;
fail:
    return false;
}

bool
aot_compile_op_get_global(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          uint32 global_idx)
{
    return compile_global(comp_ctx, func_ctx, global_idx, false);
}

bool
aot_compile_op_set_global(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                          uint32 global_idx)
{
    return compile_global(comp_ctx, func_ctx, global_idx, true);
}

