/*
 * Copyright (C) 2021 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _JIT_CODEGEN_H_
#define _JIT_CODEGEN_H_

#include "bh_platform.h"
#include "jit_ir.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize codegen module, such as instruction encoder.
 *
 * @return true if succeeded; false if failed.
 */
bool
jit_codegen_init();

/**
 * Destroy codegen module, such as instruction encoder.
 */
void
jit_codegen_destroy();

/**
 * Get hard register information of each kind.
 *
 * @return the JitHardRegInfo array of each kind
 */
const JitHardRegInfo *
jit_codegen_get_hreg_info();

/**
 * Generate native code for the given compilation context
 *
 * @param cc the compilation context that is ready to do codegen
 *
 * @return true if succeeds, false otherwise
 */
bool
jit_codegen_gen_native(JitCompContext *cc);

/**
 * lower unsupported operations to supported ones for the target.
 *
 * @param cc the compilation context that is ready to do codegen
 *
 * @return true if succeeds, false otherwise
 */
bool
jit_codegen_lower(JitCompContext *cc);

/**
 * Dump native code in the given range to assembly.
 *
 * @param begin_addr begin address of the native code
 * @param end_addr end address of the native code
 */
void
jit_codegen_dump_native(void *begin_addr, void *end_addr);

/**
 * Call jitted code
 *
 * @param exec_env the current exec_env
 */
bool
jit_codegen_call_func_jitted(void *exec_env, void *frame, void *func_inst,
                             void *target);

#ifdef __cplusplus
}
#endif

#endif /* end of _JIT_CODEGEN_H_ */
