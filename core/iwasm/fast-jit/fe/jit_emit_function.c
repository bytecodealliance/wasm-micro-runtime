/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "jit_emit_function.h"
#include "jit_emit_exception.h"
#include "../jit_frontend.h"

extern bool
jit_invoke_native(WASMExecEnv *exec_env, uint32 func_idx,
                  WASMInterpFrame *prev_frame);

bool
jit_compile_op_call(JitCompContext *cc, uint32 func_idx, bool tail_call)
{
    WASMModule *wasm_module = cc->cur_wasm_module;
    WASMFunctionImport *func_import;
    WASMFunction *func;
    WASMType *func_type;
    JitFrame *jit_frame = cc->jit_frame;
    JitReg value, result = 0, native_ret;
    JitReg func_ptrs, jitted_code = 0;
    JitInsn *insn;
    uint32 i, n, outs_off, jitted_func_idx;

    if (func_idx >= wasm_module->import_function_count) {
        func_ptrs = get_func_ptrs_reg(jit_frame);
        jitted_code = jit_cc_new_reg_ptr(cc);
        /* jitted_code = func_ptrs[func_idx - import_function_count] */
        jitted_func_idx = func_idx - wasm_module->import_function_count;
        GEN_INSN(LDPTR, jitted_code, func_ptrs,
                 NEW_CONST(I32, (uint32)sizeof(void *) * jitted_func_idx));
    }

    if (func_idx < wasm_module->import_function_count) {
        func_import = &wasm_module->import_functions[func_idx].u.function;
        func_type = func_import->func_type;
    }
    else {
        func = wasm_module
                   ->functions[func_idx - wasm_module->import_function_count];
        func_type = func->func_type;
    }

    /* Prepare parameters for the function to call */
    outs_off =
        cc->total_frame_size + offsetof(WASMInterpFrame, lp)
        + wasm_get_cell_num(func_type->types, func_type->param_count) * 4;

    for (i = 0; i < func_type->param_count; i++) {
        switch (func_type->types[func_type->param_count - 1 - i]) {
            case VALUE_TYPE_I32:
#if WASM_ENABLE_REF_TYPES != 0
            case VALUE_TYPE_EXTERNREF:
            case VALUE_TYPE_FUNCREF:
#endif
                POP_I32(value);
                outs_off -= 4;
                GEN_INSN(STI32, value, cc->fp_reg, NEW_CONST(I32, outs_off));
                break;
            case VALUE_TYPE_I64:
                POP_I64(value);
                outs_off -= 8;
                GEN_INSN(STI64, value, cc->fp_reg, NEW_CONST(I32, outs_off));
                break;
            case VALUE_TYPE_F32:
                POP_F32(value);
                outs_off -= 4;
                GEN_INSN(STF32, value, cc->fp_reg, NEW_CONST(I32, outs_off));
                break;
            case VALUE_TYPE_F64:
                POP_F64(value);
                outs_off -= 8;
                GEN_INSN(STF64, value, cc->fp_reg, NEW_CONST(I32, outs_off));
                break;
            default:
                bh_assert(0);
                break;
        }
    }

    /* Commit sp as the callee may use it to store the results */
    gen_commit_sp_ip(cc->jit_frame);

    if (func_idx < wasm_module->import_function_count) {
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
        /* Set native_ret to x86::eax, 1 is hard reg index of eax */
        native_ret = jit_reg_new(JIT_REG_KIND_I32, 1);
#else
        native_ret = jit_cc_new_reg_I32(cc);
#endif
        insn = GEN_INSN(CALLNATIVE, native_ret,
                        NEW_CONST(PTR, (uintptr_t)jit_invoke_native), 3);
        if (insn) {
            *(jit_insn_opndv(insn, 2)) = cc->exec_env_reg;
            *(jit_insn_opndv(insn, 3)) = NEW_CONST(I32, func_idx);
            *(jit_insn_opndv(insn, 4)) = cc->fp_reg;
        }

        /* Check whether there is exception thrown */
        GEN_INSN(CMP, cc->cmp_reg, native_ret, NEW_CONST(I32, 0));
        if (!jit_emit_exception(cc, EXCE_ALREADY_THROWN, JIT_OP_BEQ,
                                cc->cmp_reg, 0)) {
            return false;
        }
    }
    else {
        if (func_type->result_count > 0) {
            switch (func_type->types[func_type->param_count]) {
                case VALUE_TYPE_I32:
#if WASM_ENABLE_REF_TYPES != 0
                case VALUE_TYPE_EXTERNREF:
                case VALUE_TYPE_FUNCREF:
#endif
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
                    /* Set result to x86::eax, 1 is hard reg index of eax */
                    result = jit_reg_new(JIT_REG_KIND_I32, 1);
#else
                    result = jit_cc_new_reg_I32(cc);
#endif
                    break;
                case VALUE_TYPE_I64:
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
                    /* Set result to x86::rax, 1 is hard reg index of rax */
                    result = jit_reg_new(JIT_REG_KIND_I64, 1);
#else
                    result = jit_cc_new_reg_I64(cc);
#endif
                    break;
                case VALUE_TYPE_F32:
                    result = jit_cc_new_reg_F32(cc);
                    break;
                case VALUE_TYPE_F64:
                    result = jit_cc_new_reg_F64(cc);
                    break;
                default:
                    bh_assert(0);
                    break;
            }
        }

        GEN_INSN(CALLBC, result, 0, jitted_code);
    }

    /* Push results */
    n = cc->jit_frame->sp - cc->jit_frame->lp;
    for (i = 0; i < func_type->result_count; i++) {
        switch (func_type->types[func_type->param_count + i]) {
            case VALUE_TYPE_I32:
#if WASM_ENABLE_REF_TYPES != 0
            case VALUE_TYPE_EXTERNREF:
            case VALUE_TYPE_FUNCREF:
#endif
                value = jit_cc_new_reg_I32(cc);
                GEN_INSN(LDI32, value, cc->fp_reg,
                         NEW_CONST(I32, offset_of_local(n)));
                PUSH_I32(value);
                n++;
                break;
            case VALUE_TYPE_I64:
                value = jit_cc_new_reg_I64(cc);
                GEN_INSN(LDI64, value, cc->fp_reg,
                         NEW_CONST(I32, offset_of_local(n)));
                PUSH_I64(value);
                n += 2;
                break;
            case VALUE_TYPE_F32:
                value = jit_cc_new_reg_F32(cc);
                GEN_INSN(LDF32, value, cc->fp_reg,
                         NEW_CONST(I32, offset_of_local(n)));
                PUSH_F32(value);
                n++;
                break;
            case VALUE_TYPE_F64:
                value = jit_cc_new_reg_F64(cc);
                GEN_INSN(LDF64, value, cc->fp_reg,
                         NEW_CONST(I32, offset_of_local(n)));
                PUSH_F64(value);
                n += 2;
                break;
            default:
                bh_assert(0);
                break;
        }
    }

    /* Clear part of memory regs and table regs as their values
       may be changed in the function call */
    clear_memory_regs(jit_frame);
    clear_table_regs(jit_frame);

    /* Ignore tail call currently */
    (void)tail_call;
    return true;
fail:
    return false;
}

bool
jit_compile_op_call_indirect(JitCompContext *cc, uint32 type_idx,
                             uint32 tbl_idx)
{
    return false;
}

bool
jit_compile_op_ref_null(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_op_ref_is_null(JitCompContext *cc)
{
    return false;
}

bool
jit_compile_op_ref_func(JitCompContext *cc, uint32 func_idx)
{
    return false;
}
