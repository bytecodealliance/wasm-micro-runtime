/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_compiler.h"
#include "aot_emit_compare.h"
#include "aot_emit_conversion.h"
#include "aot_emit_memory.h"
#include "aot_emit_variable.h"
#include "aot_emit_const.h"
#include "aot_emit_exception.h"
#include "aot_emit_numberic.h"
#include "aot_emit_control.h"
#include "aot_emit_function.h"
#include "aot_emit_parametric.h"
#include "../aot/aot_runtime.h"
#include "../interpreter/wasm_opcode.h"
#include <errno.h>


#define CHECK_BUF(buf, buf_end, length) do {                \
  if (buf + length > buf_end) {                             \
    aot_set_last_error("read leb failed: unexpected end."); \
    return false;                                           \
  }                                                         \
} while (0)

static bool
read_leb(const uint8 *buf, const uint8 *buf_end,
         uint32 *p_offset, uint32 maxbits,
         bool sign, uint64 *p_result)
{
  uint64 result = 0;
  uint32 shift = 0;
  uint32 bcnt = 0;
  uint64 byte;

  while (true) {
    CHECK_BUF(buf, buf_end, 1);
    byte = buf[*p_offset];
    *p_offset += 1;
    result |= ((byte & 0x7f) << shift);
    shift += 7;
    if ((byte & 0x80) == 0) {
      break;
    }
    bcnt += 1;
  }
  if (bcnt > (((maxbits + 8) >> 3) - (maxbits + 8))) {
    aot_set_last_error("read leb failed: unsigned leb overflow.");
    return false;
  }
  if (sign && (shift < maxbits) && (byte & 0x40)) {
    /* Sign extend */
    result |= (uint64)(- (((uint64)1) << shift));
  }
  *p_result = result;
  return true;
}

#define read_leb_uint32(p, p_end, res) do {         \
  uint32 off = 0;                                   \
  uint64 res64;                                     \
  if (!read_leb(p, p_end, &off, 32, false, &res64)) \
    return false;                                   \
  p += off;                                         \
  res = (uint32)res64;                              \
} while (0)

#define read_leb_int32(p, p_end, res) do {          \
  uint32 off = 0;                                   \
  uint64 res64;                                     \
  if (!read_leb(p, p_end, &off, 32, true, &res64))  \
    return false;                                   \
  p += off;                                         \
  res = (int32)res64;                              \
} while (0)

#define read_leb_int64(p, p_end, res) do {          \
  uint32 off = 0;                                   \
  uint64 res64;                                     \
  if (!read_leb(p, p_end, &off, 64, true, &res64))  \
    return false;                                   \
  p += off;                                         \
  res = (int64)res64;                               \
} while (0)

#define COMPILE_ATOMIC_RMW(OP, NAME)                            \
  case WASM_OP_ATOMIC_RMW_I32_##NAME:                           \
    bytes = 4;                                                  \
    op_type = VALUE_TYPE_I32;                                   \
    goto OP_ATOMIC_##OP;                                        \
  case WASM_OP_ATOMIC_RMW_I64_##NAME:                           \
    bytes = 8;                                                  \
    op_type = VALUE_TYPE_I64;                                   \
    goto OP_ATOMIC_##OP;                                        \
  case WASM_OP_ATOMIC_RMW_I32_##NAME##8_U:                      \
    bytes = 1;                                                  \
    op_type = VALUE_TYPE_I32;                                   \
    goto OP_ATOMIC_##OP;                                        \
  case WASM_OP_ATOMIC_RMW_I32_##NAME##16_U:                     \
    bytes = 2;                                                  \
    op_type = VALUE_TYPE_I32;                                   \
    goto OP_ATOMIC_##OP;                                        \
  case WASM_OP_ATOMIC_RMW_I64_##NAME##8_U:                      \
    bytes = 1;                                                  \
    op_type = VALUE_TYPE_I64;                                   \
    goto OP_ATOMIC_##OP;                                        \
  case WASM_OP_ATOMIC_RMW_I64_##NAME##16_U:                     \
    bytes = 2;                                                  \
    op_type = VALUE_TYPE_I64;                                   \
    goto OP_ATOMIC_##OP;                                        \
  case WASM_OP_ATOMIC_RMW_I64_##NAME##32_U:                     \
    bytes = 4;                                                  \
    op_type = VALUE_TYPE_I64;                                   \
OP_ATOMIC_##OP:                                                 \
    bin_op = LLVMAtomicRMWBinOp##OP;                            \
    goto build_atomic_rmw;

static bool
aot_compile_func(AOTCompContext *comp_ctx, uint32 func_index)
{
  AOTFuncContext *func_ctx = comp_ctx->func_ctxes[func_index];
  uint8 *frame_ip = func_ctx->aot_func->code, opcode, *p_f32, *p_f64;
  uint8 *frame_ip_end = frame_ip + func_ctx->aot_func->code_size;
  uint8 *param_types = NULL;
  uint8 *result_types = NULL;
  uint8 value_type;
  uint16 param_count;
  uint16 result_count;
  uint32 br_depth, *br_depths, br_count;
  uint32 func_idx, type_idx, mem_idx, local_idx, global_idx, i;
  uint32 bytes = 4, align, offset;
  uint32 type_index;
  bool sign = true;
  int32 i32_const;
  int64 i64_const;
  float32 f32_const;
  float64 f64_const;
  AOTFuncType *func_type = NULL;

  /* Start to translate the opcodes */
  LLVMPositionBuilderAtEnd(comp_ctx->builder,
                           func_ctx->block_stack.block_list_head
                                   ->llvm_entry_block);
  while (frame_ip < frame_ip_end) {
    opcode = *frame_ip++;
    switch (opcode) {
      case WASM_OP_UNREACHABLE:
        if (!aot_compile_op_unreachable(comp_ctx, func_ctx, &frame_ip))
          return false;
        break;

      case WASM_OP_NOP:
        break;

      case WASM_OP_BLOCK:
      case WASM_OP_LOOP:
      case WASM_OP_IF:
        value_type = *frame_ip++;
        if (value_type == VALUE_TYPE_I32
            || value_type == VALUE_TYPE_I64
            || value_type == VALUE_TYPE_F32
            || value_type == VALUE_TYPE_F64
            || value_type == VALUE_TYPE_VOID) {
          param_count = 0;
          param_types = NULL;
          if (value_type == VALUE_TYPE_VOID) {
            result_count = 0;
            result_types = NULL;
          }
          else {
            result_count = 1;
            result_types = &value_type;
          }
        }
        else {
          frame_ip--;
          read_leb_uint32(frame_ip, frame_ip_end, type_index);
          func_type = comp_ctx->comp_data->func_types[type_index];
          param_count = func_type->param_count;
          param_types = func_type->types;
          result_count = func_type->result_count;
          result_types = func_type->types + param_count;
        }
        if (!aot_compile_op_block(comp_ctx, func_ctx,
                                  &frame_ip, frame_ip_end,
                                  (uint32)(LABEL_TYPE_BLOCK + opcode - WASM_OP_BLOCK),
                                  param_count, param_types,
                                  result_count, result_types))
          return false;
        break;

      case WASM_OP_ELSE:
        if (!aot_compile_op_else(comp_ctx, func_ctx, &frame_ip))
          return false;
        break;

      case WASM_OP_END:
        if (!aot_compile_op_end(comp_ctx, func_ctx, &frame_ip))
          return false;
        break;

      case WASM_OP_BR:
        read_leb_uint32(frame_ip, frame_ip_end, br_depth);
        if (!aot_compile_op_br(comp_ctx, func_ctx, br_depth, &frame_ip))
          return false;
        break;

      case WASM_OP_BR_IF:
        read_leb_uint32(frame_ip, frame_ip_end, br_depth);
        if (!aot_compile_op_br_if(comp_ctx, func_ctx, br_depth, &frame_ip))
          return false;
        break;

      case WASM_OP_BR_TABLE:
        read_leb_uint32(frame_ip, frame_ip_end, br_count);
        if (!(br_depths =
                wasm_runtime_malloc((uint32)sizeof(uint32) * (br_count + 1)))) {
          aot_set_last_error("allocate memory failed.");
          goto fail;
        }
        for (i = 0; i <= br_count; i++)
          read_leb_uint32(frame_ip, frame_ip_end, br_depths[i]);

        if (!aot_compile_op_br_table(comp_ctx, func_ctx,
                                     br_depths, br_count, &frame_ip)) {
          wasm_runtime_free(br_depths);
          return false;
        }

        wasm_runtime_free(br_depths);
        break;

      case WASM_OP_RETURN:
        if (!aot_compile_op_return(comp_ctx, func_ctx, &frame_ip))
          return false;
        break;

      case WASM_OP_CALL:
        read_leb_uint32(frame_ip, frame_ip_end, func_idx);
        if (!aot_compile_op_call(comp_ctx, func_ctx, func_idx, &frame_ip))
          return false;
        break;

      case WASM_OP_CALL_INDIRECT:
        read_leb_uint32(frame_ip, frame_ip_end, type_idx);
        frame_ip++; /* skip 0x00 */
        if (!aot_compile_op_call_indirect(comp_ctx, func_ctx, type_idx))
          return false;
        break;

      case WASM_OP_DROP:
        if (!aot_compile_op_drop(comp_ctx, func_ctx, true))
            return false;
        break;

      case WASM_OP_DROP_64:
        if (!aot_compile_op_drop(comp_ctx, func_ctx, false))
            return false;
        break;

      case WASM_OP_SELECT:
        if (!aot_compile_op_select(comp_ctx, func_ctx, true))
          return false;
        break;

      case WASM_OP_SELECT_64:
        if (!aot_compile_op_select(comp_ctx, func_ctx, false))
          return false;
        break;

      case WASM_OP_GET_LOCAL:
        read_leb_uint32(frame_ip, frame_ip_end, local_idx);
        if (!aot_compile_op_get_local(comp_ctx, func_ctx, local_idx))
          return false;
        break;

      case WASM_OP_SET_LOCAL:
        read_leb_uint32(frame_ip, frame_ip_end, local_idx);
        if (!aot_compile_op_set_local(comp_ctx, func_ctx, local_idx))
          return false;
        break;

      case WASM_OP_TEE_LOCAL:
        read_leb_uint32(frame_ip, frame_ip_end, local_idx);
        if (!aot_compile_op_tee_local(comp_ctx, func_ctx, local_idx))
          return false;
        break;

      case WASM_OP_GET_GLOBAL:
        read_leb_uint32(frame_ip, frame_ip_end, global_idx);
        if (!aot_compile_op_get_global(comp_ctx, func_ctx, global_idx))
          return false;
        break;

      case WASM_OP_SET_GLOBAL:
        read_leb_uint32(frame_ip, frame_ip_end, global_idx);
        if (!aot_compile_op_set_global(comp_ctx, func_ctx, global_idx))
          return false;
        break;

      case WASM_OP_I32_LOAD:
        bytes = 4;
        sign = true;
        goto op_i32_load;
      case WASM_OP_I32_LOAD8_S:
      case WASM_OP_I32_LOAD8_U:
        bytes = 1;
        sign = (opcode == WASM_OP_I32_LOAD8_S) ? true : false;
        goto op_i32_load;
      case WASM_OP_I32_LOAD16_S:
      case WASM_OP_I32_LOAD16_U:
        bytes = 2;
        sign = (opcode == WASM_OP_I32_LOAD16_S) ? true : false;
    op_i32_load:
        read_leb_uint32(frame_ip, frame_ip_end, align);
        read_leb_uint32(frame_ip, frame_ip_end, offset);
        if (!aot_compile_op_i32_load(comp_ctx, func_ctx, align, offset,
                                     bytes, sign, false))
          return false;
        break;

      case WASM_OP_I64_LOAD:
        bytes = 8;
        sign = true;
        goto op_i64_load;
      case WASM_OP_I64_LOAD8_S:
      case WASM_OP_I64_LOAD8_U:
        bytes = 1;
        sign = (opcode == WASM_OP_I64_LOAD8_S) ? true : false;
        goto op_i64_load;
      case WASM_OP_I64_LOAD16_S:
      case WASM_OP_I64_LOAD16_U:
        bytes = 2;
        sign = (opcode == WASM_OP_I64_LOAD16_S) ? true : false;
        goto op_i64_load;
      case WASM_OP_I64_LOAD32_S:
      case WASM_OP_I64_LOAD32_U:
        bytes = 4;
        sign = (opcode == WASM_OP_I64_LOAD32_S) ? true : false;
    op_i64_load:
        read_leb_uint32(frame_ip, frame_ip_end, align);
        read_leb_uint32(frame_ip, frame_ip_end, offset);
        if (!aot_compile_op_i64_load(comp_ctx, func_ctx, align, offset,
                                     bytes, sign, false))
          return false;
        break;

      case WASM_OP_F32_LOAD:
        read_leb_uint32(frame_ip, frame_ip_end, align);
        read_leb_uint32(frame_ip, frame_ip_end, offset);
        if (!aot_compile_op_f32_load(comp_ctx, func_ctx, align, offset))
          return false;
        break;

      case WASM_OP_F64_LOAD:
        read_leb_uint32(frame_ip, frame_ip_end, align);
        read_leb_uint32(frame_ip, frame_ip_end, offset);
        if (!aot_compile_op_f64_load(comp_ctx, func_ctx, align, offset))
          return false;
        break;

      case WASM_OP_I32_STORE:
        bytes = 4;
        goto op_i32_store;
      case WASM_OP_I32_STORE8:
        bytes = 1;
        goto op_i32_store;
      case WASM_OP_I32_STORE16:
        bytes = 2;
    op_i32_store:
        read_leb_uint32(frame_ip, frame_ip_end, align);
        read_leb_uint32(frame_ip, frame_ip_end, offset);
        if (!aot_compile_op_i32_store(comp_ctx, func_ctx, align,
                                      offset, bytes, false))
          return false;
        break;

      case WASM_OP_I64_STORE:
        bytes = 8;
        goto op_i64_store;
      case WASM_OP_I64_STORE8:
        bytes = 1;
        goto op_i64_store;
      case WASM_OP_I64_STORE16:
        bytes = 2;
        goto op_i64_store;
      case WASM_OP_I64_STORE32:
        bytes = 4;
    op_i64_store:
        read_leb_uint32(frame_ip, frame_ip_end, align);
        read_leb_uint32(frame_ip, frame_ip_end, offset);
        if (!aot_compile_op_i64_store(comp_ctx, func_ctx, align,
                                      offset, bytes, false))
          return false;
        break;

      case WASM_OP_F32_STORE:
        read_leb_uint32(frame_ip, frame_ip_end, align);
        read_leb_uint32(frame_ip, frame_ip_end, offset);
        if (!aot_compile_op_f32_store(comp_ctx, func_ctx, align, offset))
          return false;
        break;

      case WASM_OP_F64_STORE:
        read_leb_uint32(frame_ip, frame_ip_end, align);
        read_leb_uint32(frame_ip, frame_ip_end, offset);
        if (!aot_compile_op_f64_store(comp_ctx, func_ctx, align, offset))
          return false;
        break;

      case WASM_OP_MEMORY_SIZE:
        read_leb_uint32(frame_ip, frame_ip_end, mem_idx);
        if (!aot_compile_op_memory_size(comp_ctx, func_ctx))
          return false;
        (void)mem_idx;
        break;

      case WASM_OP_MEMORY_GROW:
        read_leb_uint32(frame_ip, frame_ip_end, mem_idx);
        if (!aot_compile_op_memory_grow(comp_ctx, func_ctx))
          return false;
        break;

      case WASM_OP_I32_CONST:
        read_leb_int32(frame_ip, frame_ip_end, i32_const);
        if (!aot_compile_op_i32_const(comp_ctx, func_ctx, i32_const))
          return false;
        break;

      case WASM_OP_I64_CONST:
        read_leb_int64(frame_ip, frame_ip_end, i64_const);
        if (!aot_compile_op_i64_const(comp_ctx, func_ctx, i64_const))
          return false;
        break;

      case WASM_OP_F32_CONST:
        p_f32 = (uint8*)&f32_const;
        for (i = 0; i < sizeof(float32); i++)
          *p_f32++ = *frame_ip++;
        if (!aot_compile_op_f32_const(comp_ctx, func_ctx, f32_const))
          return false;
        break;

      case WASM_OP_F64_CONST:
        p_f64 = (uint8*)&f64_const;
        for (i = 0; i < sizeof(float64); i++)
          *p_f64++ = *frame_ip++;
        if (!aot_compile_op_f64_const(comp_ctx, func_ctx, f64_const))
          return false;
        break;

      case WASM_OP_I32_EQZ:
      case WASM_OP_I32_EQ:
      case WASM_OP_I32_NE:
      case WASM_OP_I32_LT_S:
      case WASM_OP_I32_LT_U:
      case WASM_OP_I32_GT_S:
      case WASM_OP_I32_GT_U:
      case WASM_OP_I32_LE_S:
      case WASM_OP_I32_LE_U:
      case WASM_OP_I32_GE_S:
      case WASM_OP_I32_GE_U:
        if (!aot_compile_op_i32_compare(comp_ctx, func_ctx,
                                        INT_EQZ + opcode - WASM_OP_I32_EQZ))
          return false;
        break;

      case WASM_OP_I64_EQZ:
      case WASM_OP_I64_EQ:
      case WASM_OP_I64_NE:
      case WASM_OP_I64_LT_S:
      case WASM_OP_I64_LT_U:
      case WASM_OP_I64_GT_S:
      case WASM_OP_I64_GT_U:
      case WASM_OP_I64_LE_S:
      case WASM_OP_I64_LE_U:
      case WASM_OP_I64_GE_S:
      case WASM_OP_I64_GE_U:
        if (!aot_compile_op_i64_compare(comp_ctx, func_ctx,
                                        INT_EQZ + opcode - WASM_OP_I64_EQZ))
          return false;
        break;

      case WASM_OP_F32_EQ:
      case WASM_OP_F32_NE:
      case WASM_OP_F32_LT:
      case WASM_OP_F32_GT:
      case WASM_OP_F32_LE:
      case WASM_OP_F32_GE:
        if (!aot_compile_op_f32_compare(comp_ctx, func_ctx,
                                        FLOAT_EQ + opcode - WASM_OP_F32_EQ))
          return false;
        break;

      case WASM_OP_F64_EQ:
      case WASM_OP_F64_NE:
      case WASM_OP_F64_LT:
      case WASM_OP_F64_GT:
      case WASM_OP_F64_LE:
      case WASM_OP_F64_GE:
        if (!aot_compile_op_f64_compare(comp_ctx, func_ctx,
                                        FLOAT_EQ + opcode - WASM_OP_F64_EQ))
          return false;
        break;

      case WASM_OP_I32_CLZ:
        if (!aot_compile_op_i32_clz(comp_ctx, func_ctx))
          return false;
        break;

      case WASM_OP_I32_CTZ:
        if (!aot_compile_op_i32_ctz(comp_ctx, func_ctx))
          return false;
        break;

      case WASM_OP_I32_POPCNT:
        if (!aot_compile_op_i32_popcnt(comp_ctx, func_ctx))
          return false;
        break;

      case WASM_OP_I32_ADD:
      case WASM_OP_I32_SUB:
      case WASM_OP_I32_MUL:
      case WASM_OP_I32_DIV_S:
      case WASM_OP_I32_DIV_U:
      case WASM_OP_I32_REM_S:
      case WASM_OP_I32_REM_U:
        if (!aot_compile_op_i32_arithmetic(comp_ctx, func_ctx,
                                           INT_ADD + opcode - WASM_OP_I32_ADD,
                                           &frame_ip))
          return false;
        break;

      case WASM_OP_I32_AND:
      case WASM_OP_I32_OR:
      case WASM_OP_I32_XOR:
        if (!aot_compile_op_i32_bitwise(comp_ctx, func_ctx,
                                        INT_SHL + opcode - WASM_OP_I32_AND))
          return false;
        break;

      case WASM_OP_I32_SHL:
      case WASM_OP_I32_SHR_S:
      case WASM_OP_I32_SHR_U:
      case WASM_OP_I32_ROTL:
      case WASM_OP_I32_ROTR:
        if (!aot_compile_op_i32_shift(comp_ctx, func_ctx,
                                      INT_SHL + opcode - WASM_OP_I32_SHL))
          return false;
        break;

      case WASM_OP_I64_CLZ:
        if (!aot_compile_op_i64_clz(comp_ctx, func_ctx))
          return false;
        break;

      case WASM_OP_I64_CTZ:
        if (!aot_compile_op_i64_ctz(comp_ctx, func_ctx))
          return false;
        break;

      case WASM_OP_I64_POPCNT:
        if (!aot_compile_op_i64_popcnt(comp_ctx, func_ctx))
          return false;
        break;

      case WASM_OP_I64_ADD:
      case WASM_OP_I64_SUB:
      case WASM_OP_I64_MUL:
      case WASM_OP_I64_DIV_S:
      case WASM_OP_I64_DIV_U:
      case WASM_OP_I64_REM_S:
      case WASM_OP_I64_REM_U:
        if (!aot_compile_op_i64_arithmetic(comp_ctx, func_ctx,
                                           INT_ADD + opcode - WASM_OP_I64_ADD,
                                           &frame_ip))
          return false;
        break;

      case WASM_OP_I64_AND:
      case WASM_OP_I64_OR:
      case WASM_OP_I64_XOR:
        if (!aot_compile_op_i64_bitwise(comp_ctx, func_ctx,
                                        INT_SHL + opcode - WASM_OP_I64_AND))
          return false;
        break;

      case WASM_OP_I64_SHL:
      case WASM_OP_I64_SHR_S:
      case WASM_OP_I64_SHR_U:
      case WASM_OP_I64_ROTL:
      case WASM_OP_I64_ROTR:
        if (!aot_compile_op_i64_shift(comp_ctx, func_ctx,
                                      INT_SHL + opcode - WASM_OP_I64_SHL))
          return false;
        break;

      case WASM_OP_F32_ABS:
      case WASM_OP_F32_NEG:
      case WASM_OP_F32_CEIL:
      case WASM_OP_F32_FLOOR:
      case WASM_OP_F32_TRUNC:
      case WASM_OP_F32_NEAREST:
      case WASM_OP_F32_SQRT:
        if (!aot_compile_op_f32_math(comp_ctx, func_ctx,
                                     FLOAT_ABS + opcode - WASM_OP_F32_ABS))
          return false;
        break;

      case WASM_OP_F32_ADD:
      case WASM_OP_F32_SUB:
      case WASM_OP_F32_MUL:
      case WASM_OP_F32_DIV:
      case WASM_OP_F32_MIN:
      case WASM_OP_F32_MAX:
        if (!aot_compile_op_f32_arithmetic(comp_ctx, func_ctx,
                                           FLOAT_ADD + opcode - WASM_OP_F32_ADD))
          return false;
        break;

      case WASM_OP_F32_COPYSIGN:
        if (!aot_compile_op_f32_copysign(comp_ctx, func_ctx))
          return false;
        break;

      case WASM_OP_F64_ABS:
      case WASM_OP_F64_NEG:
      case WASM_OP_F64_CEIL:
      case WASM_OP_F64_FLOOR:
      case WASM_OP_F64_TRUNC:
      case WASM_OP_F64_NEAREST:
      case WASM_OP_F64_SQRT:
        if (!aot_compile_op_f64_math(comp_ctx, func_ctx,
                                     FLOAT_ABS + opcode - WASM_OP_F64_ABS))
          return false;
        break;

      case WASM_OP_F64_ADD:
      case WASM_OP_F64_SUB:
      case WASM_OP_F64_MUL:
      case WASM_OP_F64_DIV:
      case WASM_OP_F64_MIN:
      case WASM_OP_F64_MAX:
        if (!aot_compile_op_f64_arithmetic(comp_ctx, func_ctx,
                                           FLOAT_ADD + opcode - WASM_OP_F64_ADD))
          return false;
        break;

      case WASM_OP_F64_COPYSIGN:
        if (!aot_compile_op_f64_copysign(comp_ctx, func_ctx))
          return false;
        break;

      case WASM_OP_I32_WRAP_I64:
        if (!aot_compile_op_i32_wrap_i64(comp_ctx, func_ctx))
          return false;
        break;

      case WASM_OP_I32_TRUNC_S_F32:
      case WASM_OP_I32_TRUNC_U_F32:
        sign = (opcode == WASM_OP_I32_TRUNC_S_F32) ? true : false;
        if (!aot_compile_op_i32_trunc_f32(comp_ctx, func_ctx, sign, false))
          return false;
        break;

      case WASM_OP_I32_TRUNC_S_F64:
      case WASM_OP_I32_TRUNC_U_F64:
        sign = (opcode == WASM_OP_I32_TRUNC_S_F64) ? true : false;
        if (!aot_compile_op_i32_trunc_f64(comp_ctx, func_ctx, sign, false))
          return false;
        break;

      case WASM_OP_I64_EXTEND_S_I32:
      case WASM_OP_I64_EXTEND_U_I32:
        sign = (opcode == WASM_OP_I64_EXTEND_S_I32) ? true : false;
        if (!aot_compile_op_i64_extend_i32(comp_ctx, func_ctx, sign))
          return false;
        break;

      case WASM_OP_I64_TRUNC_S_F32:
      case WASM_OP_I64_TRUNC_U_F32:
        sign = (opcode == WASM_OP_I64_TRUNC_S_F32) ? true : false;
        if (!aot_compile_op_i64_trunc_f32(comp_ctx, func_ctx, sign, false))
          return false;
        break;

      case WASM_OP_I64_TRUNC_S_F64:
      case WASM_OP_I64_TRUNC_U_F64:
        sign = (opcode == WASM_OP_I64_TRUNC_S_F64) ? true : false;
        if (!aot_compile_op_i64_trunc_f64(comp_ctx, func_ctx, sign, false))
          return false;
        break;

      case WASM_OP_F32_CONVERT_S_I32:
      case WASM_OP_F32_CONVERT_U_I32:
        sign = (opcode == WASM_OP_F32_CONVERT_S_I32) ? true : false;
        if (!aot_compile_op_f32_convert_i32(comp_ctx, func_ctx, sign))
          return false;
        break;

      case WASM_OP_F32_CONVERT_S_I64:
      case WASM_OP_F32_CONVERT_U_I64:
        sign = (opcode == WASM_OP_F32_CONVERT_S_I64) ? true : false;
        if (!aot_compile_op_f32_convert_i64(comp_ctx, func_ctx, sign))
          return false;
        break;

      case WASM_OP_F32_DEMOTE_F64:
        if (!aot_compile_op_f32_demote_f64(comp_ctx, func_ctx))
          return false;
        break;

      case WASM_OP_F64_CONVERT_S_I32:
      case WASM_OP_F64_CONVERT_U_I32:
        sign = (opcode == WASM_OP_F64_CONVERT_S_I32) ? true : false;
        if (!aot_compile_op_f64_convert_i32(comp_ctx, func_ctx, sign))
          return false;
        break;

      case WASM_OP_F64_CONVERT_S_I64:
      case WASM_OP_F64_CONVERT_U_I64:
        sign = (opcode == WASM_OP_F64_CONVERT_S_I64) ? true : false;
        if (!aot_compile_op_f64_convert_i64(comp_ctx, func_ctx, sign))
          return false;
        break;

      case WASM_OP_F64_PROMOTE_F32:
        if (!aot_compile_op_f64_promote_f32(comp_ctx, func_ctx))
          return false;
        break;

      case WASM_OP_I32_REINTERPRET_F32:
        if (!aot_compile_op_i32_reinterpret_f32(comp_ctx, func_ctx))
            return false;
        break;

      case WASM_OP_I64_REINTERPRET_F64:
        if (!aot_compile_op_i64_reinterpret_f64(comp_ctx, func_ctx))
            return false;
        break;

      case WASM_OP_F32_REINTERPRET_I32:
        if (!aot_compile_op_f32_reinterpret_i32(comp_ctx, func_ctx))
            return false;
        break;

      case WASM_OP_F64_REINTERPRET_I64:
        if (!aot_compile_op_f64_reinterpret_i64(comp_ctx, func_ctx))
            return false;
        break;

      case WASM_OP_I32_EXTEND8_S:
        if (!aot_compile_op_i32_extend_i32(comp_ctx, func_ctx, 8))
          return false;
        break;

      case WASM_OP_I32_EXTEND16_S:
        if (!aot_compile_op_i32_extend_i32(comp_ctx, func_ctx, 16))
          return false;
        break;

      case WASM_OP_I64_EXTEND8_S:
        if (!aot_compile_op_i64_extend_i64(comp_ctx, func_ctx, 8))
          return false;
        break;

      case WASM_OP_I64_EXTEND16_S:
        if (!aot_compile_op_i64_extend_i64(comp_ctx, func_ctx, 16))
          return false;
        break;

      case WASM_OP_I64_EXTEND32_S:
        if (!aot_compile_op_i64_extend_i64(comp_ctx, func_ctx, 32))
          return false;
        break;

      case WASM_OP_MISC_PREFIX:
      {
        uint32 opcode1;

        read_leb_uint32(frame_ip, frame_ip_end, opcode1);
        opcode = (uint32)opcode1;

        switch (opcode) {
          case WASM_OP_I32_TRUNC_SAT_S_F32:
          case WASM_OP_I32_TRUNC_SAT_U_F32:
            sign = (opcode == WASM_OP_I32_TRUNC_SAT_S_F32) ? true : false;
            if (!aot_compile_op_i32_trunc_f32(comp_ctx, func_ctx, sign, true))
              return false;
            break;
          case WASM_OP_I32_TRUNC_SAT_S_F64:
          case WASM_OP_I32_TRUNC_SAT_U_F64:
            sign = (opcode == WASM_OP_I32_TRUNC_SAT_S_F64) ? true : false;
            if (!aot_compile_op_i32_trunc_f64(comp_ctx, func_ctx, sign, true))
              return false;
            break;
          case WASM_OP_I64_TRUNC_SAT_S_F32:
          case WASM_OP_I64_TRUNC_SAT_U_F32:
            sign = (opcode == WASM_OP_I64_TRUNC_SAT_S_F32) ? true : false;
            if (!aot_compile_op_i64_trunc_f32(comp_ctx, func_ctx, sign, true))
              return false;
            break;
          case WASM_OP_I64_TRUNC_SAT_S_F64:
          case WASM_OP_I64_TRUNC_SAT_U_F64:
            sign = (opcode == WASM_OP_I64_TRUNC_SAT_S_F64) ? true : false;
            if (!aot_compile_op_i64_trunc_f64(comp_ctx, func_ctx, sign, true))
              return false;
            break;
#if WASM_ENABLE_BULK_MEMORY != 0
          case WASM_OP_MEMORY_INIT:
          {
            uint32 seg_index;
            read_leb_uint32(frame_ip, frame_ip_end, seg_index);
            frame_ip ++;
            if (!aot_compile_op_memory_init(comp_ctx, func_ctx, seg_index))
              return false;
            break;
          }
          case WASM_OP_DATA_DROP:
          {
            uint32 seg_index;
            read_leb_uint32(frame_ip, frame_ip_end, seg_index);
            if (!aot_compile_op_data_drop(comp_ctx, func_ctx, seg_index))
              return false;
            break;
          }
          case WASM_OP_MEMORY_COPY:
          {
            frame_ip += 2;
            if (!aot_compile_op_memory_copy(comp_ctx, func_ctx))
              return false;
            break;
          }
          case WASM_OP_MEMORY_FILL:
          {
            frame_ip ++;
            if (!aot_compile_op_memory_fill(comp_ctx, func_ctx))
              return false;
            break;
          }
#endif /* WASM_ENABLE_BULK_MEMORY */
          default:
            break;
        }
        break;
      }
#if WASM_ENABLE_SHARED_MEMORY != 0
      case WASM_OP_ATOMIC_PREFIX:
      {
        uint8 bin_op, op_type;

        if (frame_ip < frame_ip_end) {
          opcode = *frame_ip++;
        }
        if (opcode != WASM_OP_ATOMIC_FENCE) {
          read_leb_uint32(frame_ip, frame_ip_end, align);
          read_leb_uint32(frame_ip, frame_ip_end, offset);
        }
        switch (opcode) {
          case WASM_OP_ATOMIC_WAIT32:
            if (!aot_compile_op_atomic_wait(comp_ctx, func_ctx, VALUE_TYPE_I32,
                                            align, offset, 4))
              return false;
            break;
          case WASM_OP_ATOMIC_WAIT64:
            if (!aot_compile_op_atomic_wait(comp_ctx, func_ctx, VALUE_TYPE_I64,
                                            align, offset, 8))
              return false;
            break;
          case WASM_OP_ATOMIC_NOTIFY:
            if (!aot_compiler_op_atomic_notify(comp_ctx, func_ctx, align,
                                               offset, bytes))
              return false;
            break;
          case WASM_OP_ATOMIC_I32_LOAD:
            bytes = 4;
            goto op_atomic_i32_load;
          case WASM_OP_ATOMIC_I32_LOAD8_U:
            bytes = 1;
            goto op_atomic_i32_load;
          case WASM_OP_ATOMIC_I32_LOAD16_U:
            bytes = 2;
          op_atomic_i32_load:
            if (!aot_compile_op_i32_load(comp_ctx, func_ctx, align,
                                         offset, bytes, sign, true))
              return false;
            break;

          case WASM_OP_ATOMIC_I64_LOAD:
            bytes = 8;
            goto op_atomic_i64_load;
          case WASM_OP_ATOMIC_I64_LOAD8_U:
            bytes = 1;
            goto op_atomic_i64_load;
          case WASM_OP_ATOMIC_I64_LOAD16_U:
            bytes = 2;
            goto op_atomic_i64_load;
          case WASM_OP_ATOMIC_I64_LOAD32_U:
            bytes = 4;
          op_atomic_i64_load:
            if (!aot_compile_op_i64_load(comp_ctx, func_ctx, align,
                                         offset, bytes, sign, true))
              return false;
            break;

          case WASM_OP_ATOMIC_I32_STORE:
            bytes = 4;
            goto op_atomic_i32_store;
          case WASM_OP_ATOMIC_I32_STORE8:
            bytes = 1;
            goto op_atomic_i32_store;
          case WASM_OP_ATOMIC_I32_STORE16:
            bytes = 2;
          op_atomic_i32_store:
            if (!aot_compile_op_i32_store(comp_ctx, func_ctx, align,
                                          offset, bytes, true))
              return false;
            break;

          case WASM_OP_ATOMIC_I64_STORE:
            bytes = 8;
            goto op_atomic_i64_store;
          case WASM_OP_ATOMIC_I64_STORE8:
            bytes = 1;
            goto op_atomic_i64_store;
          case WASM_OP_ATOMIC_I64_STORE16:
            bytes = 2;
            goto op_atomic_i64_store;
          case WASM_OP_ATOMIC_I64_STORE32:
            bytes = 4;
          op_atomic_i64_store:
            if (!aot_compile_op_i64_store(comp_ctx, func_ctx, align,
                                          offset, bytes, true))
              return false;
            break;

          case WASM_OP_ATOMIC_RMW_I32_CMPXCHG:
            bytes = 4;
            op_type = VALUE_TYPE_I32;
            goto op_atomic_cmpxchg;
          case WASM_OP_ATOMIC_RMW_I64_CMPXCHG:
            bytes = 8;
            op_type = VALUE_TYPE_I64;
            goto op_atomic_cmpxchg;
          case WASM_OP_ATOMIC_RMW_I32_CMPXCHG8_U:
            bytes = 1;
            op_type = VALUE_TYPE_I32;
            goto op_atomic_cmpxchg;
          case WASM_OP_ATOMIC_RMW_I32_CMPXCHG16_U:
            bytes = 2;
            op_type = VALUE_TYPE_I32;
            goto op_atomic_cmpxchg;
          case WASM_OP_ATOMIC_RMW_I64_CMPXCHG8_U:
            bytes = 1;
            op_type = VALUE_TYPE_I64;
            goto op_atomic_cmpxchg;
          case WASM_OP_ATOMIC_RMW_I64_CMPXCHG16_U:
            bytes = 2;
            op_type = VALUE_TYPE_I64;
            goto op_atomic_cmpxchg;
          case WASM_OP_ATOMIC_RMW_I64_CMPXCHG32_U:
            bytes = 4;
            op_type = VALUE_TYPE_I64;
          op_atomic_cmpxchg:
            if (!aot_compile_op_atomic_cmpxchg(comp_ctx, func_ctx,
                                               op_type, align,
                                               offset, bytes))
              return false;
            break;

          COMPILE_ATOMIC_RMW(Add, ADD);
          COMPILE_ATOMIC_RMW(Sub, SUB);
          COMPILE_ATOMIC_RMW(And, AND);
          COMPILE_ATOMIC_RMW(Or, OR);
          COMPILE_ATOMIC_RMW(Xor, XOR);
          COMPILE_ATOMIC_RMW(Xchg, XCHG);

build_atomic_rmw:
            if (!aot_compile_op_atomic_rmw(comp_ctx, func_ctx,
                                           bin_op, op_type,
                                           align, offset, bytes))
              return false;
            break;

          default:
            break;
        }
        break;
      }
#endif /* end of WASM_ENABLE_SHARED_MEMORY */

      default:
        break;
    }
  }

  /* Move func_return block to the bottom */
  if (func_ctx->func_return_block) {
      LLVMBasicBlockRef last_block =
          LLVMGetLastBasicBlock(func_ctx->func);
      if (last_block != func_ctx->func_return_block)
          LLVMMoveBasicBlockAfter(func_ctx->func_return_block,
                                  last_block);
  }

  /* Move got_exception block to the bottom */
  if (func_ctx->got_exception_block) {
      LLVMBasicBlockRef last_block =
          LLVMGetLastBasicBlock(func_ctx->func);
      if (last_block != func_ctx->got_exception_block)
          LLVMMoveBasicBlockAfter(func_ctx->got_exception_block,
                                  last_block);

      /* Move all other exception blocks before got_exception block */
      for (i = 0; i < EXCE_NUM; i++) {
          if (func_ctx->exception_blocks[i])
              LLVMMoveBasicBlockBefore(func_ctx->exception_blocks[i],
                                       func_ctx->got_exception_block);
      }
  }
  return true;

fail:
  return false;
}

bool
aot_compile_wasm(AOTCompContext *comp_ctx)
{
  char *msg = NULL;
  bool ret;
  uint32 i;

  bh_print_time("Begin to compile WASM bytecode to LLVM IR");

  for (i = 0; i < comp_ctx->func_ctx_count; i++)
    if (!aot_compile_func(comp_ctx, i)) {
#if 0
      LLVMDumpModule(comp_ctx->module);
      char *err;
      LLVMTargetMachineEmitToFile(comp_ctx->target_machine, comp_ctx->module,
                                  "./test.o", LLVMObjectFile, &err);
#endif
      return false;
    }

#if 0
  LLVMDumpModule(comp_ctx->module);
  /* Clear error no, LLVMDumpModule may set errno */
  errno = 0;
#endif

  bh_print_time("Begin to verify LLVM module");

  ret = LLVMVerifyModule(comp_ctx->module, LLVMPrintMessageAction, &msg);
  if (!ret && msg) {
      if (msg[0] != '\0') {
          aot_set_last_error(msg);
          LLVMDisposeMessage(msg);
          return false;
      }
      LLVMDisposeMessage(msg);
  }

  bh_print_time("Begin to run function optimization passes");

  if (comp_ctx->optimize) {
      LLVMInitializeFunctionPassManager(comp_ctx->pass_mgr);
      for (i = 0; i < comp_ctx->func_ctx_count; i++)
          LLVMRunFunctionPassManager(comp_ctx->pass_mgr,
                                     comp_ctx->func_ctxes[i]->func);
  }

  return true;
}

bool
aot_emit_llvm_file(AOTCompContext *comp_ctx, const char *file_name)
{
    char *err = NULL;

    bh_print_time("Begin to emit LLVM IR file");

    if (LLVMPrintModuleToFile(comp_ctx->module, file_name, &err) != 0) {
        if (err) {
            LLVMDisposeMessage(err);
            err = NULL;
        }
        aot_set_last_error("emit llvm ir to file failed.");
        return false;
    }

    return true;
}

bool
aot_emit_object_file(AOTCompContext *comp_ctx, char *file_name)
{
    char *err = NULL;

    bh_print_time("Begin to emit object file");

    if (LLVMTargetMachineEmitToFile(comp_ctx->target_machine,
                                    comp_ctx->module,
                                    file_name,
                                    LLVMObjectFile,
                                    &err) != 0) {
        if (err) {
            LLVMDisposeMessage(err);
            err = NULL;
        }
        aot_set_last_error("emit elf to memory buffer failed.");
        return false;
    }

    return true;
}

