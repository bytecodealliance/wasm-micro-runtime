/*
 * Copyright (C) 2021 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _JIT_COMPILER_H_
#define _JIT_COMPILER_H_

#include "bh_platform.h"
#include "../interpreter/wasm_runtime.h"
#include "jit_ir.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
jit_compiler_init();

void
jit_compiler_destroy();

const char *
jit_compiler_get_pass_name(unsigned i);

bool
jit_compiler_compile(WASMModule *module, uint32 func_idx);

bool
jit_compiler_compile_all(WASMModule *module);

bool
jit_interp_switch_to_jitted(void *exec_env, void *frame,
                            WASMFunctionInstance *func_inst, void *target);

/*
 * Pass declarations:
 */

/**
 * Dump the compilation context.
 */
bool
jit_pass_dump(JitCompContext *cc);

/**
 * Update CFG (usually before dump for better readability).
 */
bool
jit_pass_update_cfg(JitCompContext *cc);

/**
 * Translate profiling result into MIR.
 */
bool
jit_pass_frontend(JitCompContext *cc);

/**
 * Convert MIR to LIR.
 */
bool
jit_pass_lower_fe(JitCompContext *cc);

/**
 * Lower unsupported operations into supported ones.
 */
bool
jit_pass_lower_cg(JitCompContext *cc);

/**
 * Register allocation.
 */
bool
jit_pass_regalloc(JitCompContext *cc);

/**
 * Native code generation.
 */
bool
jit_pass_codegen(JitCompContext *cc);

/**
 * Register the jitted code so that it can be executed.
 */
bool
jit_pass_register_jitted_code(JitCompContext *cc);

#ifdef __cplusplus
}
#endif

#endif /* end of _JIT_COMPILER_H_ */
