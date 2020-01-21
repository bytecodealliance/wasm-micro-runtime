/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _AOT_EMIT_FUNCTION_H_
#define _AOT_EMIT_FUNCTION_H_

#include "aot_compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
aot_compile_op_call(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                    uint32 func_idx, uint8 **p_frame_ip);

bool
aot_compile_op_call_indirect(AOTCompContext *comp_ctx, AOTFuncContext *func_ctx,
                             uint32 type_idx);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _AOT_EMIT_FUNCTION_H_ */

