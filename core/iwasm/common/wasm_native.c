/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_native.h"
#include "wasm_runtime_common.h"
#include "bh_log.h"

#if !defined(BH_PLATFORM_ZEPHYR) && !defined(BH_PLATFORM_ALIOS_THINGS) \
    && !defined(BH_PLATFORM_OPENRTOS) && !defined(BH_PLATFORM_ESP_IDF)
#define ENABLE_QUICKSORT 1
#else
#define ENABLE_QUICKSORT 0
#endif

#define ENABLE_SORT_DEBUG 0

#if ENABLE_SORT_DEBUG != 0
#include <sys/time.h>
#endif

static NativeSymbolsVec g_native_symbols_vec = NULL;
static uint32 g_native_libs_count = 0;
static uint32 g_native_libs_size = 0;

enum {
#if WASM_ENABLE_LIBC_BUILTIN != 0
    ID_LIBC_BUILTIN = 0,
#endif
#if WASM_ENABLE_SPEC_TEST
    ID_SPECTEST,
#endif
#if WASM_ENABLE_LIBC_WASI != 0
    ID_LIBC_WASI_UNSTABLE,
    ID_LIBC_WASI_PREVIEW1,
#endif
#if WASM_ENABLE_BASE_LIB != 0
    ID_BASE_LIB,
#endif
#if WASM_ENABLE_APP_FRAMEWORK != 0
    ID_APP_FRAME,
#endif
#if WASM_ENABLE_LIB_PTHREAD != 0
    ID_PTHREAD,
#endif
#if WASM_ENABLE_LIBC_EMCC != 0
    ID_LIBC_EMCC,
#endif
    ID_USER,
    NODE_COUNT
};

uint32
get_libc_builtin_export_apis(NativeSymbol **p_libc_builtin_apis);

#if WASM_ENABLE_SPEC_TEST != 0
uint32
get_spectest_export_apis(NativeSymbol **p_libc_builtin_apis);
#endif

uint32
get_libc_wasi_export_apis(NativeSymbol **p_libc_wasi_apis);

uint32
get_base_lib_export_apis(NativeSymbol **p_base_lib_apis);

uint32
get_ext_lib_export_apis(NativeSymbol **p_ext_lib_apis);

#if WASM_ENABLE_LIB_PTHREAD != 0
bool
lib_pthread_init();

void
lib_pthread_destroy();

uint32
get_lib_pthread_export_apis(NativeSymbol **p_lib_pthread_apis);
#endif

uint32
get_libc_emcc_export_apis(NativeSymbol **p_libc_emcc_apis);

bool
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

    if ((uint32)(p_end - p) < (uint32)(type->param_count + 1))
        /* signatures of parameters, and ')' */
        return false;

    for (i = 0; i < type->param_count; i++) {
        sig = *p++;
        if ((type->types[i] >= VALUE_TYPE_F64
             && type->types[i] <= VALUE_TYPE_I32
             && sig == sig_map[type->types[i] - VALUE_TYPE_F64])
#if WASM_ENABLE_REF_TYPES != 0
            || (sig == 'i' && type->types[i] == VALUE_TYPE_EXTERNREF)
#endif
        )
            /* normal parameter */
            continue;

        if (type->types[i] != VALUE_TYPE_I32)
            /* pointer and string must be i32 type */
            return false;

        if (sig == '*') {
            /* it is a pointer */
            if (i + 1 < type->param_count
                && type->types[i + 1] == VALUE_TYPE_I32 && *p == '~') {
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
/*
#if ENABLE_QUICKSORT == 0
static void
sort_symbol_ptr(NativeSymbol *native_symbols, uint32 n_native_symbols)
{
    uint32 i, j;
    NativeSymbol temp;

    for (i = 0; i < n_native_symbols - 1; i++) {
        for (j = i + 1; j < n_native_symbols; j++) {
            if (strcmp(native_symbols[i].symbol, native_symbols[j].symbol)
                > 0) {
                temp = native_symbols[i];
                native_symbols[i] = native_symbols[j];
                native_symbols[j] = temp;
            }
        }
    }
}
#else
static void
swap_symbol(NativeSymbol *left, NativeSymbol *right)
{
    NativeSymbol temp = *left;
    *left = *right;
    *right = temp;
}

static void
quick_sort_symbols(NativeSymbol *native_symbols, int left, int right)
{
    NativeSymbol base_symbol;
    int pin_left = left;
    int pin_right = right;

    if (left >= right) {
        return;
    }

    base_symbol = native_symbols[left];
    while (left < right) {
        while (left < right
               && strcmp(native_symbols[right].symbol, base_symbol.symbol)
                      > 0) {
            right--;
        }

        if (left < right) {
            swap_symbol(&native_symbols[left], &native_symbols[right]);
            left++;
        }

        while (left < right
               && strcmp(native_symbols[left].symbol, base_symbol.symbol) < 0) {
            left++;
        }

        if (left < right) {
            swap_symbol(&native_symbols[left], &native_symbols[right]);
            right--;
        }
    }
    native_symbols[left] = base_symbol;

    quick_sort_symbols(native_symbols, pin_left, left - 1);
    quick_sort_symbols(native_symbols, left + 1, pin_right);
}
#endif
*/
static void *
lookup_symbol(NativeSymbol *native_symbols, uint32 n_native_symbols,
              const ConstStrDescription *symbol, const char **p_signature, void **p_attachment)
{/*
    int low = 0, mid, ret;
    int high = (int32)n_native_symbols - 1;

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
*/
    uint32 i = 0;
    for (i = 0; i < n_native_symbols; i ++) {
        NativeSymbol * native_symbol = &native_symbols[i];
        if (native_symbol->u.symbol == symbol) {
            *p_signature = native_symbol->signature;
            *p_attachment = native_symbol->attachment;
            return native_symbol->func_ptr;
        }
    }

    return NULL;
}

void *
wasm_native_resolve_symbol(const ConstStrDescription *module_name,
                            const ConstStrDescription ** p_field_name,
                            const WASMType *func_type, const char **p_signature,
                            void **p_attachment, bool *p_call_conv_raw)
{
    WASMRuntime * runtime = wasm_runtime_get_runtime();
    NativeSymbolsNode *node = NULL;
    const char *signature = NULL;
    const ConstStrDescription * field_name = *p_field_name;
    void *func_ptr = NULL, *attachment;
    uint32 field_id =0, first_sym_id = 0, symbol_index = 0;
    bool pass_check = false;

    if (wasm_runtime_is_system_symbol(runtime, module_name) &&
        wasm_runtime_is_system_symbol(runtime, field_name)) {
        for (uint32 i = 0; i < ID_USER; i ++) {
            node = &g_native_symbols_vec[i];
            if (node->module_name == module_name) {
                field_id = wasm_runtime_get_syssymbol_id(runtime, field_name);
                first_sym_id = wasm_runtime_get_syssymbol_id(runtime, node->native_symbols[0].u.symbol);

                if (field_id < first_sym_id + node->n_native_symbols) {
                    symbol_index = field_id - first_sym_id;
                    func_ptr = node->native_symbols[symbol_index].func_ptr;
                    signature = node->native_symbols[symbol_index].signature;
                    attachment = node->native_symbols[symbol_index].attachment;
                    break;
                }
            }
        }
    } else {
        for (uint32 i = ID_USER; i < g_native_libs_count; i ++) {
            node = &g_native_symbols_vec[i];
            if (node->module_name == module_name) {
                func_ptr =
                    lookup_symbol(node->native_symbols, node->n_native_symbols,
                                   field_name, &signature, &attachment);

                if (func_ptr) {
                    break;
                }

                if (field_name->str[0] == '_') {
                    const ConstStrDescription * new_field_name =
                        wasm_runtime_records_const_string(runtime, &field_name->str[1], field_name->len - 1, NULL, 0);
                    func_ptr =
                        lookup_symbol(node->native_symbols, node->n_native_symbols,
                                   new_field_name, &signature, &attachment);

                    if (func_ptr)
                        break;
                }
            }
        }
    }

#if 0
    node = g_native_symbols_list;
    while (node) {
        node_next = node->next;
        if (!strcmp(node->module_name, module_name)) {
            if ((func_ptr =
                     lookup_symbol(node->native_symbols, node->n_native_symbols,
                                   field_name, &signature, &attachment))
                || (field_name[0] == '_'
                    && (func_ptr = lookup_symbol(
                            node->native_symbols, node->n_native_symbols,
                            field_name + 1, &signature, &attachment))))
                break;
        }
        node = node_next;
    }
#endif

    if (func_ptr) {
        if (signature && signature[0] != '\0') {
            /* signature is not empty, check its format */
            if (!check_symbol_signature(func_type, signature)) {
                // hotfix abort() if launching AS module
                if (symbol_index && node->native_symbols[++symbol_index].u.symbol == field_name) {
                    signature = node->native_symbols[symbol_index].signature;
                    if (check_symbol_signature(func_type, signature)) {
                        *p_signature = signature;
                        func_ptr = node->native_symbols[symbol_index].func_ptr;
                        attachment = node->native_symbols[symbol_index].attachment;
                        pass_check = true;
                    }
                }

                if (!pass_check) {
#if WASM_ENABLE_WAMR_COMPILER == 0
                    /* Output warning except running aot compiler */
                    LOG_WARNING("failed to check signature '%s' and resolve "
                                "pointer params for import function (%s %s)\n",
                                signature, module_name, field_name);
#endif
                    return NULL;
                }
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
register_natives(const char *module_name, NativeSymbol *native_symbols,
                 uint32 n_native_symbols, bool call_conv_raw)
{
    WASMRuntime * runtime = wasm_runtime_get_runtime();
/*
#if ENABLE_SORT_DEBUG != 0
    struct timeval start;
    struct timeval end;
    unsigned long timer;
#endif
*/
    if (!runtime)
        return false;

    if ((g_native_libs_count + 1) > g_native_libs_size) {
        g_native_symbols_vec = wasm_runtime_realloc(
                g_native_symbols_vec, (g_native_libs_size + 2) * sizeof(NativeSymbolsNode));
        if (!g_native_symbols_vec) {
            return false;
        }

        g_native_libs_size += 2;
        bh_assert((g_native_libs_count + 2) == g_native_libs_size);
    }

    const ConstStrDescription * csp_module_name =
        wasm_runtime_records_const_string(runtime, module_name, strlen(module_name), NULL, 0);

    g_native_symbols_vec[g_native_libs_count].module_name = csp_module_name;
    g_native_symbols_vec[g_native_libs_count].native_symbols = NULL;
    g_native_symbols_vec[g_native_libs_count].n_native_symbols = n_native_symbols;
    g_native_symbols_vec[g_native_libs_count].call_conv_raw = call_conv_raw;

    NativeSymbol * csp_native_symbols = (NativeSymbol*)wasm_runtime_malloc(sizeof(NativeSymbol) * n_native_symbols);
    if (!csp_native_symbols) {
        return false;
    }

    for (uint32 i = 0; i < n_native_symbols; i++) {
        memcpy(&csp_native_symbols[i], &native_symbols[i], sizeof(NativeSymbol));
        csp_native_symbols[i].u.symbol =
            wasm_runtime_records_const_string(runtime, native_symbols[i].u.symbol_str,
                                        strlen(native_symbols[i].u.symbol_str), NULL, 0);
    }

    g_native_symbols_vec[g_native_libs_count].native_symbols = csp_native_symbols;

    g_native_libs_count ++;

/*
#if ENABLE_SORT_DEBUG != 0
    gettimeofday(&start, NULL);
#endif

#if ENABLE_QUICKSORT == 0
    sort_symbol_ptr(native_symbols, n_native_symbols);
#else
    quick_sort_symbols(native_symbols, 0, (int)(n_native_symbols - 1));
#endif

#if ENABLE_SORT_DEBUG != 0
    gettimeofday(&end, NULL);
    timer =
        1000000 * (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec);
    LOG_ERROR("module_name: %s, nums: %d, sorted used: %ld us", module_name,
              n_native_symbols, timer);
#endif
*/
    return true;
}

bool
wasm_native_register_natives(const char *module_name,
                             NativeSymbol *native_symbols,
                             uint32 n_native_symbols)
{
    return register_natives(module_name, native_symbols, n_native_symbols,
                            false);
}

bool
wasm_native_register_natives_raw(const char *module_name,
                                 NativeSymbol *native_symbols,
                                 uint32 n_native_symbols)
{
    return register_natives(module_name, native_symbols, n_native_symbols,
                            true);
}

bool
wasm_native_init()
{
    WASMRuntime * runtime = wasm_runtime_get_runtime();
    NativeSymbol *native_symbols;
    uint32 n_native_symbols = 0;
    uint32 n_native_nodes = g_native_libs_size = NODE_COUNT;
    g_native_libs_count = NODE_COUNT - 1;

    if (!runtime)
        return false;

    if (n_native_nodes) {
        g_native_symbols_vec = (NativeSymbolsNode*)wasm_runtime_malloc(
                                sizeof(NativeSymbolsNode) * n_native_nodes);
        if (!g_native_symbols_vec)
            return false;

        memset(g_native_symbols_vec, 0, sizeof(NativeSymbolsNode) * n_native_nodes);
    }

#if WASM_ENABLE_LIBC_BUILTIN != 0
    n_native_symbols = get_libc_builtin_export_apis(&native_symbols);
    //if (!wasm_native_register_natives(CONST_STR_POOL_DESC(WAMR_CSP_env), native_symbols, n_native_symbols))
    //    return false;
    if (n_native_symbols) {
        for (uint32 i = 0; i < n_native_symbols; i ++)
            native_symbols[i].u.symbol = CONST_STR_POOL_DESC(runtime, native_symbols[i].u.symbol_key);

        g_native_symbols_vec[ID_LIBC_BUILTIN].module_name = CONST_STR_POOL_DESC(runtime, WAMR_CSP_env);
        g_native_symbols_vec[ID_LIBC_BUILTIN].native_symbols = native_symbols;
        g_native_symbols_vec[ID_LIBC_BUILTIN].n_native_symbols = n_native_symbols;
        g_native_symbols_vec[ID_LIBC_BUILTIN].call_conv_raw = false;
    }
#endif

#if WASM_ENABLE_SPEC_TEST != 0
    n_native_symbols = get_spectest_export_apis(&native_symbols);
    if (n_native_symbols) {
        for (uint32 i = 0; i < n_native_symbols; i ++)
            native_symbols[i].u.symbol = CONST_STR_POOL_DESC(runtime, native_symbols[i].u.symbol_key);

        g_native_symbols_vec[ID_SPECTEST].module_name = CONST_STR_POOL_DESC(runtime, WAMR_CSP_spectest);
        g_native_symbols_vec[ID_SPECTEST].native_symbols = native_symbols;
        g_native_symbols_vec[ID_SPECTEST].n_native_symbols = n_native_symbols;
        g_native_symbols_vec[ID_SPECTEST].call_conv_raw = false;
    }
#endif

#if WASM_ENABLE_LIBC_WASI != 0
    n_native_symbols = get_libc_wasi_export_apis(&native_symbols);
    if (n_native_symbols) {
        for (uint32 i = 0; i < n_native_symbols; i ++)
            native_symbols[i].u.symbol = CONST_STR_POOL_DESC(runtime, native_symbols[i].u.symbol_key);

        g_native_symbols_vec[ID_LIBC_WASI_UNSTABLE].module_name = CONST_STR_POOL_DESC(runtime, WAMR_CSP_wasi_unstable);
        g_native_symbols_vec[ID_LIBC_WASI_UNSTABLE].native_symbols = native_symbols;
        g_native_symbols_vec[ID_LIBC_WASI_UNSTABLE].n_native_symbols = n_native_symbols;
        g_native_symbols_vec[ID_LIBC_WASI_UNSTABLE].call_conv_raw = false;

        g_native_symbols_vec[ID_LIBC_WASI_PREVIEW1].module_name = CONST_STR_POOL_DESC(runtime, WAMR_CSP_wasi_snapshot_preview1);
        g_native_symbols_vec[ID_LIBC_WASI_PREVIEW1].native_symbols = native_symbols;
        g_native_symbols_vec[ID_LIBC_WASI_PREVIEW1].n_native_symbols = n_native_symbols;
        g_native_symbols_vec[ID_LIBC_WASI_PREVIEW1].call_conv_raw = false;
    }
#endif

#if WASM_ENABLE_BASE_LIB != 0
    n_native_symbols = get_base_lib_export_apis(&native_symbols);
    if (n_native_symbols) {
        for (uint32 i = 0; i < n_native_symbols; i ++)
            native_symbols[i].u.symbol = CONST_STR_POOL_DESC(runtime, native_symbols[i].u.symbol_key);

        g_native_symbols_vec[ID_BASE_LIB].module_name = CONST_STR_POOL_DESC(runtime, WAMR_CSP_env);
        g_native_symbols_vec[ID_BASE_LIB].native_symbols = native_symbols;
        g_native_symbols_vec[ID_BASE_LIB].n_native_symbols = n_native_symbols;
        g_native_symbols_vec[ID_BASE_LIB].call_conv_raw = false;
    }
#endif

#if WASM_ENABLE_APP_FRAMEWORK != 0
    n_native_symbols = get_ext_lib_export_apis(&native_symbols);
    if (n_native_symbols) {
        for (uint32 i = 0; i < n_native_symbols; i ++)
            native_symbols[i].u.symbol = CONST_STR_POOL_DESC(runtime, native_symbols[i].u.symbol_key);

        g_native_symbols_vec[ID_APP_FRAME].module_name = CONST_STR_POOL_DESC(runtime, WAMR_CSP_env);
        g_native_symbols_vec[ID_APP_FRAME].native_symbols = native_symbols;
        g_native_symbols_vec[ID_APP_FRAME].n_native_symbols = n_native_symbols;
        g_native_symbols_vec[ID_APP_FRAME].call_conv_raw = false;
    }
#endif

#if WASM_ENABLE_LIB_PTHREAD != 0
    if (!lib_pthread_init())
        return false;

    n_native_symbols = get_lib_pthread_export_apis(&native_symbols);
    if (n_native_symbols) {
        for (uint32 i = 0; i < n_native_symbols; i ++)
            native_symbols[i].u.symbol = CONST_STR_POOL_DESC(runtime, native_symbols[i].u.symbol_key);

        g_native_symbols_vec[ID_PTHREAD].module_name = CONST_STR_POOL_DESC(runtime, WAMR_CSP_env);
        g_native_symbols_vec[ID_PTHREAD].native_symbols = native_symbols;
        g_native_symbols_vec[ID_PTHREAD].n_native_symbols = n_native_symbols;
        g_native_symbols_vec[ID_PTHREAD].call_conv_raw = false;
    }
#endif

#if WASM_ENABLE_LIBC_EMCC != 0
    n_native_symbols = get_libc_emcc_export_apis(&native_symbols);
    if (n_native_symbols) {
        for (uint32 i = 0; i < n_native_symbols; i ++)
            native_symbols[i].u.symbol = CONST_STR_POOL_DESC(runtime, native_symbols[i].u.symbol_key);

        g_native_symbols_vec[ID_LIBC_EMCC].module_name = CONST_STR_POOL_DESC(runtime, WAMR_CSP_env);
        g_native_symbols_vec[ID_LIBC_EMCC].native_symbols = native_symbols;
        g_native_symbols_vec[ID_LIBC_EMCC].n_native_symbols = n_native_symbols;
        g_native_symbols_vec[ID_LIBC_EMCC].call_conv_raw = false;
    }
#endif

    return true;
}

void
wasm_native_destroy()
{
#if WASM_ENABLE_LIB_PTHREAD != 0
    lib_pthread_destroy();
#endif

    for (uint32 i = ID_USER; i < g_native_libs_count; i ++) {
        if (g_native_symbols_vec[i].native_symbols)
            wasm_runtime_free(g_native_symbols_vec[i].native_symbols);
    }

    if (g_native_symbols_vec)
        wasm_runtime_free(g_native_symbols_vec);

    g_native_symbols_vec = NULL;
}
