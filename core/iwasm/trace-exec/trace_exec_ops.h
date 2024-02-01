/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _TRACE_EXEC_OPS_H_
#define _TRACE_EXEC_OPS_H_

#include "trace_exec.h"
#include "wasm_opcode.h"

struct trace_exec_op_info {
    const char *opcode_name;
    enum trace_exec_opcode_kind kind;
};

static const struct trace_exec_op_info simd_info[0xff + 1] = {
    [SIMD_v128_load] = { "v128.load", IMM_memarg_OP_i32 },
    [SIMD_v128_load16x4_s] = { "v128_load16x4_s", IMM_memarg_OP_i32 },
    [SIMD_v128_store] = { "v128.store", IMM_memarg_OP_i32_v128 },
    [SIMD_v128_const] = { "v128.const", IMM_v128_OP_0 },
    [SIMD_i8x16_extract_lane_s] = { "i8x16_extract_lane_s", IMM_i8_OP_v128 },
    [SIMD_i8x16_replace_lane] = { "i8x16.replace_lane", IMM_i8_OP_v128_i32 },
    [SIMD_v128_load32_zero] = { "v128.load32_zero", IMM_memarg_OP_i32 },
    [SIMD_f32x4_gt] = { "f32x4.gt", IMM_0_OP_v128_v128 },
    [SIMD_f64x2_trunc] = { "f64x2.trunc", IMM_0_OP_v128 },
    [SIMD_i16x8_sub_sat_s] = { "i16x8.sub_sat_s", IMM_0_OP_v128_v128 },
    [SIMD_i16x8_sub_sat_u] = { "i16x8.sub_sat_u", IMM_0_OP_v128_v128 },
    [SIMD_i16x8_max_u] = { "i16x8.max_u", IMM_0_OP_v128_v128 },
    [SIMD_i32x4_max_s] = { "i32x4.max_s", IMM_0_OP_v128_v128 },
    [SIMD_i64x2_sub] = { "i64x2.sub", IMM_0_OP_v128_v128 },
    [SIMD_f32x4_add] = { "f32x4.add", IMM_0_OP_v128_v128 },
    [SIMD_f32x4_abs] = { "f32x4.abs", IMM_0_OP_v128 },
    [SIMD_f32x4_min] = { "f32x4.min", IMM_0_OP_v128_v128 },
    [SIMD_f32x4_max] = { "f32x4.max", IMM_0_OP_v128_v128 },
    [SIMD_f64x2_min] = { "f64x2.min", IMM_0_OP_v128_v128 },
    [SIMD_f64x2_max] = { "f64x2.max", IMM_0_OP_v128_v128 },
    [SIMD_f64x2_pmin] = { "f64x2.pmin", IMM_0_OP_v128_v128 },
};

static const struct trace_exec_op_info opcode_info[0xff + 1] = {
    [WASM_OP_IF] = { "if", IMM_0_OP_i32 },
    [WASM_OP_ELSE] = { "else", IMM_0_OP_0 },
    [WASM_OP_END] = { "end", IMM_0_OP_0 },
    [WASM_OP_BR] = { "br", IMM_i32_OP_0 },
    [WASM_OP_BR_IF] = { "br_if", IMM_i32_OP_i32 },
    [WASM_OP_CALL] = { "call", IMM_i32_OP_0 },
    [WASM_OP_CALL_INDIRECT] = { "call_indirect", IMM_ty_tbl_OP_i32 },
    [WASM_OP_DROP] = { "drop", IMM_0_OP_0 },
    [WASM_OP_GET_LOCAL] = { "local.get", IMM_i32_OP_0 },
    [WASM_OP_TEE_LOCAL] = { "local.tee", IMM_i32_OP_0 },
    [WASM_OP_I64_LOAD16_S] = { "i64.load16_s", IMM_memarg_OP_i32 },
    [WASM_OP_F32_CONST] = { "f32.const", IMM_f32_OP_0 },
    [WASM_OP_I32_EQZ] = { "i32.eqz", IMM_0_OP_i32 },
    [WASM_OP_I32_ADD] = { "i32.add", IMM_0_OP_i32_i32 },
    [WASM_OP_I32_AND] = { "i32.and", IMM_0_OP_i32_i32 },
    [WASM_OP_I32_OR] = { "i32.or", IMM_0_OP_i32_i32 },
    [WASM_OP_I32_ROTL] = { "i32.rotl", IMM_0_OP_i32_i32 },
    [WASM_OP_I32_TRUNC_S_F32] = { "i32.trunc_f32_s", IMM_0_OP_f32 },
    [WASM_OP_I32_TRUNC_U_F32] = { "i32.trunc_f32_u", IMM_0_OP_f32 },
    [WASM_OP_I32_TRUNC_S_F64] = { "i32.trunc_f64_s", IMM_0_OP_f64 },
    [WASM_OP_I32_TRUNC_U_F64] = { "i32.trunc_f64_u", IMM_0_OP_f64 },
};

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
} /* end of extern "C" */
#endif
#endif /* _TRACE_EXEC_OPS_H_ */