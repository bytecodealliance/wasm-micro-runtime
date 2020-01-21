/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_native.h"


typedef struct NativeSymbol {
    const char *symbol;
    void *func_ptr;
} NativeSymbol;

static bool
sort_symbol_ptr(NativeSymbol *ptr, int len)
{
    int i, j;
    NativeSymbol temp;

    for (i = 0; i < len - 1; ++i) {
        for (j = i + 1; j < len; ++j) {
            if (strcmp((ptr+i)->symbol, (ptr+j)->symbol) > 0) {
                temp = ptr[i];
                ptr[i] = ptr[j];
                ptr[j] = temp;
            }
        }
    }

    return true;
}

static void *
lookup_symbol(NativeSymbol *ptr, int len, const char *symbol)
{
    int low = 0, mid, ret;
    int high = len - 1;

    while (low <= high) {
        mid = (low + high) / 2;
        ret = strcmp(symbol, ptr[mid].symbol);

        if (ret == 0)
            return ptr[mid].func_ptr;
        else if (ret < 0)
            high = mid - 1;
        else
            low = mid + 1;
    }

    return NULL;
}

#if WASM_ENABLE_BASE_LIB != 0
static bool is_base_lib_sorted = false;
static NativeSymbol *base_native_symbol_defs;
static int base_native_symbol_len;

int
get_base_lib_export_apis(NativeSymbol **p_base_lib_apis);

void *
wasm_native_lookup_base_lib_func(const char *module_name,
                                 const char *func_name)
{
    void *ret;

    if (strcmp(module_name, "env"))
        return NULL;

    if (!is_base_lib_sorted) {
        base_native_symbol_len = get_base_lib_export_apis(&base_native_symbol_defs);

        if (base_native_symbol_len > 0)
            sort_symbol_ptr(base_native_symbol_defs, base_native_symbol_len);

        is_base_lib_sorted = true;
    }

    if ((ret = lookup_symbol(base_native_symbol_defs, base_native_symbol_len,
                             func_name))
        || (func_name[0] == '_'
            && (ret = lookup_symbol(base_native_symbol_defs, base_native_symbol_len,
                                    func_name + 1))))
        return ret;

    return NULL;
}
#endif /* end of WASM_ENABLE_BASE_LIB */

static bool is_ext_lib_sorted = false;
static NativeSymbol *ext_native_symbol_defs;
static int ext_native_symbol_len;

int
get_ext_lib_export_apis(NativeSymbol **p_ext_lib_apis);

void *
wasm_native_lookup_extension_lib_func(const char *module_name,
                                      const char *func_name)
{
    void *ret;

    if (strcmp(module_name, "env"))
        return NULL;

    if (!is_ext_lib_sorted) {
        ext_native_symbol_len = get_ext_lib_export_apis(&ext_native_symbol_defs);

        if (ext_native_symbol_len > 0)
            sort_symbol_ptr(ext_native_symbol_defs, ext_native_symbol_len);

        is_ext_lib_sorted = true;
    }

    if ((ret = lookup_symbol(ext_native_symbol_defs, ext_native_symbol_len,
                             func_name))
        || (func_name[0] == '_'
            && (ret = lookup_symbol(ext_native_symbol_defs, ext_native_symbol_len,
                                    func_name + 1))))
        return ret;

    return NULL;
}

