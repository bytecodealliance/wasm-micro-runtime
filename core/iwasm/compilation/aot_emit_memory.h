/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _AOT_EMIT_MEMORY_H_
#define _AOT_EMIT_MEMORY_H_

#include "aot_compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
aot_compile_op_i32_load(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                        uint32 align, uint32 offset, uint32 bytes, bool sign);

bool
aot_compile_op_i64_load(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                        uint32 align, uint32 offset, uint32 bytes, bool sign);

bool
aot_compile_op_f32_load(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                        uint32 align, uint32 offset);

bool
aot_compile_op_f64_load(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                        uint32 align, uint32 offset);

bool
aot_compile_op_i32_store(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 align, uint32 offset, uint32 bytes);

bool
aot_compile_op_i64_store(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 align, uint32 offset, uint32 bytes);

bool
aot_compile_op_f32_store(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 align, uint32 offset);

bool
aot_compile_op_f64_store(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                         uint32 align, uint32 offset);

bool
aot_compile_op_memory_size(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx);

bool
aot_compile_op_memory_grow(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _AOT_EMIT_MEMORY_H_ */

