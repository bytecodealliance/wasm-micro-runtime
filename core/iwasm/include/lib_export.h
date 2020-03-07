/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _LIB_EXPORT_H_
#define _LIB_EXPORT_H_

#include <inttypes.h>

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
uint32_t
get_base_lib_export_apis(NativeSymbol **p_base_lib_apis);

#ifdef __cplusplus
}
#endif

#endif

