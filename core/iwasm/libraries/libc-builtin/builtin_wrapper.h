/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#ifndef _BUILTIN_WRAPPER_H_
#define _BUILTIN_WRAPPER_H_

#include "wasm_export.h"
#include "../interpreter/wasm.h"
#include "../common/wasm_runtime_common.h"

/*************************************
 * Functions
 *************************************/

/*************************************
 * Globals
 *************************************/
typedef struct WASMNativeGlobalDef {
    const char *module_name;
    const char *name;
    uint8 type;
    bool is_mutable;
    WASMValue value;
} WASMNativeGlobalDef;

/*************************************
 * Tables
 *************************************/

typedef struct WASMNativeTableDef {
    const char *module_name;
    const char *name;
    uint8 elem_type;

} WASMNativeTableDef;

/*************************************
 * Memories
 *************************************/

typedef struct WASMNativeMemoryDef {
    const char *module_name;
    const char *name;
} WASMNativeMemoryDef;

#endif
