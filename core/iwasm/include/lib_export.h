/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _LIB_EXPORT_H_
#define _LIB_EXPORT_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NativeSymbol {
    const char *symbol;
    void *func_ptr;
    const char *signature;
} NativeSymbol;

#define EXPORT_WASM_API(symbol)  {#symbol, (void*)symbol, NULL}
#define EXPORT_WASM_API2(symbol) {#symbol, (void*)symbol##_wrapper, NULL}

#define EXPORT_WASM_API_WITH_SIG(symbol, signature) \
                                 {#symbol, (void*)symbol, signature}
#define EXPORT_WASM_API_WITH_SIG2(symbol, signature) \
                                 {#symbol, (void*)symbol##_wrapper, signature}

/**
 * Get the exported APIs of base lib
 *
 * @param p_base_lib_apis return the exported API array of base lib
 *
 * @return the number of the exported API
 */
int
get_base_lib_export_apis(NativeSymbol **p_base_lib_apis);

/**
 * Get the exported APIs of extended lib, this API isn't provided by WASM VM,
 * it must be provided by developer to register the extended native APIs,
 * for example, developer can register his native APIs to extended_native_symbol_defs,
 * array, and include file ext_lib_export.h which implements this API.
 * And if developer hasn't any native API to register, he can define an empty
 * extended_native_symbol_defs array, and then include file ext_lib_export.h to
 * implements this API.
 *
 * @param p_base_lib_apis return the exported API array of extend lib
 *
 * @return the number of the exported API
 */
int
get_extend_lib_export_apis(NativeSymbol **p_base_lib_apis);

#ifdef __cplusplus
}
#endif

#endif

