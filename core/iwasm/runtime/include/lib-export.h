/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _LIB_EXPORT_H_
#define _LIB_EXPORT_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NativeSymbol {
    const char *symbol;
    void *func_ptr;
} NativeSymbol;

#define EXPORT_WASM_API(symbol)  {#symbol, symbol}
#define EXPORT_WASM_API2(symbol) {#symbol, symbol##_wrapper}

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
 * Get the exported APIs of extend lib
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

