/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"


static bool sort_flag = false;

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

int
get_base_lib_export_apis(NativeSymbol **p_base_lib_apis);

int
get_ext_lib_export_apis(NativeSymbol **p_ext_lib_apis);

static NativeSymbol *base_native_symbol_defs;
static NativeSymbol *ext_native_symbol_defs;
static int base_native_symbol_len;
static int ext_native_symbol_len;

void *
wasm_dlsym(void *handle, const char *symbol)
{
    void *ret;

    if (!sort_flag) {
        base_native_symbol_len = get_base_lib_export_apis(&base_native_symbol_defs);
        ext_native_symbol_len = get_ext_lib_export_apis(&ext_native_symbol_defs);

        if (base_native_symbol_len > 0)
            sort_symbol_ptr(base_native_symbol_defs, base_native_symbol_len);

        if (ext_native_symbol_len > 0)
            sort_symbol_ptr(ext_native_symbol_defs, ext_native_symbol_len);

        sort_flag = true;
    }

    if (!symbol)
        return NULL;

    if ((ret = lookup_symbol(base_native_symbol_defs, base_native_symbol_len,
                             symbol))
        || (ret = lookup_symbol(ext_native_symbol_defs, ext_native_symbol_len,
                                symbol)))
        return ret;

    return NULL;
}

