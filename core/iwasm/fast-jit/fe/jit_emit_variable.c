/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "jit_emit_variable.h"
#include "../jit_frontend.h"

#define CHECK_LOCAL(idx)                                                     \
    do {                                                                     \
        if (idx                                                              \
            >= wasm_func->func_type->param_count + wasm_func->local_count) { \
            jit_set_last_error(cc, "local index out of range");              \
            goto fail;                                                       \
        }                                                                    \
    } while (0)

static uint8
get_local_type(const WASMFunction *wasm_func, uint32 local_idx)
{
    uint32 param_count = wasm_func->func_type->param_count;
    return local_idx < param_count
               ? wasm_func->func_type->types[local_idx]
               : wasm_func->local_types[local_idx - param_count];
}

bool
jit_compile_op_get_local(JitCompContext *cc, uint32 local_idx)
{
    WASMFunction *wasm_func = cc->cur_wasm_func;
    uint16 *local_offsets = wasm_func->local_offsets;
    uint16 local_offset;
    uint8 local_type;
    JitReg value = 0;

    CHECK_LOCAL(local_idx);

    local_offset = local_offsets[local_idx];
    local_type = get_local_type(wasm_func, local_idx);

    switch (local_type) {
        case VALUE_TYPE_I32:
#if WASM_ENABLE_REF_TYPES != 0
        case VALUE_TYPE_EXTERNREF:
        case VALUE_TYPE_FUNCREF:
#endif
            value = local_i32(cc->jit_frame, local_offset);

            break;
        case VALUE_TYPE_I64:
            value = local_i64(cc->jit_frame, local_offset);
            break;
        case VALUE_TYPE_F32:
            value = local_f32(cc->jit_frame, local_offset);
            break;
        case VALUE_TYPE_F64:
            value = local_f64(cc->jit_frame, local_offset);
            break;
        default:
            bh_assert(0);
            break;
    }

    PUSH(value, local_type);
    return true;
fail:
    return false;
}

bool
jit_compile_op_set_local(JitCompContext *cc, uint32 local_idx)
{
    WASMFunction *wasm_func = cc->cur_wasm_func;
    uint16 *local_offsets = wasm_func->local_offsets;
    uint16 local_offset;
    uint8 local_type;
    JitReg value;

    CHECK_LOCAL(local_idx);

    local_offset = local_offsets[local_idx];
    local_type = get_local_type(wasm_func, local_idx);

    switch (local_type) {
        case VALUE_TYPE_I32:
#if WASM_ENABLE_REF_TYPES != 0
        case VALUE_TYPE_EXTERNREF:
        case VALUE_TYPE_FUNCREF:
#endif
            POP_I32(value);
            set_local_i32(cc->jit_frame, local_offset, value);
            break;
        case VALUE_TYPE_I64:
            POP_I64(value);
            set_local_i64(cc->jit_frame, local_offset, value);
            break;
        case VALUE_TYPE_F32:
            POP_F32(value);
            set_local_f32(cc->jit_frame, local_offset, value);
            break;
        case VALUE_TYPE_F64:
            POP_F64(value);
            set_local_f64(cc->jit_frame, local_offset, value);
            break;
        default:
            bh_assert(0);
            break;
    }

    return true;
fail:
    return false;
}

bool
jit_compile_op_tee_local(JitCompContext *cc, uint32 local_idx)
{
    WASMFunction *wasm_func = cc->cur_wasm_func;
    uint16 *local_offsets = wasm_func->local_offsets;
    uint16 local_offset;
    uint8 local_type;
    JitReg value = 0;

    CHECK_LOCAL(local_idx);

    local_offset = local_offsets[local_idx];
    local_type = get_local_type(wasm_func, local_idx);

    switch (local_type) {
        case VALUE_TYPE_I32:
#if WASM_ENABLE_REF_TYPES != 0
        case VALUE_TYPE_EXTERNREF:
        case VALUE_TYPE_FUNCREF:
#endif
            POP_I32(value);
            set_local_i32(cc->jit_frame, local_offset, value);
            PUSH_I32(value);
            break;
        case VALUE_TYPE_I64:
            POP_I64(value);
            set_local_i64(cc->jit_frame, local_offset, value);
            PUSH_I64(value);
            break;
        case VALUE_TYPE_F32:
            POP_F32(value);
            set_local_f32(cc->jit_frame, local_offset, value);
            PUSH_F32(value);
            break;
        case VALUE_TYPE_F64:
            POP_F64(value);
            set_local_f64(cc->jit_frame, local_offset, value);
            PUSH_F64(value);
            break;
        default:
            bh_assert(0);
            break;
    }

    return true;
fail:
    return false;
}

static uint8
get_global_type(const WASMModule *module, uint32 global_idx)
{
    return module->globals[global_idx].type;
}

static JitReg
get_global_data(JitCompContext *cc)
{
    JitReg module_inst = jit_cc_new_reg_I64(cc);
    JitReg global_data = jit_cc_new_reg_I64(cc);

    /* module_inst = exec_env->module_inst */
    GEN_INSN(LDI64, module_inst, cc->exec_env_reg,
             NEW_CONST(I32, offsetof(WASMExecEnv, module_inst)));
    /* global_data = module_inst->global_data*/
    GEN_INSN(LDI64, global_data, module_inst,
             NEW_CONST(I32, offsetof(WASMModuleInstance, global_data)));
    return global_data;
}

static JitReg
get_data_offset(JitCompContext *cc, uint32 global_idx)
{
    JitReg module_inst = jit_cc_new_reg_I64(cc);
    JitReg globals = jit_cc_new_reg_I64(cc);
    JitReg global_inst = jit_cc_new_reg_I64(cc);
    JitReg data_offset = jit_cc_new_reg_I64(cc);

    /* module_inst = exec_env->module_inst */
    GEN_INSN(LDI64, module_inst, cc->exec_env_reg,
             NEW_CONST(I32, offsetof(WASMExecEnv, module_inst)));
    /* globals = module_inst->globals */
    GEN_INSN(LDI64, globals, module_inst,
             NEW_CONST(I32, offsetof(WASMModuleInstance, globals)));
    /* global_inst = globals + global_idx */
    GEN_INSN(ADD, global_inst, globals,
             NEW_CONST(I64, global_idx * sizeof(WASMGlobalInstance)));
    /* data_offset = global_inst->data_offset */
    GEN_INSN(LDI32, data_offset, global_inst,
             NEW_CONST(I32, offsetof(WASMGlobalInstance, data_offset)));
    return data_offset;
}

bool
jit_compile_op_get_global(JitCompContext *cc, uint32 global_idx)
{
    uint8 global_type = 0;
    JitReg value = 0;

    if (global_idx >= cc->cur_wasm_module->global_count) {
        jit_set_last_error(cc, "the global index is out of range");
        goto fail;
    }

    global_type = get_global_type(cc->cur_wasm_module, global_idx);
    switch (global_type) {
        case VALUE_TYPE_I32:
#if WASM_ENABLE_REF_TYPES != 0
        case VALUE_TYPE_EXTERNREF:
        case VALUE_TYPE_FUNCREF:
#endif
        {
            value = jit_cc_new_reg_I32(cc);
            GEN_INSN(LDI32, value, get_global_data(cc),
                     get_data_offset(cc, global_idx));
            break;
        }
        case VALUE_TYPE_I64:
        {
            value = jit_cc_new_reg_I64(cc);
            GEN_INSN(LDI64, value, get_global_data(cc),
                     get_data_offset(cc, global_idx));
            break;
        }
        case VALUE_TYPE_F32:
        {
            value = jit_cc_new_reg_F32(cc);
            GEN_INSN(LDF32, value, get_global_data(cc),
                     get_data_offset(cc, global_idx));
            break;
        }
        case VALUE_TYPE_F64:
        {
            value = jit_cc_new_reg_F64(cc);
            GEN_INSN(LDF64, value, get_global_data(cc),
                     get_data_offset(cc, global_idx));
            break;
        }
        default:
        {
            jit_set_last_error(cc, "unexpected global type");
            goto fail;
        }
    }

    PUSH(value, global_type);

    return true;
fail:
    return false;
}

bool
jit_compile_op_set_global(JitCompContext *cc, uint32 global_idx,
                          bool is_aux_stack)
{
    uint8 global_type = 0;
    JitReg value = 0;

    if (is_aux_stack) {
        jit_set_last_error(cc, "doesn't support set global_aux_stack");
        goto fail;
    }

    if (global_idx >= cc->cur_wasm_module->global_count) {
        jit_set_last_error(cc, "the global index is out of range");
        goto fail;
    }

    global_type = get_global_type(cc->cur_wasm_module, global_idx);
    switch (global_type) {
        case VALUE_TYPE_I32:
#if WASM_ENABLE_REF_TYPES != 0
        case VALUE_TYPE_EXTERNREF:
        case VALUE_TYPE_FUNCREF:
#endif
        {
            POP_I32(value);
            GEN_INSN(STI32, value, get_global_data(cc),
                     get_data_offset(cc, global_idx));
            break;
        }
        case VALUE_TYPE_I64:
        {
            POP_I64(value);
            GEN_INSN(STI64, value, get_global_data(cc),
                     get_data_offset(cc, global_idx));
            break;
        }
        case VALUE_TYPE_F32:
        {
            POP_F32(value);
            GEN_INSN(STI32, value, get_global_data(cc),
                     get_data_offset(cc, global_idx));
            break;
        }
        case VALUE_TYPE_F64:
        {
            POP_F64(value);
            GEN_INSN(STF64, value, get_global_data(cc),
                     get_data_offset(cc, global_idx));
            break;
        }
        default:
        {
            jit_set_last_error(cc, "unexpected global type");
            goto fail;
        }
    }

    return true;
fail:
    return false;
}
