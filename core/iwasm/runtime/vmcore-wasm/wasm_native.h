/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WASM_NATIVE_H
#define _WASM_NATIVE_H

#include "wasm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the native module, e.g. sort the function defs
 * and the global defs.
 *
 * @return true if success, false otherwise
 */
bool
wasm_native_init();

/**
 * Lookup native function implementation of a given import function.
 *
 * @param module_name the module name of the import function
 * @param func_name the function name of the import function
 *
 * @return return the native function pointer if success, NULL otherwise
 */
void*
wasm_native_func_lookup(const char *module_name, const char *func_name);

/**
 * Lookup global variable of a given import global
 *
 * @param module_name the module name of the import global
 * @param global_name the global name of the import global
 * @param global return the global data
 *
 * @param return true if success, false otherwise
 */
bool
wasm_native_global_lookup(const char *module_name, const char *global_name,
                          WASMGlobalImport *global);

#ifdef __cplusplus
}
#endif

#endif /* end of _WASM_NATIVE_H */
