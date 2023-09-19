/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _AOT_EMIT_GC_H_
#define _AOT_EMIT_GC_H_

#include "aot_compiler.h"
#include "aot_runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

#if WASM_ENABLE_GC != 0

bool
aot_call_aot_create_func_obj(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                             LLVMValueRef func_idx, LLVMValueRef *p_gc_obj);

bool
aot_call_aot_obj_is_instance_of(AOTCompContext *comp_ctx,
                                AOTFuncContext *func_ctx, LLVMValueRef gc_obj,
                                LLVMValueRef heap_type, LLVMValueRef *castable);

bool
aot_call_wasm_obj_is_type_of(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                             LLVMValueRef gc_obj, LLVMValueRef heap_type,
                             LLVMValueRef *castable);

bool
aot_compile_op_ref_as_non_null(AOTCompContext *comp_ctx,
                               AOTFuncContext *func_ctx);

bool
aot_compile_op_i31_new(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx);

bool
aot_compile_op_i31_get(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                       bool sign);

bool
aot_compile_op_ref_test(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                        int32 heap_type, bool nullable, bool cast);

bool
aot_compile_op_extern_internalize(AOTCompContext *comp_ctx,
                                  AOTFuncContext *func_ctx);

bool
aot_compile_op_extern_externalize(AOTCompContext *comp_ctx,
                                  AOTFuncContext *func_ctx);

#endif

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _AOT_EMIT_GC_H_ */
