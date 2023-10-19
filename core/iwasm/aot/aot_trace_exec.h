/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _AOT_TRACE_EXEC_H
#define _AOT_TRACE_EXEC_H

#include "bh_platform.h"
#include "platform_common.h"
#if WASM_ENABLE_JIT != 0 || WASM_ENABLE_WAMR_COMPILER != 0
#include "../compilation/aot_llvm.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum trace_exec_value_kind {
    TRACE_V_I8 = 0,
    TRACE_V_I32,
    TRACE_V_I64,
    TRACE_V_F32,
    TRACE_V_F64,
    TRACE_V_V128,
};

struct trace_exec_value {
    enum trace_exec_value_kind kind;
    /*
     * it is a workaround for an observation that
     * `LLVMBuildAlloca(struct trace_exec_value[2])` result will have
     * paddings between the two elements, which will cause the
     * mis-understanding of the offset of the second
     * element.
     *
     * __attribute__((packed)) will lead a potential risk about unaligned
     * pointer.
     *
     * TODO: is there a way to apply LLVMSetAlignment()? the pain point is
     * the result of LLVMBuildAlloca() is a pointer
     */
    uint32 padding1;
    uint32 padding2;
    uint32 padding3;
    union {
        int8 i8;
        int32 i32;
        int64 i64;
        float32 f32;
        float64 f64;
        uint64 v128[2];
    } of;
};

enum trace_exec_opcode_kind {
    IMM_0_OP_0,
    IMM_0_OP_i32,
    IMM_0_OP_f32,
    IMM_0_OP_f64,
    IMM_0_OP_v128,
    IMM_0_OP_i32_i32,
    IMM_0_OP_v128_v128,
    IMM_i32_OP_0,
    IMM_v128_OP_0,
    IMM_i32_OP_i32,
    IMM_i8_OP_v128_i32,
    IMM_memarg_OP_i32,      // XX.load
    IMM_memarg_OP_i32_v128, // XX.store
};

struct trace_exec_instruction {
    uint8 opcode;
    uint8 ext_opcode;
    enum trace_exec_opcode_kind kind;
    struct trace_exec_value *imms;
    struct trace_exec_value *opds;
};

#if WASM_ENABLE_JIT != 0 || WASM_ENABLE_WAMR_COMPILER != 0
/* ============================== compilation ============================== */
bool
aot_trace_exec_build_call_helper(AOTCompContext *comp_ctx,
                                 AOTFuncContext *func_ctx, uint32 func_idx,
                                 uint8 opcode, uint8 ext_opcode, uint8 *ip);
#endif

#if WASM_ENABLE_JIT != 0 || WASM_ENABLE_AOT != 0
/* ============================== execution ============================== */
void
aot_trace_exec_helper(uint32 func_idx, uint64 offset, const char *opcode_name,
                      struct trace_exec_instruction *instr);
#endif
#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* _AOT_LLVM_H_ */