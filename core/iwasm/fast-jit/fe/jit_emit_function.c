/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "jit_emit_function.h"
#include "jit_emit_exception.h"
#include "../jit_frontend.h"
#include "../jit_codegen.h"
#include "../../interpreter/wasm_runtime.h"

extern bool
jit_invoke_native(WASMExecEnv *exec_env, uint32 func_idx,
                  WASMInterpFrame *prev_frame);

/* Prepare parameters for the function to call */
static bool
pre_call(JitCompContext *cc, const WASMType *func_type)
{
    JitReg value;
    uint32 i, outs_off;
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
                goto fail;
        }
    }

    /* Commit sp as the callee may use it to store the results */
    gen_commit_sp_ip(cc->jit_frame);

    return true;
fail:
    return false;
}

/* Push results */
static bool
post_return(JitCompContext *cc, const WASMType *func_type)
{
    uint32 i, n;
    JitReg value;

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
                goto fail;
        }
    }

    /* Update the committed_sp as the callee has updated the frame sp */
    cc->jit_frame->committed_sp = cc->jit_frame->sp;

    return true;
fail:
    return false;
}

bool
jit_compile_op_call(JitCompContext *cc, uint32 func_idx, bool tail_call)
{
    WASMModule *wasm_module = cc->cur_wasm_module;
    WASMFunctionImport *func_import;
    WASMFunction *func;
    WASMType *func_type;
    JitFrame *jit_frame = cc->jit_frame;
    JitReg result = 0, native_ret;
    JitReg func_ptrs, jitted_code = 0;
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
    JitReg eax_hreg = jit_codegen_get_hreg_by_name("eax");
    JitReg rax_hreg = jit_codegen_get_hreg_by_name("rax");
#endif
    JitInsn *insn;
    uint32 jitted_func_idx;

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

    if (!pre_call(cc, func_type)) {
        goto fail;
    }

    if (func_idx < wasm_module->import_function_count) {
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
        /* Set native_ret to x86::eax */
        native_ret = eax_hreg;
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

#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
        jit_lock_reg_in_insn(cc, insn, native_ret);
#endif

        /* Check whether there is exception thrown */
        GEN_INSN(CMP, cc->cmp_reg, native_ret, NEW_CONST(I32, 0));
        if (!jit_emit_exception(cc, EXCE_ALREADY_THROWN, JIT_OP_BEQ,
                                cc->cmp_reg, NULL)) {
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
                    result = eax_hreg;
#else
                    result = jit_cc_new_reg_I32(cc);
#endif
                    break;
                case VALUE_TYPE_I64:
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
                    result = rax_hreg;
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

    if (!post_return(cc, func_type)) {
        goto fail;
    }

    /* Clear part of memory regs and table regs as their values
       may be changed in the function call */
    if (cc->cur_wasm_module->possible_memory_grow)
        clear_memory_regs(jit_frame);
    clear_table_regs(jit_frame);

    /* Ignore tail call currently */
    (void)tail_call;
    return true;
fail:
    return false;
}

static JitReg
pack_argv(JitCompContext *cc)
{
    /* reuse the stack of the next frame*/
    uint32 stack_base;
    JitReg argv;

    stack_base = cc->total_frame_size + offsetof(WASMInterpFrame, lp);
    argv = jit_cc_new_reg_ptr(cc);
    GEN_INSN(ADD, argv, cc->fp_reg, NEW_CONST(PTR, stack_base));
    return argv;
}

static bool
unpack_argv(JitCompContext *cc, const WASMType *func_type, JitReg argv)
{
    /* argv to stack*/
    uint32 i, top_by_cell, offset_by_cell;
    JitReg value;

    /* stack top */
    top_by_cell = cc->jit_frame->sp - cc->jit_frame->lp;
    offset_by_cell = 0;
    for (i = 0; i < func_type->result_count; i++) {
        switch (func_type->types[func_type->param_count + i]) {
            case VALUE_TYPE_I32:
#if WASM_ENABLE_REF_TYPES != 0
            case VALUE_TYPE_EXTERNREF:
            case VALUE_TYPE_FUNCREF:
#endif
            {
                value = jit_cc_new_reg_I32(cc);
                GEN_INSN(LDI32, value, argv, NEW_CONST(I32, offset_by_cell));
                GEN_INSN(STI32, value, cc->fp_reg,
                         NEW_CONST(I32, offset_of_local(top_by_cell
                                                        + offset_by_cell)));
                offset_by_cell += 1;
                break;
            }
            case VALUE_TYPE_I64:
            {
                value = jit_cc_new_reg_I64(cc);
                GEN_INSN(LDI64, value, argv, NEW_CONST(I32, offset_by_cell));
                GEN_INSN(STI64, value, cc->fp_reg,
                         NEW_CONST(I32, offset_of_local(top_by_cell
                                                        + offset_by_cell)));
                offset_by_cell += 2;
                break;
            }
            case VALUE_TYPE_F32:
            {
                value = jit_cc_new_reg_F32(cc);
                GEN_INSN(LDF32, value, argv, NEW_CONST(I32, offset_by_cell));
                GEN_INSN(STF32, value, cc->fp_reg,
                         NEW_CONST(I32, offset_of_local(top_by_cell
                                                        + offset_by_cell)));
                offset_by_cell += 1;
                break;
            }
            case VALUE_TYPE_F64:
            {
                value = jit_cc_new_reg_F64(cc);
                GEN_INSN(LDF64, value, argv, NEW_CONST(I32, offset_by_cell));
                GEN_INSN(STF64, value, cc->fp_reg,
                         NEW_CONST(I32, offset_of_local(top_by_cell
                                                        + offset_by_cell)));
                offset_by_cell += 2;
                break;
            }
            default:
            {
                bh_assert(0);
                goto fail;
            }
        }
    }

    return true;
fail:
    return false;
}

bool
jit_compile_op_call_indirect(JitCompContext *cc, uint32 type_idx,
                             uint32 tbl_idx)
{
    JitReg element_indices, native_ret, argv;
    WASMType *func_type;
    JitInsn *insn;

    POP_I32(element_indices);

    func_type = cc->cur_wasm_module->types[type_idx];
    if (!pre_call(cc, func_type)) {
        goto fail;
    }

    argv = pack_argv(cc);

#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64)
    /* Set native_ret to x86::eax */
    native_ret = jit_codegen_get_hreg_by_name("eax");
#else
    native_ret = jit_cc_new_reg_I32(cc);
#endif

    insn = GEN_INSN(CALLNATIVE, native_ret,
                    NEW_CONST(PTR, (uintptr_t)wasm_call_indirect), 5);
    if (!insn) {
        goto fail;
    }

    *(jit_insn_opndv(insn, 2)) = cc->exec_env_reg;
    *(jit_insn_opndv(insn, 3)) = NEW_CONST(I32, tbl_idx);
    *(jit_insn_opndv(insn, 4)) = element_indices;
    *(jit_insn_opndv(insn, 5)) = NEW_CONST(I32, func_type->param_count);
    *(jit_insn_opndv(insn, 6)) = argv;

    jit_lock_reg_in_insn(cc, insn, native_ret);

    /* Check whether there is exception thrown */
    GEN_INSN(CMP, cc->cmp_reg, native_ret, NEW_CONST(I32, 0));
    if (!jit_emit_exception(cc, EXCE_ALREADY_THROWN, JIT_OP_BEQ, cc->cmp_reg,
                            NULL)) {
        return false;
    }

    if (!unpack_argv(cc, func_type, argv)) {
        goto fail;
    }

    if (!post_return(cc, func_type)) {
        goto fail;
    }

    /* Clear part of memory regs and table regs as their values
       may be changed in the function call */
    if (cc->cur_wasm_module->possible_memory_grow)
        clear_memory_regs(cc->jit_frame);
    clear_table_regs(cc->jit_frame);
    return true;
fail:
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
