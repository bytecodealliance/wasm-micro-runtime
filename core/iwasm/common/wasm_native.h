/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WASM_NATIVE_H
#define _WASM_NATIVE_H

#include "bh_common.h"
#if WASM_ENABLE_INTERP != 0 || WASM_ENABLE_JIT != 0
#include "../interpreter/wasm.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Lookup native function implementation of a given import function
 * in libc builtin API's
 *
 * @param module_name the module name of the import function
 * @param func_name the function name of the import function
 *
 * @return return the native function pointer if success, NULL otherwise
 */
void *
wasm_native_lookup_libc_builtin_func(const char *module_name,
                                     const char *func_name);

#if WASM_ENABLE_INTERP != 0 || WASM_ENABLE_JIT != 0
/**
 * Lookup global variable of a given import global
 * in libc builtin globals
 *
 * @param module_name the module name of the import global
 * @param global_name the global name of the import global
 * @param global return the global data
 *
 * @param return true if success, false otherwise
 */
bool
wasm_native_lookup_libc_builtin_global(const char *module_name,
                                       const char *global_name,
                                       WASMGlobalImport *global);
#endif

/**
 * Lookup native function implementation of a given import function
 * in libc wasi API's
 *
 * @param module_name the module name of the import function
 * @param func_name the function name of the import function
 *
 * @return return the native function pointer if success, NULL otherwise
 */
void *
wasm_native_lookup_libc_wasi_func(const char *module_name,
                                  const char *func_name);

/**
 * Lookup native function implementation of a given import function
 * in base lib API's
 *
 * @param module_name the module name of the import function
 * @param func_name the function name of the import function
 *
 * @return return the native function pointer if success, NULL otherwise
 */
void *
wasm_native_lookup_base_lib_func(const char *module_name,
                                 const char *func_name);

/**
 * Lookup native function implementation of a given import function
 * in extension lib API's
 *
 * @param module_name the module name of the import function
 * @param func_name the function name of the import function
 *
 * @return return the native function pointer if success, NULL otherwise
 */
void *
wasm_native_lookup_extension_lib_func(const char *module_name,
                                      const char *func_name);

#ifdef __cplusplus
}
#endif

#endif /* end of _WASM_NATIVE_H */

