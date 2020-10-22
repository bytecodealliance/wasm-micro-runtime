/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _AOT_COMPILER_H_
#define _AOT_COMPILER_H_

#include "aot.h"
#include "aot_llvm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum IntCond {
  INT_EQZ = 0,
  INT_EQ,
  INT_NE,
  INT_LT_S,
  INT_LT_U,
  INT_GT_S,
  INT_GT_U,
  INT_LE_S,
  INT_LE_U,
  INT_GE_S,
  INT_GE_U
} IntCond;

typedef enum FloatCond {
  FLOAT_EQ = 0,
  FLOAT_NE,
  FLOAT_LT,
  FLOAT_GT,
  FLOAT_LE,
  FLOAT_GE
} FloatCond;

typedef enum IntArithmetic {
  INT_ADD = 0,
  INT_SUB,
  INT_MUL,
  INT_DIV_S,
  INT_DIV_U,
  INT_REM_S,
  INT_REM_U
} IntArithmetic;

typedef enum IntBitwise {
  INT_AND = 0,
  INT_OR,
  INT_XOR,
} IntBitwise;

typedef enum IntShift {
  INT_SHL = 0,
  INT_SHR_S,
  INT_SHR_U,
  INT_ROTL,
  INT_ROTR
} IntShift;

typedef enum FloatMath {
  FLOAT_ABS = 0,
  FLOAT_NEG,
  FLOAT_CEIL,
  FLOAT_FLOOR,
  FLOAT_TRUNC,
  FLOAT_NEAREST,
  FLOAT_SQRT
} FloatMath;

typedef enum FloatArithmetic {
  FLOAT_ADD = 0,
  FLOAT_SUB,
  FLOAT_MUL,
  FLOAT_DIV,
  FLOAT_MIN,
  FLOAT_MAX
} FloatArithmetic;

#define CHECK_STACK() do {                                  \
    if (!func_ctx->block_stack.block_list_end) {            \
      aot_set_last_error("WASM block stack underflow.");    \
      goto fail;                                            \
    }                                                       \
    if (!func_ctx->block_stack.block_list_end->             \
                    value_stack.value_list_end) {           \
      aot_set_last_error("WASM data stack underflow.");     \
      goto fail;                                            \
    }                                                       \
  } while (0)

#define POP(llvm_value, value_type) do {                    \
    AOTValue *aot_value;                                    \
    CHECK_STACK();                                          \
    aot_value = aot_value_stack_pop                         \
      (&func_ctx->block_stack.block_list_end->value_stack); \
    if ((value_type != VALUE_TYPE_I32                       \
         && aot_value->type != value_type)                  \
        || (value_type == VALUE_TYPE_I32                    \
            && (aot_value->type != VALUE_TYPE_I32           \
                && aot_value->type != VALUE_TYPE_I1))) {    \
      aot_set_last_error("invalid WASM stack data type.");  \
      wasm_runtime_free(aot_value);                         \
      goto fail;                                            \
    }                                                       \
    if (aot_value->type == value_type)                      \
      llvm_value = aot_value->value;                        \
    else {                                                  \
      bh_assert(aot_value->type == VALUE_TYPE_I1);          \
      if (!(llvm_value = LLVMBuildZExt(comp_ctx->builder,   \
            aot_value->value, I32_TYPE, "i1toi32"))) {      \
        aot_set_last_error("invalid WASM stack data type.");\
        wasm_runtime_free(aot_value);                       \
        goto fail;                                          \
      }                                                     \
    }                                                       \
    wasm_runtime_free(aot_value);                           \
  } while (0)

#define POP_I32(v) POP(v, VALUE_TYPE_I32)
#define POP_I64(v) POP(v, VALUE_TYPE_I64)
#define POP_F32(v) POP(v, VALUE_TYPE_F32)
#define POP_F64(v) POP(v, VALUE_TYPE_F64)

#define POP_COND(llvm_value) do {                           \
    AOTValue *aot_value;                                    \
    CHECK_STACK();                                          \
    aot_value = aot_value_stack_pop                         \
      (&func_ctx->block_stack.block_list_end->value_stack); \
    if (aot_value->type != VALUE_TYPE_I1                    \
        && aot_value->type != VALUE_TYPE_I32) {             \
      aot_set_last_error("invalid WASM stack data type.");  \
      wasm_runtime_free(aot_value);                         \
      goto fail;                                            \
    }                                                       \
    if (aot_value->type == VALUE_TYPE_I1)                   \
      llvm_value = aot_value->value;                        \
    else {                                                  \
      if (!(llvm_value = LLVMBuildICmp(comp_ctx->builder,   \
                    LLVMIntNE, aot_value->value, I32_ZERO,  \
                    "i1_cond"))){                           \
        aot_set_last_error("llvm build trunc failed.");     \
        wasm_runtime_free(aot_value);                       \
        goto fail;                                          \
      }                                                     \
    }                                                       \
    wasm_runtime_free(aot_value);                           \
  } while (0)

#define PUSH(llvm_value, value_type) do {                   \
    AOTValue *aot_value;                                    \
    if (!func_ctx->block_stack.block_list_end) {            \
      aot_set_last_error("WASM block stack underflow.");    \
      goto fail;                                            \
    }                                                       \
    aot_value = wasm_runtime_malloc(sizeof(AOTValue));      \
    memset(aot_value, 0, sizeof(AOTValue));                 \
    if (!aot_value) {                                       \
      aot_set_last_error("allocate memory failed.");        \
      goto fail;                                            \
    }                                                       \
    aot_value->type = value_type;                           \
    aot_value->value = llvm_value;                          \
    aot_value_stack_push                                    \
        (&func_ctx->block_stack.block_list_end->value_stack,\
         aot_value);                                        \
  } while (0)

#define PUSH_I32(v) PUSH(v, VALUE_TYPE_I32)
#define PUSH_I64(v) PUSH(v, VALUE_TYPE_I64)
#define PUSH_F32(v) PUSH(v, VALUE_TYPE_F32)
#define PUSH_F64(v) PUSH(v, VALUE_TYPE_F64)
#define PUSH_COND(v) PUSH(v, VALUE_TYPE_I1)

#define TO_LLVM_TYPE(wasm_type) \
    wasm_type_to_llvm_type(&comp_ctx->basic_types, wasm_type)

#define I32_TYPE comp_ctx->basic_types.int32_type
#define I64_TYPE comp_ctx->basic_types.int64_type
#define F32_TYPE comp_ctx->basic_types.float32_type
#define F64_TYPE comp_ctx->basic_types.float64_type
#define VOID_TYPE comp_ctx->basic_types.void_type
#define INT1_TYPE comp_ctx->basic_types.int1_type
#define INT8_TYPE comp_ctx->basic_types.int8_type
#define INT16_TYPE comp_ctx->basic_types.int16_type
#define MD_TYPE comp_ctx->basic_types.meta_data_type
#define INT8_PTR_TYPE comp_ctx->basic_types.int8_ptr_type
#define INT16_PTR_TYPE comp_ctx->basic_types.int16_ptr_type
#define INT32_PTR_TYPE comp_ctx->basic_types.int32_ptr_type
#define INT64_PTR_TYPE comp_ctx->basic_types.int64_ptr_type
#define F32_PTR_TYPE comp_ctx->basic_types.float32_ptr_type
#define F64_PTR_TYPE comp_ctx->basic_types.float64_ptr_type

#define I32_CONST(v) LLVMConstInt(I32_TYPE, v, true)
#define I64_CONST(v) LLVMConstInt(I64_TYPE, v, true)
#define F32_CONST(v) LLVMConstReal(F32_TYPE, v)
#define F64_CONST(v) LLVMConstReal(F64_TYPE, v)
#define I8_CONST(v) LLVMConstInt(INT8_TYPE, v, true)

#define I8_ZERO     (comp_ctx->llvm_consts.i8_zero)
#define I32_ZERO    (comp_ctx->llvm_consts.i32_zero)
#define I64_ZERO    (comp_ctx->llvm_consts.i64_zero)
#define F32_ZERO    (comp_ctx->llvm_consts.f32_zero)
#define F64_ZERO    (comp_ctx->llvm_consts.f64_zero)
#define I32_ONE     (comp_ctx->llvm_consts.i32_one)
#define I32_TWO     (comp_ctx->llvm_consts.i32_two)
#define I32_THREE   (comp_ctx->llvm_consts.i32_three)
#define I32_FOUR    (comp_ctx->llvm_consts.i32_four)
#define I32_EIGHT   (comp_ctx->llvm_consts.i32_eight)
#define I32_NEG_ONE (comp_ctx->llvm_consts.i32_neg_one)
#define I64_NEG_ONE (comp_ctx->llvm_consts.i64_neg_one)
#define I32_MIN     (comp_ctx->llvm_consts.i32_min)
#define I64_MIN     (comp_ctx->llvm_consts.i64_min)
#define I32_31     (comp_ctx->llvm_consts.i32_31)
#define I32_32     (comp_ctx->llvm_consts.i32_32)
#define I64_63     (comp_ctx->llvm_consts.i64_63)
#define I64_64     (comp_ctx->llvm_consts.i64_64)

#define CHECK_LLVM_CONST(v) do {                        \
    if (!v) {                                           \
      aot_set_last_error("create llvm const failed.");  \
      goto fail;                                        \
    }                                                   \
  } while (0)

bool
aot_compile_wasm(AOTCompContext *comp_ctx);

bool
aot_emit_llvm_file(AOTCompContext *comp_ctx, const char *file_name);

bool
aot_emit_aot_file(AOTCompContext *comp_ctx,
                  AOTCompData *comp_data,
                  const char *file_name);

bool
aot_emit_object_file(AOTCompContext *comp_ctx, char *file_name);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _AOT_COMPILER_H_ */

