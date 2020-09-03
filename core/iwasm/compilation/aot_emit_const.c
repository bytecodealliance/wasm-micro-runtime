/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_emit_const.h"


bool
aot_compile_op_i32_const(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         int32 i32_const)
{
  LLVMValueRef value = I32_CONST((uint32)i32_const);
  CHECK_LLVM_CONST(value);
  PUSH_I32(value);
  return true;
fail:
  return false;
}

bool
aot_compile_op_i64_const(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         int64 i64_const)
{
  LLVMValueRef value = I64_CONST((uint64)i64_const);
  CHECK_LLVM_CONST(value);
  PUSH_I64(value);
  return true;
fail:
  return false;
}

bool
aot_compile_op_f32_const(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         float32 f32_const)
{
  LLVMValueRef alloca, value;

  if (!isnan(f32_const)) {
    value = F32_CONST(f32_const);
    CHECK_LLVM_CONST(value);
    PUSH_F32(value);
  }
  else {
      int32 i32_const;
      memcpy(&i32_const, &f32_const, sizeof(int32));
      if (!(alloca = LLVMBuildAlloca(comp_ctx->builder,
                                     I32_TYPE, "i32_ptr"))) {
          aot_set_last_error("llvm build alloca failed.");
          return false;
      }
      if (!LLVMBuildStore(comp_ctx->builder,
                          I32_CONST((uint32)i32_const), alloca)) {
          aot_set_last_error("llvm build store failed.");
          return false;
      }
      if (!(alloca = LLVMBuildBitCast(comp_ctx->builder,
                                      alloca, F32_PTR_TYPE, "f32_ptr"))) {
          aot_set_last_error("llvm build bitcast failed.");
          return false;
      }
      if (!(value = LLVMBuildLoad(comp_ctx->builder, alloca, ""))) {
          aot_set_last_error("llvm build load failed.");
          return false;
      }
      PUSH_F32(value);
  }

  return true;
fail:
  return false;
}

bool
aot_compile_op_f64_const(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         float64 f64_const)
{
  LLVMValueRef alloca, value;

  if (!isnan(f64_const)) {
    value = F64_CONST(f64_const);
    CHECK_LLVM_CONST(value);
    PUSH_F64(value);
  }
  else {
      int64 i64_const;
      memcpy(&i64_const, &f64_const, sizeof(int64));
      if (!(alloca = LLVMBuildAlloca(comp_ctx->builder,
                                     I64_TYPE, "i64_ptr"))) {
          aot_set_last_error("llvm build alloca failed.");
          return false;
      }
      value = I64_CONST((uint64)i64_const);
      CHECK_LLVM_CONST(value);
      if (!LLVMBuildStore(comp_ctx->builder, value, alloca)) {
          aot_set_last_error("llvm build store failed.");
          return false;
      }
      if (!(alloca = LLVMBuildBitCast(comp_ctx->builder,
                                      alloca, F64_PTR_TYPE, "f64_ptr"))) {
          aot_set_last_error("llvm build bitcast failed.");
          return false;
      }
      if (!(value = LLVMBuildLoad(comp_ctx->builder, alloca, ""))) {
          aot_set_last_error("llvm build load failed.");
          return false;
      }
      PUSH_F64(value);
  }

  return true;
fail:
  return false;
}

