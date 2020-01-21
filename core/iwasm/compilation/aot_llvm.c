/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_llvm.h"
#include "bh_memory.h"
#include "aot_compiler.h"
#include "../aot/aot_runtime.h"


LLVMTypeRef
wasm_type_to_llvm_type(AOTLLVMTypes *llvm_types, uint8 wasm_type)
{
    switch (wasm_type) {
        case VALUE_TYPE_I32:
            return llvm_types->int32_type;
        case VALUE_TYPE_I64:
            return llvm_types->int64_type;
        case VALUE_TYPE_F32:
            return llvm_types->float32_type;
        case VALUE_TYPE_F64:
            return llvm_types->float64_type;
        case VALUE_TYPE_VOID:
            return llvm_types->void_type;
    }
    return NULL;
}

/**
 * Add LLVM function
 */
static LLVMValueRef
aot_add_llvm_func(AOTCompContext *comp_ctx, AOTFuncType *aot_func_type,
                  uint32 func_index)
{
    LLVMValueRef func = NULL;
    LLVMTypeRef *param_types, ret_type, func_type;
    LLVMValueRef local_value;
    char func_name[32];
    uint64 size;
    uint32 i, j = 0, param_count = (uint64)aot_func_type->param_count;

    /* aot context as first parameter */
    param_count++;

    /* Initialize parameter types of the LLVM function */
    size = sizeof(LLVMTypeRef) * ((uint64)param_count);
    if (size >= UINT32_MAX
        || !(param_types = wasm_malloc((uint32)size))) {
        aot_set_last_error("allocate memory failed.");
        return NULL;
    }

    /* exec env as first parameter */
    param_types[j++] = comp_ctx->exec_env_type;
    for (i = 0; i < aot_func_type->param_count; i++)
        param_types[j++] = TO_LLVM_TYPE(aot_func_type->types[i]);

    /* Resolve return type of the LLVM function */
    if (aot_func_type->result_count)
        ret_type = TO_LLVM_TYPE(aot_func_type->types[aot_func_type->param_count]);
    else
        ret_type = VOID_TYPE;

    /* Resolve function prototype */
    if (!(func_type = LLVMFunctionType(ret_type, param_types,
                                       param_count, false))) {
        aot_set_last_error("create LLVM function type failed.");
        goto fail;
    }

    /* Add LLVM function */
    snprintf(func_name, sizeof(func_name), "%s%d", AOT_FUNC_PREFIX, func_index);
    if (!(func = LLVMAddFunction(comp_ctx->module, func_name, func_type))) {
        aot_set_last_error("add LLVM function failed.");
        goto fail;
    }

    j = 0;
    local_value = LLVMGetParam(func, j++);
    LLVMSetValueName(local_value, "exec_env");

    /* Set parameter names */
    for (i = 0; i < aot_func_type->param_count; i++) {
        local_value = LLVMGetParam(func, j++);
        LLVMSetValueName(local_value, "");
    }

fail:
    wasm_free(param_types);
    return func;
}

/**
 * Create first AOTBlock, or function block for the function
 */
static AOTBlock *
aot_create_func_block(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                      AOTFunc *func, AOTFuncType *aot_func_type)
{
    AOTBlock *aot_block;

    /* Allocate memory */
    if (!(aot_block = wasm_malloc(sizeof(AOTBlock)))) {
        aot_set_last_error("allocate memory failed.");
        return NULL;
    }

    memset(aot_block, 0, sizeof(AOTBlock));

    /* Set block type and return type */
    aot_block->block_type = BLOCK_TYPE_FUNCTION;
    if (aot_func_type->result_count)
        aot_block->return_type = aot_func_type->types[aot_func_type->param_count];
    else
        aot_block->return_type = VALUE_TYPE_VOID;

    aot_block->wasm_code_end = func->code + func->code_size;

    /* Add function entry block */
    if (!(aot_block->llvm_entry_block =
                LLVMAppendBasicBlockInContext(comp_ctx->context, func_ctx->func,
                                              "func_begin"))) {
        aot_set_last_error("add LLVM basic block failed.");
        goto fail;
    }

    return aot_block;

fail:
    wasm_free(aot_block);
    return NULL;
}

static bool
create_exception_blocks(AOTFuncContext *func_ctx)
{
    if (!(func_ctx->exception_blocks =
                wasm_malloc(sizeof(LLVMBasicBlockRef) * EXCE_NUM))) {
        aot_set_last_error("allocate memory failed.");
        return false;;
    }

    memset(func_ctx->exception_blocks, 0,
           sizeof(LLVMBasicBlockRef) * EXCE_NUM);
  return true;
}

static bool
create_memory_info(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                   LLVMTypeRef int8_ptr_type, uint32 func_index)
{
    LLVMValueRef offset;
    WASMModule *module = comp_ctx->comp_data->wasm_module;
    WASMFunction *func = module->functions[func_index];
    bool mem_space_unchanged = (!func->has_op_memory_grow && !func->has_op_func_call)
                               || (!module->possible_memory_grow);

    func_ctx->mem_space_unchanged = mem_space_unchanged;

    /* Load memory base address */
    offset = I32_CONST(offsetof(AOTModuleInstance, memory_data.ptr));
    if (!(func_ctx->mem_base_addr =
                LLVMBuildInBoundsGEP(comp_ctx->builder, func_ctx->aot_inst,
                                     &offset, 1, "mem_base_addr_offset"))) {
        aot_set_last_error("llvm build in bounds gep failed");
        return false;
    }
    if (!(func_ctx->mem_base_addr =
                LLVMBuildBitCast(comp_ctx->builder, func_ctx->mem_base_addr,
                                 int8_ptr_type, "mem_base_addr_ptr"))) {
        aot_set_last_error("llvm build bit cast failed");
        return false;
    }
    if (mem_space_unchanged) {
        if (!(func_ctx->mem_base_addr =
                    LLVMBuildLoad(comp_ctx->builder, func_ctx->mem_base_addr,
                                  "mem_base_addr"))) {
            aot_set_last_error("llvm build load failed");
            return false;
        }
    }

    /* Load memory data size */
    offset = I32_CONST(offsetof(AOTModuleInstance, memory_data_size));
    if (!(func_ctx->mem_data_size =
                LLVMBuildInBoundsGEP(comp_ctx->builder, func_ctx->aot_inst,
                                     &offset, 1, "mem_data_size_offset"))) {
        aot_set_last_error("llvm build in bounds gep failed");
        return false;
    }
    if (!(func_ctx->mem_data_size =
                LLVMBuildBitCast(comp_ctx->builder, func_ctx->mem_data_size,
                                 INT32_PTR_TYPE, "mem_data_size_ptr"))) {
        aot_set_last_error("llvm build bit cast failed");
        return false;
    }
    if (mem_space_unchanged) {
        if (!(func_ctx->mem_data_size =
                    LLVMBuildLoad(comp_ctx->builder, func_ctx->mem_data_size,
                                  "mem_data_size"))) {
            aot_set_last_error("llvm build load failed");
            return false;
        }
        if (!(func_ctx->mem_bound_1_byte =
                    LLVMBuildSub(comp_ctx->builder,
                                 func_ctx->mem_data_size, I32_ONE,
                                 "mem_bound_1_byte"))
            || !(func_ctx->mem_bound_2_bytes =
                    LLVMBuildSub(comp_ctx->builder,
                                 func_ctx->mem_data_size, I32_TWO,
                                 "mem_bound_2_bytes"))
            || !(func_ctx->mem_bound_4_bytes =
                    LLVMBuildSub(comp_ctx->builder,
                                 func_ctx->mem_data_size, I32_FOUR,
                                 "mem_bound_4_bytes"))
            || !(func_ctx->mem_bound_8_bytes =
                    LLVMBuildSub(comp_ctx->builder,
                                 func_ctx->mem_data_size, I32_EIGHT,
                                 "mem_bound_8_bytes"))) {
            aot_set_last_error("llvm build sub failed");
            return false;
        }
    }

    /* Load heap base address */
    offset = I32_CONST(offsetof(AOTModuleInstance, heap_data.ptr));
    if (!(func_ctx->heap_base_addr =
                LLVMBuildInBoundsGEP(comp_ctx->builder, func_ctx->aot_inst,
                                     &offset, 1, "heap_base_addr_offset"))) {
        aot_set_last_error("llvm build in bounds gep failed");
        return false;
    }
    if (!(func_ctx->heap_base_addr =
                LLVMBuildBitCast(comp_ctx->builder, func_ctx->heap_base_addr,
                                 int8_ptr_type, "heap_base_addr_tmp"))) {
        aot_set_last_error("llvm build bit cast failed");
        return false;
    }
    if (!(func_ctx->heap_base_addr =
                LLVMBuildLoad(comp_ctx->builder, func_ctx->heap_base_addr,
                              "heap_base_addr"))) {
        aot_set_last_error("llvm build load failed");
        return false;
    }

    /* Load heap base offset */
    offset = I32_CONST(offsetof(AOTModuleInstance, heap_base_offset));
    if (!(func_ctx->heap_base_offset =
                LLVMBuildInBoundsGEP(comp_ctx->builder, func_ctx->aot_inst,
                                     &offset, 1, "heap_base_offset_offset"))) {
        aot_set_last_error("llvm build in bounds gep failed");
        return false;
    }
    if (!(func_ctx->heap_base_offset =
                LLVMBuildBitCast(comp_ctx->builder, func_ctx->heap_base_offset,
                                 INT32_PTR_TYPE, "heap_base_offset_tmp"))) {
        aot_set_last_error("llvm build bit cast failed");
        return false;
    }
    if (!(func_ctx->heap_base_offset =
                LLVMBuildLoad(comp_ctx->builder, func_ctx->heap_base_offset,
                              "heap_base_offset"))) {
        aot_set_last_error("llvm build load failed");
        return false;
    }

    /* Load heap data size */
    offset = I32_CONST(offsetof(AOTModuleInstance, heap_data_size));
    if (!(func_ctx->heap_data_size =
                LLVMBuildInBoundsGEP(comp_ctx->builder, func_ctx->aot_inst,
                                     &offset, 1, "heap_data_size_offset"))) {
        aot_set_last_error("llvm build in bounds gep failed");
        return false;
    }
    if (!(func_ctx->heap_data_size =
                LLVMBuildBitCast(comp_ctx->builder, func_ctx->heap_data_size,
                                 INT32_PTR_TYPE, "heap_data_size_tmp"))) {
        aot_set_last_error("llvm build bit cast failed");
        return false;
    }
    if (!(func_ctx->heap_data_size =
                LLVMBuildLoad(comp_ctx->builder, func_ctx->heap_data_size,
                              "heap_data_size"))) {
        aot_set_last_error("llvm build load failed");
        return false;
    }
    if (!(func_ctx->heap_bound_1_byte =
                LLVMBuildSub(comp_ctx->builder,
                             func_ctx->heap_data_size, I32_ONE,
                             "heap_bound_1_byte"))
            || !(func_ctx->heap_bound_2_bytes =
                LLVMBuildSub(comp_ctx->builder,
                             func_ctx->heap_data_size, I32_TWO,
                             "heap_bound_2_bytes"))
            || !(func_ctx->heap_bound_4_bytes =
                LLVMBuildSub(comp_ctx->builder,
                             func_ctx->heap_data_size, I32_FOUR,
                             "heap_bound_4_bytes"))
            || !(func_ctx->heap_bound_8_bytes =
                LLVMBuildSub(comp_ctx->builder,
                             func_ctx->heap_data_size, I32_EIGHT,
                             "heap_bound_8_bytes"))) {
        aot_set_last_error("llvm build sub failed");
        return false;
    }

    return true;
}

static bool
create_table_base(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef offset;

    offset = I32_CONST(offsetof(AOTModuleInstance, global_table_heap_data.bytes)
                       + comp_ctx->comp_data->global_data_size);
    func_ctx->table_base = LLVMBuildInBoundsGEP(comp_ctx->builder,
                                                func_ctx->aot_inst,
                                                &offset, 1,
                                                "table_base_tmp");
    if (!func_ctx->table_base) {
        aot_set_last_error("llvm build in bounds gep failed.");
        return false;
    }
    func_ctx->table_base = LLVMBuildBitCast(comp_ctx->builder, func_ctx->table_base,
                                            INT32_PTR_TYPE, "table_base");
    if (!func_ctx->table_base) {
        aot_set_last_error("llvm build bit cast failed.");
        return false;
    }
    return true;
}

static bool
create_cur_exception(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef offset;

    offset = I32_CONST(offsetof(AOTModuleInstance, cur_exception));
    func_ctx->cur_exception = LLVMBuildInBoundsGEP(comp_ctx->builder,
                                                   func_ctx->aot_inst,
                                                   &offset, 1,
                                                   "cur_execption");
    if (!func_ctx->cur_exception) {
        aot_set_last_error("llvm build in bounds gep failed.");
        return false;
    }
    return true;
}

static bool
create_func_ptrs(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx)
{
    LLVMValueRef offset, func_ptrs_ptr;
    LLVMTypeRef void_ptr_type;

    offset = I32_CONST(offsetof(AOTModuleInstance, func_ptrs.ptr));
    func_ptrs_ptr = LLVMBuildInBoundsGEP(comp_ctx->builder,
                                         func_ctx->aot_inst,
                                         &offset, 1,
                                         "func_ptrs_ptr");
    if (!func_ptrs_ptr) {
        aot_set_last_error("llvm build in bounds gep failed.");
        return false;
    }

    if (!(void_ptr_type = LLVMPointerType(VOID_PTR_TYPE, 0))
        || !(void_ptr_type = LLVMPointerType(void_ptr_type, 0))) {
        aot_set_last_error("llvm get pointer type failed.");
        return false;
    }

    func_ctx->func_ptrs = LLVMBuildBitCast(comp_ctx->builder, func_ptrs_ptr,
                                           void_ptr_type, "func_ptrs_tmp");
    if (!func_ctx->func_ptrs) {
        aot_set_last_error("llvm build bit cast failed.");
        return false;
    }

    func_ctx->func_ptrs = LLVMBuildLoad(comp_ctx->builder, func_ctx->func_ptrs,
            "func_ptrs");
    if (!func_ctx->func_ptrs) {
        aot_set_last_error("llvm build load failed.");
        return false;
    }

    return true;
}

static bool
create_func_type_indexes(AOTCompContext *comp_ctx,
                         AOTFuncContext *func_ctx)
{
    LLVMValueRef offset, func_type_indexes_ptr;
    LLVMTypeRef int32_ptr_type;

    offset = I32_CONST(offsetof(AOTModuleInstance, func_type_indexes.ptr));
    func_type_indexes_ptr = LLVMBuildInBoundsGEP(comp_ctx->builder,
                                                 func_ctx->aot_inst,
                                                 &offset, 1,
                                                 "func_type_indexes_ptr");
    if (!func_type_indexes_ptr) {
        aot_set_last_error("llvm build add failed.");
        return false;
    }

    if (!(int32_ptr_type = LLVMPointerType(INT32_PTR_TYPE, 0))) {
        aot_set_last_error("llvm get pointer type failed.");
        return false;
    }

    func_ctx->func_type_indexes = LLVMBuildBitCast(comp_ctx->builder,
                                                   func_type_indexes_ptr,
                                                   int32_ptr_type,
                                                   "func_type_indexes_tmp");
    if (!func_ctx->func_type_indexes) {
        aot_set_last_error("llvm build bit cast failed.");
        return false;
    }

    func_ctx->func_type_indexes = LLVMBuildLoad(comp_ctx->builder,
                                                func_ctx->func_type_indexes,
                                                "func_type_indexes");
    if (!func_ctx->func_type_indexes) {
        aot_set_last_error("llvm build load failed.");
        return false;
    }
    return true;
}

/**
 * Create function compiler context
 */
static AOTFuncContext *
aot_create_func_context(AOTCompData *comp_data, AOTCompContext *comp_ctx,
                        AOTFunc *func, uint32 func_index)
{
    AOTFuncContext *func_ctx;
    AOTFuncType *aot_func_type = comp_data->func_types[func->func_type_index];
    AOTBlock *aot_block;
    LLVMTypeRef int8_ptr_type;
    LLVMValueRef aot_inst_offset = I32_TWO, aot_inst_addr;
    char local_name[32];
    uint64 size;
    uint32 i, j = 0;

    /* Allocate memory for the function context */
    size = offsetof(AOTFuncContext, locals) + sizeof(LLVMValueRef) *
                    ((uint64)aot_func_type->param_count + func->local_count);
    if (size >= UINT32_MAX
        || !(func_ctx = wasm_malloc((uint32)size))) {
        aot_set_last_error("allocate memory failed.");
        return NULL;
    }

    memset(func_ctx, 0, (uint32)size);
    func_ctx->aot_func = func;

    /* Add LLVM function */
    if (!(func_ctx->func = aot_add_llvm_func(comp_ctx, aot_func_type, func_index)))
        goto fail;

    /* Create function's first AOTBlock */
    if (!(aot_block = aot_create_func_block(comp_ctx, func_ctx,
                                            func, aot_func_type)))
        goto fail;

    aot_block_stack_push(&func_ctx->block_stack, aot_block);

    /* Add local variables */
    LLVMPositionBuilderAtEnd(comp_ctx->builder, aot_block->llvm_entry_block);

    /* Save the pameters for fast access */
    func_ctx->exec_env = LLVMGetParam(func_ctx->func, j++);

    /* Get aot inst address, the layout of exec_env is:
       exec_env->next, exec_env->prev, and exec_env->module_inst */
    if (!(aot_inst_addr =
                LLVMBuildInBoundsGEP(comp_ctx->builder, func_ctx->exec_env,
                                     &aot_inst_offset, 1, "aot_inst_addr"))) {
        aot_set_last_error("llvm build in bounds gep failed");
        goto fail;
    }

    /* Load aot inst */
    if (!(func_ctx->aot_inst = LLVMBuildLoad(comp_ctx->builder,
                                             aot_inst_addr, "aot_inst"))) {
        aot_set_last_error("llvm build load failed");
        goto fail;
    }

    for (i = 0; i < aot_func_type->param_count; i++, j++) {
        snprintf(local_name, sizeof(local_name), "l%d", i);
        func_ctx->locals[i] =
            LLVMBuildAlloca(comp_ctx->builder,
                            TO_LLVM_TYPE(aot_func_type->types[i]),
                            local_name);
        if (!func_ctx->locals[i]) {
            aot_set_last_error("llvm build alloca failed.");
            goto fail;
        }
        if (!LLVMBuildStore(comp_ctx->builder,
                            LLVMGetParam(func_ctx->func, j),
                            func_ctx->locals[i])) {
            aot_set_last_error("llvm build store failed.");
            goto fail;
        }
    }

    for (i = 0; i < func->local_count; i++) {
        LLVMTypeRef local_type;
        LLVMValueRef local_value = NULL;
        snprintf(local_name, sizeof(local_name), "l%d",
                 aot_func_type->param_count + i);
        local_type = TO_LLVM_TYPE(func->local_types[i]);
        func_ctx->locals[aot_func_type->param_count + i] =
            LLVMBuildAlloca(comp_ctx->builder, local_type, local_name);
        if (!func_ctx->locals[aot_func_type->param_count + i]) {
            aot_set_last_error("llvm build alloca failed.");
            goto fail;
        }
        switch (func->local_types[i]) {
            case VALUE_TYPE_I32:
                local_value = I32_ZERO;
                break;
            case VALUE_TYPE_I64:
                local_value = I64_ZERO;
                break;
            case VALUE_TYPE_F32:
                local_value = F32_ZERO;
                break;
            case VALUE_TYPE_F64:
                local_value = F64_ZERO;
                break;
            default:
                bh_assert(0);
                break;
        }
        if (!LLVMBuildStore(comp_ctx->builder, local_value,
                            func_ctx->locals[aot_func_type->param_count + i])) {
            aot_set_last_error("llvm build store failed.");
            goto fail;
        }
    }

    if (!(int8_ptr_type = LLVMPointerType(INT8_PTR_TYPE, 0))) {
        aot_set_last_error("llvm add pointer type failed.");
        goto fail;
    }

    /* Create exception blocks */
    if (!create_exception_blocks(func_ctx))
        goto fail;

    /* Create base addr, end addr, data size of mem, heap */
    if (!create_memory_info(comp_ctx, func_ctx, int8_ptr_type, func_index))
        goto fail;

    /* Load table base */
    if (!create_table_base(comp_ctx, func_ctx))
        goto fail;

    /* Load current exception */
    if (!create_cur_exception(comp_ctx, func_ctx))
        goto fail;

    /* Load function pointers */
    if (!create_func_ptrs(comp_ctx, func_ctx))
      goto fail;

    /* Load function type indexes */
    if (!create_func_type_indexes(comp_ctx, func_ctx))
        goto fail;

    return func_ctx;

fail:
    if (func_ctx->exception_blocks)
        wasm_free(func_ctx->exception_blocks);
    aot_block_stack_destroy(&func_ctx->block_stack);
    wasm_free(func_ctx);
    return NULL;
}

static void
aot_destroy_func_contexts(AOTFuncContext **func_ctxes, uint32 count)
{
    uint32 i;

    for (i = 0; i < count; i++)
        if (func_ctxes[i]) {
            if (func_ctxes[i]->exception_blocks)
                wasm_free(func_ctxes[i]->exception_blocks);
            aot_block_stack_destroy(&func_ctxes[i]->block_stack);
            wasm_free(func_ctxes[i]);
        }
    wasm_free(func_ctxes);
}

/**
 * Create function compiler contexts
 */
static AOTFuncContext **
aot_create_func_contexts(AOTCompData *comp_data, AOTCompContext *comp_ctx)
{
    AOTFuncContext **func_ctxes;
    uint64 size;
    uint32 i;

    /* Allocate memory */
    size = sizeof(AOTFuncContext*) * (uint64)comp_data->func_count;
    if (size >= UINT32_MAX
        || !(func_ctxes = wasm_malloc((uint32)size))) {
        aot_set_last_error("allocate memory failed.");
        return NULL;
    }

    memset(func_ctxes, 0, size);

    /* Create each function context */
    for (i = 0; i < comp_data->func_count; i++) {
        AOTFunc *func = comp_data->funcs[i];
        if (!(func_ctxes[i] = aot_create_func_context(comp_data, comp_ctx,
                                                      func, i))) {
            aot_destroy_func_contexts(func_ctxes, comp_data->func_count);
            return NULL;
        }
    }

    return func_ctxes;
}

static bool
aot_set_llvm_basic_types(AOTLLVMTypes *basic_types, LLVMContextRef context)
{
    basic_types->int1_type = LLVMInt1TypeInContext(context);
    basic_types->int8_type = LLVMInt8TypeInContext(context);
    basic_types->int16_type = LLVMInt16TypeInContext(context);
    basic_types->int32_type = LLVMInt32TypeInContext(context);
    basic_types->int64_type = LLVMInt64TypeInContext(context);
    basic_types->float32_type = LLVMFloatTypeInContext(context);
    basic_types->float64_type = LLVMDoubleTypeInContext(context);
    basic_types->void_type = LLVMVoidTypeInContext(context);

    basic_types->meta_data_type = LLVMMetadataTypeInContext(context);

    basic_types->int8_ptr_type = LLVMPointerType(basic_types->int8_type, 0);
    basic_types->int16_ptr_type = LLVMPointerType(basic_types->int16_type, 0);
    basic_types->int32_ptr_type = LLVMPointerType(basic_types->int32_type, 0);
    basic_types->int64_ptr_type = LLVMPointerType(basic_types->int64_type, 0);
    basic_types->float32_ptr_type = LLVMPointerType(basic_types->float32_type, 0);
    basic_types->float64_ptr_type = LLVMPointerType(basic_types->float64_type, 0);
    basic_types->void_ptr_type = LLVMPointerType(basic_types->void_type, 0);

    return (basic_types->int8_ptr_type
            && basic_types->int16_ptr_type
            && basic_types->int32_ptr_type
            && basic_types->int64_ptr_type
            && basic_types->float32_ptr_type
            && basic_types->float64_ptr_type
            && basic_types->void_ptr_type
            && basic_types->meta_data_type) ? true : false;
}

static bool
aot_create_llvm_consts(AOTLLVMConsts *consts, AOTCompContext *comp_ctx)
{
    consts->i8_zero = I8_CONST(0);
    consts->i32_zero = I32_CONST(0);
    consts->i64_zero = I64_CONST(0);
    consts->f32_zero = F32_CONST(0);
    consts->f64_zero = F64_CONST(0);
    consts->i32_one = I32_CONST(1);
    consts->i32_two = I32_CONST(2);
    consts->i32_four = I32_CONST(4);
    consts->i32_eight = I32_CONST(8);
    consts->i32_neg_one = I32_CONST((uint32)-1);
    consts->i64_neg_one = I64_CONST((uint64)-1);
    consts->i32_min = I32_CONST((uint32)INT32_MIN);
    consts->i64_min = I64_CONST((uint64)INT64_MIN);
    consts->i32_31 = I32_CONST(31);
    consts->i32_32 = I32_CONST(32);
    consts->i64_63 = I64_CONST(63);
    consts->i64_64 = I64_CONST(64);

    return (consts->i8_zero
            && consts->i32_zero
            && consts->i64_zero
            && consts->f32_zero
            && consts->f64_zero
            && consts->i32_one
            && consts->i32_two
            && consts->i32_four
            && consts->i32_eight
            && consts->i32_neg_one
            && consts->i64_neg_one
            && consts->i32_min
            && consts->i64_min
            && consts->i32_31
            && consts->i32_32
            && consts->i64_63
            && consts->i64_64) ? true : false;
}

typedef struct ArchItem {
    char *arch;
    bool support_eb;
} ArchItem;

static ArchItem valid_archs[] = {
    { "x86_64", false },
    { "i386", false },
    { "mips", true },
    { "armv4", true },
    { "armv4t", true },
    { "armv5t", true },
    { "armv5te", true },
    { "armv5tej", true },
    { "armv6", true },
    { "armv6kz", true },
    { "armv6t2", true },
    { "armv6k", true },
    { "armv7", true },
    { "armv6m", true },
    { "armv6sm", true },
    { "armv7em", true },
    { "armv8a", true },
    { "armv8r", true },
    { "armv8m.base", true },
    { "armv8m.main", true },
    { "armv8.1m.main", true },
    { "thumbv4", true },
    { "thumbv4t", true },
    { "thumbv5t", true },
    { "thumbv5te", true },
    { "thumbv5tej", true },
    { "thumbv6", true },
    { "thumbv6kz", true },
    { "thumbv6t2", true },
    { "thumbv6k", true },
    { "thumbv7", true },
    { "thumbv6m", true },
    { "thumbv6sm", true },
    { "thumbv7em", true },
    { "thumbv8a", true },
    { "thumbv8r", true },
    { "thumbv8m.base", true },
    { "thumbv8m.main", true },
    { "thumbv8.1m.main", true }
};

static const char *valid_abis[] = {
    "gnu",
    "eabi",
    "gnueabihf"
};

static void
print_supported_targets()
{
    uint32 i;
    bh_printf("Supported targets:\n");
    for (i = 0; i < sizeof(valid_archs) / sizeof(ArchItem); i++) {
        bh_printf("%s ", valid_archs[i].arch);
        if (valid_archs[i].support_eb)
            bh_printf("%seb ", valid_archs[i].arch);
    }
    bh_printf("\n");
}

static void
print_supported_abis()
{
    uint32 i;
    bh_printf("Supported ABI: ");
    for (i = 0; i < sizeof(valid_abis) / sizeof(const char *); i++)
        bh_printf("%s ", valid_abis[i]);
    bh_printf("\n");
}

static bool
check_target_arch(const char *target_arch)
{
    uint32 i;
    char *arch;
    bool support_eb;

    for (i = 0; i < sizeof(valid_archs) / sizeof(ArchItem); i++) {
        arch = valid_archs[i].arch;
        support_eb = valid_archs[i].support_eb;

        if (!strncmp(target_arch, arch, strlen(arch))
            && ((support_eb && (!strcmp(target_arch + strlen(arch), "eb")
                                || !strcmp(target_arch + strlen(arch), "")))
                || (!support_eb && !strcmp(target_arch + strlen(arch), "")))) {
            return true;
        }
    }
    return false;
}

static bool
check_target_abi(const char *target_abi)
{
    uint32 i;
    for (i = 0; i < sizeof(valid_abis) / sizeof(char *); i++) {
        if (!strcmp(target_abi, valid_abis[i]))
            return true;
    }
    return false;
}


static void
get_target_arch_from_triple(const char *triple, char *arch_buf, uint32 buf_size)
{
    uint32 i = 0;
    while (*triple != '-' && *triple != '\0' && i < buf_size - 1)
        arch_buf[i++] = *triple++;
    /* Make sure buffer is long enough */
    bh_assert(*triple == '-' || *triple == '\0');
}

void LLVMAddPromoteMemoryToRegisterPass(LLVMPassManagerRef PM);

AOTCompContext *
aot_create_comp_context(AOTCompData *comp_data,
                        aot_comp_option_t option)
{
    AOTCompContext *comp_ctx, *ret = NULL;
    /*LLVMTypeRef elem_types[8];*/
    struct LLVMMCJITCompilerOptions jit_options;
    LLVMTargetRef target;
    char *triple = NULL, *triple_norm, *arch, *abi, *cpu, *features, buf[128];
    char *triple_norm_new = NULL, *cpu_new = NULL;
    char *err = NULL, *fp_round= "round.tonearest", *fp_exce = "fpexcept.strict";
    char triple_buf[32] = {0};
    uint32 opt_level;

    /* Initialize LLVM environment */
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmPrinters();
    LLVMLinkInMCJIT();

    /* Allocate memory */
    if (!(comp_ctx = wasm_malloc(sizeof(AOTCompContext)))) {
        aot_set_last_error("allocate memory failed.");
        return NULL;
    }

    memset(comp_ctx, 0, sizeof(AOTCompContext));
    comp_ctx->comp_data = comp_data;

    /* Create LLVM context, module and builder */
    if (!(comp_ctx->context = LLVMContextCreate())) {
        aot_set_last_error("create LLVM context failed.");
        goto fail;
    }

    if (!(comp_ctx->builder = LLVMCreateBuilderInContext(comp_ctx->context))) {
        aot_set_last_error("create LLVM builder failed.");
        goto fail;
    }

    if (!(comp_ctx->module =
                LLVMModuleCreateWithNameInContext("WASM Module", comp_ctx->context))) {
        aot_set_last_error("create LLVM module failed.");
        goto fail;
    }

    if (option->is_jit_mode) {
        /* Create LLVM execution engine */
        LLVMInitializeMCJITCompilerOptions(&jit_options, sizeof(jit_options));
        jit_options.OptLevel = LLVMCodeGenLevelAggressive;
        /*jit_options.CodeModel = LLVMCodeModelSmall;*/
        if (LLVMCreateMCJITCompilerForModule
                (&comp_ctx->exec_engine, comp_ctx->module,
                 &jit_options, sizeof(jit_options), &err) != 0) {
            if (err) {
                LLVMDisposeMessage(err);
                err = NULL;
            }
            aot_set_last_error("create LLVM JIT compiler failed.");
            goto fail;
        }
        comp_ctx->is_jit_mode = true;
    }
    else {
        /* Create LLVM target machine */
        arch = option->target_arch;
        abi = option->target_abi;
        cpu = option->target_cpu;
        features = option->cpu_features;
        opt_level = option->opt_level;

        if (arch) {
            /* Add default sub-arch if not specified */
            if (!strcmp(arch, "arm"))
                arch = "armv4";
            else if (!strcmp(arch, "armeb"))
                arch = "armv4eb";
            else if (!strcmp(arch, "thumb"))
                arch = "thumbv4t";
            else if (!strcmp(arch, "thumbeb"))
                arch = "thumbv4teb";
        }

        /* Check target arch */
        if (arch && !check_target_arch(arch)) {
            if (!strcmp(arch, "help"))
                print_supported_targets();
            else
                aot_set_last_error("Invalid target. "
                                   "Use --target=help to list all supported targets");
            goto fail;
        }

        /* Check target ABI */
        if (abi && !check_target_abi(abi)) {
            if (!strcmp(abi, "help"))
                print_supported_abis();
            else
                aot_set_last_error("Invalid target ABI. "
                                   "Use --target-abi=help to list all supported ABI");
            goto fail;
        }

        if (arch) {
            /* Construct target triple: <arch>-<vendor>-<sys>-<abi> */
            const char *vendor_sys = "-pc-linux-";
            if (!abi)
                abi = "gnu";
            bh_assert(strlen(arch) + strlen(vendor_sys) + strlen(abi) < sizeof(triple_buf));
            memcpy(triple_buf, arch, strlen(arch));
            memcpy(triple_buf + strlen(arch), vendor_sys, strlen(vendor_sys));
            memcpy(triple_buf + strlen(arch) + strlen(vendor_sys), abi, strlen(abi));
            triple = triple_buf;
        }

        if (!cpu && features) {
            aot_set_last_error("cpu isn't specified for cpu features.");
            goto fail;
        }

        if (!triple && !cpu) {
            /* Get a triple for the host machine */
            if (!(triple_norm = triple_norm_new = LLVMGetDefaultTargetTriple())) {
                aot_set_last_error("llvm get default target triple failed.");
                goto fail;
            }
            /* Get CPU name of the host machine */
            if (!(cpu = cpu_new = LLVMGetHostCPUName())) {
                aot_set_last_error("llvm get host cpu name failed.");
                goto fail;
            }
        }
        else if (triple) {
            /* Normalize a target triple */
            if (!(triple_norm = triple_norm_new = LLVMNormalizeTargetTriple(triple))) {
                snprintf(buf, sizeof(buf),
                         "llvm normlalize target triple (%s) failed.", triple);
                aot_set_last_error(buf);
                goto fail;
            }
            if (!cpu)
                cpu = "";
        }
        else { /* triple is NULL, cpu isn't NULL */
            snprintf(buf, sizeof(buf),
                    "target isn't specified for cpu %s.", cpu);
            aot_set_last_error(buf);
            goto fail;
        }

        if (!features)
            features = "";

        /* Get target with triple, note that LLVMGetTargetFromTriple()
           return 0 when success, but not true. */
        if (LLVMGetTargetFromTriple(triple_norm, &target, &err) != 0) {
            if (err) {
                LLVMDisposeMessage(err);
                err = NULL;
            }
            snprintf(buf, sizeof(buf),
                     "llvm get target from triple (%s) failed", triple_norm);
            aot_set_last_error(buf);
            goto fail;
        }

        /* Save target arch */
        get_target_arch_from_triple(triple_norm, comp_ctx->target_arch,
                                    sizeof(comp_ctx->target_arch));

        bh_printf("Create AoT compiler with:\n");
        bh_printf("  target:        %s\n", comp_ctx->target_arch);
        bh_printf("  target cpu:    %s\n", cpu);
        bh_printf("  cpu features:  %s\n", features);
        bh_printf("  opt level:     %d\n", opt_level);
        switch (option->output_format) {
            case AOT_LLVMIR_UNOPT_FILE:
                bh_printf("  output format: unoptimized LLVM IR\n");
                break;
            case AOT_LLVMIR_OPT_FILE:
                bh_printf("  output format: optimized LLVM IR\n");
                break;
            case AOT_FORMAT_FILE:
                bh_printf("  output format: AoT file\n");
                break;
            case AOT_OBJECT_FILE:
                bh_printf("  output format: native object file\n");
                break;
        }

        if (!LLVMTargetHasTargetMachine(target)) {
            snprintf(buf, sizeof(buf),
                     "no target machine for this target (%s).", triple_norm);
            aot_set_last_error(buf);
            goto fail;
        }

        if (!LLVMTargetHasAsmBackend(target)) {
            snprintf(buf, sizeof(buf),
                     "no asm backend for this target (%s).", LLVMGetTargetName(target));
            aot_set_last_error(buf);
            goto fail;
        }

        /* Create the target machine */
        if (!(comp_ctx->target_machine =
                    LLVMCreateTargetMachine(target, triple_norm, cpu, features,
                                            opt_level, LLVMRelocStatic,
                                            LLVMCodeModelSmall))) {
            aot_set_last_error("create LLVM target machine failed.");
            goto fail;
        }
    }

    comp_ctx->optimize = true;
    if (option->output_format == AOT_LLVMIR_UNOPT_FILE)
        comp_ctx->optimize = false;

    if (!(comp_ctx->pass_mgr = LLVMCreateFunctionPassManagerForModule
                (comp_ctx->module))) {
        aot_set_last_error("create LLVM pass manager failed.");
        goto fail;
    }
    LLVMAddPromoteMemoryToRegisterPass(comp_ctx->pass_mgr);
    LLVMAddInstructionCombiningPass(comp_ctx->pass_mgr);
    LLVMAddCFGSimplificationPass(comp_ctx->pass_mgr);
    LLVMAddJumpThreadingPass(comp_ctx->pass_mgr);
    LLVMAddConstantPropagationPass(comp_ctx->pass_mgr);

    /* Create metadata for llvm float experimental constrained intrinsics */
    if (!(comp_ctx->fp_rounding_mode =
                LLVMMDStringInContext(comp_ctx->context,
                                      fp_round,
                                      (uint32)strlen(fp_round)))
        || !(comp_ctx->fp_exception_behavior =
                LLVMMDStringInContext(comp_ctx->context,
                                      fp_exce,
                                      (uint32)strlen(fp_exce)))) {
        aot_set_last_error("create float llvm metadata failed.");
        goto fail;
    }

    if (!aot_set_llvm_basic_types(&comp_ctx->basic_types, comp_ctx->context)) {
        aot_set_last_error("create LLVM basic types failed.");
        goto fail;
    }

    if (!aot_create_llvm_consts(&comp_ctx->llvm_consts, comp_ctx)) {
        aot_set_last_error("create LLVM const values failed.");
        goto fail;
    }

    /* set exec_env data type to int8** */
    if (!(comp_ctx->exec_env_type = LLVMPointerType(INT8_PTR_TYPE, 0))) {
        aot_set_last_error("llvm get pointer type failed.");
        goto fail;
    }

    /* set aot_inst data type to int8* */
    comp_ctx->aot_inst_type = INT8_PTR_TYPE;

    /* Create function context for each function */
    comp_ctx->func_ctx_count = comp_data->func_count;
    if (!(comp_ctx->func_ctxes = aot_create_func_contexts(comp_data, comp_ctx)))
        goto fail;

    ret = comp_ctx;

fail:
    if (triple_norm_new)
        LLVMDisposeMessage(triple_norm_new);
    if (cpu_new)
        LLVMDisposeMessage(cpu_new);

    if (!ret)
        aot_destroy_comp_context(comp_ctx);
    return ret;
}

void
aot_destroy_comp_context(AOTCompContext *comp_ctx)
{
    if (!comp_ctx)
        return;

    if (comp_ctx->pass_mgr)
        LLVMDisposePassManager(comp_ctx->pass_mgr);

    if (comp_ctx->target_machine)
        LLVMDisposeTargetMachine(comp_ctx->target_machine);

    if (comp_ctx->builder)
        LLVMDisposeBuilder(comp_ctx->builder);

    if (comp_ctx->exec_engine) {
        LLVMDisposeExecutionEngine(comp_ctx->exec_engine);
        /* The LLVM module is freed when disposing execution engine,
           no need to dispose it again. */
    }
    else if (comp_ctx->module)
        LLVMDisposeModule(comp_ctx->module);

    if (comp_ctx->context)
        LLVMContextDispose(comp_ctx->context);

    if (comp_ctx->func_ctxes)
        aot_destroy_func_contexts(comp_ctx->func_ctxes, comp_ctx->func_ctx_count);

    wasm_free(comp_ctx);
}

void
aot_value_stack_push(AOTValueStack *stack, AOTValue *value)
{
    if (!stack->value_list_head)
        stack->value_list_head = stack->value_list_end = value;
    else {
        stack->value_list_end->next = value;
        value->prev = stack->value_list_end;
        stack->value_list_end = value;
    }
}

AOTValue *
aot_value_stack_pop(AOTValueStack *stack)
{
    AOTValue *value = stack->value_list_end;

    bh_assert(stack->value_list_end);

    if (stack->value_list_head == stack->value_list_end)
        stack->value_list_head = stack->value_list_end = NULL;
    else {
        stack->value_list_end = stack->value_list_end->prev;
        stack->value_list_end->next = NULL;
        value->prev = NULL;
    }

    return value;
}

void
aot_value_stack_destroy(AOTValueStack *stack)
{
    AOTValue *value = stack->value_list_head, *p;

    while (value) {
        p = value->next;
        wasm_free(value);
        value = p;
    }
}

void
aot_block_stack_push(AOTBlockStack *stack, AOTBlock *block)
{
    if (!stack->block_list_head)
        stack->block_list_head = stack->block_list_end = block;
    else {
        stack->block_list_end->next = block;
        block->prev = stack->block_list_end;
        stack->block_list_end = block;
    }
}

AOTBlock *
aot_block_stack_pop(AOTBlockStack *stack)
{
    AOTBlock *block = stack->block_list_end;

    bh_assert(stack->block_list_end);

    if (stack->block_list_head == stack->block_list_end)
        stack->block_list_head = stack->block_list_end = NULL;
    else {
        stack->block_list_end = stack->block_list_end->prev;
        stack->block_list_end->next = NULL;
        block->prev = NULL;
    }

    return block;
}

void
aot_block_stack_destroy(AOTBlockStack *stack)
{
    AOTBlock *block = stack->block_list_head, *p;

    while (block) {
        p = block->next;
        aot_value_stack_destroy(&block->value_stack);
        wasm_free(block);
        block = p;
    }
}

void
aot_block_destroy(AOTBlock *block)
{
    aot_value_stack_destroy(&block->value_stack);
    wasm_free(block);
}
