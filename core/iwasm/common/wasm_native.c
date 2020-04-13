/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_native.h"
#include "wasm_runtime_common.h"
#include "bh_log.h"


static NativeSymbolsList g_native_symbols_list = NULL;
static NativeSymbolsList g_native_symbols_list_end = NULL;

uint32
get_libc_builtin_export_apis(NativeSymbol **p_libc_builtin_apis);

uint32
get_spectest_export_apis(NativeSymbol **p_libc_builtin_apis);

uint32
get_libc_wasi_export_apis(NativeSymbol **p_libc_wasi_apis);

uint32
get_base_lib_export_apis(NativeSymbol **p_base_lib_apis);

uint32
get_ext_lib_export_apis(NativeSymbol **p_ext_lib_apis);

static bool
check_symbol_signature(const WASMType *type, const char *signature)
{
    const char *p = signature, *p_end;
    char sig_map[] = { 'F', 'f', 'I', 'i' }, sig;
    uint32 i = 0;

    if (!p || strlen(p) < 2)
        return false;

    p_end = p + strlen(signature);

    if (*p++ != '(')
        return false;

    if ((uint32)(p_end - p) < type->param_count + 1)
        /* signatures of parameters, and ')' */
        return false;

    for (i = 0; i < type->param_count; i++) {
        sig = *p++;
        if (sig == sig_map[type->types[i] - VALUE_TYPE_F64])
            /* normal parameter */
            continue;

        if (type->types[i] != VALUE_TYPE_I32)
            /* pointer and string must be i32 type */
            return false;

        if (sig == '*') {
            /* it is a pointer */
            if (i + 1 < type->param_count
                && type->types[i + 1] == VALUE_TYPE_I32
                && *p == '~') {
                /* pointer length followed */
                i++;
                p++;
            }
        }
        else if (sig == '$') {
            /* it is a string */
        }
        else {
            /* invalid signature */
            return false;
        }
    }

    if (*p++ != ')')
        return false;

    if (type->result_count) {
        if (p >= p_end)
            return false;
        if (*p++ != sig_map[type->types[i] - VALUE_TYPE_F64])
            return false;
    }

    if (*p != '\0')
        return false;

    return true;
}

static void
sort_symbol_ptr(NativeSymbol *native_symbols, uint32 n_native_symbols)
{
    uint32 i, j;
    NativeSymbol temp;

    for (i = 0; i < n_native_symbols - 1; i++) {
        for (j = i + 1; j < n_native_symbols; j++) {
            if (strcmp(native_symbols[i].symbol,
                       native_symbols[j].symbol) > 0) {
                temp = native_symbols[i];
                native_symbols[i] = native_symbols[j];
                native_symbols[j] = temp;
            }
        }
    }
}

static void *
lookup_symbol(NativeSymbol *native_symbols, uint32 n_native_symbols,
              const char *symbol, const char **p_signature, void **p_attachment)
{
    int low = 0, mid, ret;
    int high = n_native_symbols - 1;

    while (low <= high) {
        mid = (low + high) / 2;
        ret = strcmp(symbol, native_symbols[mid].symbol);
        if (ret == 0) {
            *p_signature = native_symbols[mid].signature;
            *p_attachment = native_symbols[mid].attachment;
            return native_symbols[mid].func_ptr;
        }
        else if (ret < 0)
            high = mid - 1;
        else
            low = mid + 1;
    }

    return NULL;
}

void*
wasm_native_resolve_symbol(const char *module_name, const char *field_name,
                           const WASMType *func_type, const char **p_signature,
                           void **p_attachment, bool *p_call_conv_raw)
{
    NativeSymbolsNode *node, *node_next;
    const char *signature = NULL;
    void *func_ptr = NULL, *attachment;

    node = g_native_symbols_list;
    while (node) {
        node_next = node->next;
        if (!strcmp(node->module_name, module_name)) {
            if ((func_ptr = lookup_symbol(node->native_symbols,
                                          node->n_native_symbols,
                                          field_name, &signature, &attachment))
                || (field_name[0] == '_'
                    && (func_ptr = lookup_symbol(node->native_symbols,
                                                 node->n_native_symbols,
                                                 field_name + 1,
                                                 &signature, &attachment))))
            break;
        }
        node = node_next;
    }

    if (func_ptr) {
        if (signature && signature[0] != '\0') {
            /* signature is not empty, check its format */
            if (!check_symbol_signature(func_type, signature)) {
#if WASM_ENABLE_WAMR_COMPILER == 0 /* Output warning except running aot compiler */
                LOG_WARNING("failed to check signature '%s' and resolve "
                            "pointer params for import function (%s %s)\n",
                            signature, module_name, field_name);
#endif
                return NULL;
            }
            else
                /* Save signature for runtime to do pointer check and
                   address conversion */
                *p_signature = signature;
        }
        else
            /* signature is empty */
            *p_signature = NULL;

        *p_attachment = attachment;
        *p_call_conv_raw = node->call_conv_raw;
    }

    return func_ptr;
}

static bool
register_natives(const char *module_name,
                 NativeSymbol *native_symbols,
                 uint32 n_native_symbols,
                 bool call_conv_raw)
{
    NativeSymbolsNode *node;

    if (!(node = wasm_runtime_malloc(sizeof(NativeSymbolsNode))))
        return false;

    node->module_name = module_name;
    node->native_symbols = native_symbols;
    node->n_native_symbols = n_native_symbols;
    node->call_conv_raw = call_conv_raw;
    node->next = NULL;

    if (g_native_symbols_list_end) {
        g_native_symbols_list_end->next = node;
        g_native_symbols_list_end = node;
    }
    else {
        g_native_symbols_list = g_native_symbols_list_end = node;
    }

    sort_symbol_ptr(native_symbols, n_native_symbols);
    return true;
}

bool
wasm_native_register_natives(const char *module_name,
                             NativeSymbol *native_symbols,
                             uint32 n_native_symbols)
{
    return register_natives(module_name, native_symbols, n_native_symbols, false);
}

bool
wasm_native_register_natives_raw(const char *module_name,
                                 NativeSymbol *native_symbols,
                                 uint32 n_native_symbols)
{
    return register_natives(module_name, native_symbols, n_native_symbols, true);
}

bool
wasm_native_init()
{
    NativeSymbol *native_symbols;
    uint32 n_native_symbols;

#if WASM_ENABLE_LIBC_BUILTIN != 0
    n_native_symbols = get_libc_builtin_export_apis(&native_symbols);
    if (!wasm_native_register_natives("env",
                                       native_symbols, n_native_symbols))
        return false;

    n_native_symbols = get_spectest_export_apis(&native_symbols);
    if (!wasm_native_register_natives("spectest",
                                       native_symbols, n_native_symbols))
        return false;
#endif

#if WASM_ENABLE_LIBC_WASI != 0
    n_native_symbols = get_libc_wasi_export_apis(&native_symbols);
    if (!wasm_native_register_natives("wasi_unstable",
                                      native_symbols, n_native_symbols))
        return false;
    if (!wasm_native_register_natives("wasi_snapshot_preview1",
                                      native_symbols, n_native_symbols))
        return false;
#endif

#if WASM_ENABLE_BASE_LIB != 0
    n_native_symbols = get_base_lib_export_apis(&native_symbols);
    if (n_native_symbols > 0
        && !wasm_native_register_natives("env",
                                         native_symbols, n_native_symbols))
        return false;
#endif

#if WASM_ENABLE_APP_FRAMEWORK != 0
    n_native_symbols = get_ext_lib_export_apis(&native_symbols);
    if (n_native_symbols > 0
        && !wasm_native_register_natives("env",
                                         native_symbols, n_native_symbols))
        return false;
#endif

    return true;
}

void
wasm_native_destroy()
{
    NativeSymbolsNode *node, *node_next;

    node = g_native_symbols_list;
    while (node) {
        node_next = node->next;
        wasm_runtime_free(node);
        node = node_next;
    }

    g_native_symbols_list = g_native_symbols_list_end = NULL;
}

