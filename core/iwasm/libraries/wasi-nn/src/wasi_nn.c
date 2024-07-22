/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#include "wasi_nn_private.h"
#include "wasi_nn_app_native.h"
#include "logger.h"

#include "bh_platform.h"
#include "wasi_nn_types.h"
#include "wasm_export.h"

#define HASHMAP_INITIAL_SIZE 20

/* Global variables */
// if using `load_by_name`, there is no known `encoding` at the time of loading
// so, just keep one `api_function` is enough
static api_function lookup = { 0 };

#define call_wasi_nn_func(wasi_error, func, ...)                  \
    do {                                                          \
        if (lookup.func) {                                        \
            wasi_error = lookup.func(__VA_ARGS__);                \
            if (wasi_error != success)                            \
                NN_ERR_PRINTF("Error %s: %d", #func, wasi_error); \
        }                                                         \
        else {                                                    \
            NN_ERR_PRINTF("Error %s is not registered", #func);   \
            wasi_error = unsupported_operation;                   \
        }                                                         \
    } while (0)

static HashMap *hashmap;

static void
wasi_nn_ctx_destroy(WASINNContext *wasi_nn_ctx);

static uint32
hash_func(const void *key)
{
    // fnv1a_hash
    const uint32 FNV_PRIME = 16777619;
    const uint32 FNV_OFFSET_BASIS = 2166136261U;

    uint32 hash = FNV_OFFSET_BASIS;
    const unsigned char *bytes = (const unsigned char *)key;

    for (size_t i = 0; i < sizeof(uintptr_t); ++i) {
        hash ^= bytes[i];
        hash *= FNV_PRIME;
    }

    return hash;
}

static bool
key_equal_func(void *key1, void *key2)
{
    return key1 == key2;
}

static void
key_destroy_func(void *key1)
{
    /* key type is wasm_module_inst_t*. do nothing */
}

static void
value_destroy_func(void *value)
{
    wasi_nn_ctx_destroy((WASINNContext *)value);
}

static WASINNContext *
wasi_nn_initialize_context()
{
    NN_DBG_PRINTF("[WASI NN] INIT...");

    WASINNContext *wasi_nn_ctx =
        (WASINNContext *)wasm_runtime_malloc(sizeof(WASINNContext));
    if (wasi_nn_ctx == NULL) {
        NN_ERR_PRINTF("Error when allocating memory for WASI-NN context");
        return NULL;
    }
    wasi_nn_ctx->is_model_loaded = false;

    /* only one backend can be registered */
    wasi_nn_error res;
    call_wasi_nn_func(res, init, &wasi_nn_ctx->backend_ctx);
    if (res != success) {
        wasm_runtime_free(wasi_nn_ctx);
        return NULL;
    }

    return wasi_nn_ctx;
}

static bool
wasi_nn_initialize()
{
    NN_DBG_PRINTF("[WASI NN General] Initializing wasi-nn");
    // hashmap { instance: wasi_nn_ctx }
    hashmap = bh_hash_map_create(HASHMAP_INITIAL_SIZE, true, hash_func,
                                 key_equal_func, key_destroy_func,
                                 value_destroy_func);
    if (hashmap == NULL) {
        NN_ERR_PRINTF("Error while initializing hashmap");
        return false;
    }
    return true;
}

/* Get wasi-nn context from module instance */
static WASINNContext *
wasm_runtime_get_wasi_nn_ctx(wasm_module_inst_t instance)
{
    WASINNContext *wasi_nn_ctx =
        (WASINNContext *)bh_hash_map_find(hashmap, (void *)instance);
    if (wasi_nn_ctx == NULL) {
        wasi_nn_ctx = wasi_nn_initialize_context();
        if (wasi_nn_ctx == NULL)
            return NULL;
        bool ok =
            bh_hash_map_insert(hashmap, (void *)instance, (void *)wasi_nn_ctx);
        if (!ok) {
            NN_ERR_PRINTF("Error while storing context");
            wasi_nn_ctx_destroy(wasi_nn_ctx);
            return NULL;
        }
    }

    return wasi_nn_ctx;
}

static void
wasi_nn_ctx_destroy(WASINNContext *wasi_nn_ctx)
{
    NN_DBG_PRINTF("[WASI NN] DEINIT...");

    if (wasi_nn_ctx == NULL) {
        NN_ERR_PRINTF(
            "Error when deallocating memory. WASI-NN context is NULL");
        return;
    }
    NN_DBG_PRINTF("Freeing wasi-nn");
    NN_DBG_PRINTF("-> is_model_loaded: %d", wasi_nn_ctx->is_model_loaded);
    NN_DBG_PRINTF("-> current_encoding: %d", wasi_nn_ctx->current_encoding);

    /* only one backend can be registered */
    wasi_nn_error res;
    call_wasi_nn_func(res, deinit, wasi_nn_ctx->backend_ctx);

    wasm_runtime_free(wasi_nn_ctx);
}

void
wasi_nn_destroy()
{
    // destroy hashmap will destroy keys and values
    bh_hash_map_destroy(hashmap);
}

/* Utils */

static wasi_nn_error
is_model_initialized(WASINNContext *wasi_nn_ctx)
{
    if (!wasi_nn_ctx->is_model_loaded) {
        NN_ERR_PRINTF("Model not initialized.");
        return runtime_error;
    }
    return success;
}

/* WASI-NN implementation */

#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
wasi_nn_error
wasi_nn_load(wasm_exec_env_t exec_env, graph_builder_wasm *builder,
             uint32_t builder_wasm_size, graph_encoding encoding,
             execution_target target, graph *g)
#else  /* WASM_ENABLE_WASI_EPHEMERAL_NN == 0 */
wasi_nn_error
wasi_nn_load(wasm_exec_env_t exec_env, graph_builder_array_wasm *builder,
             graph_encoding encoding, execution_target target, graph *g)
#endif /* WASM_ENABLE_WASI_EPHEMERAL_NN != 0 */
{
    NN_DBG_PRINTF("[WASI NN] LOAD [encoding=%d, target=%d]...", encoding,
                  target);

    wasm_module_inst_t instance = wasm_runtime_get_module_inst(exec_env);
    if (!instance)
        return runtime_error;

    wasi_nn_error res;
    graph_builder_array builder_native = { 0 };
#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
    if (success
        != (res = graph_builder_array_app_native(
                instance, builder, builder_wasm_size, &builder_native)))
        return res;
#else  /* WASM_ENABLE_WASI_EPHEMERAL_NN == 0 */
    if (success
        != (res = graph_builder_array_app_native(instance, builder,
                                                 &builder_native)))
        return res;
#endif /* WASM_ENABLE_WASI_EPHEMERAL_NN != 0 */

    if (!wasm_runtime_validate_native_addr(instance, g,
                                           (uint64)sizeof(graph))) {
        NN_ERR_PRINTF("graph is invalid");
        res = invalid_argument;
        goto fail;
    }

    WASINNContext *wasi_nn_ctx = wasm_runtime_get_wasi_nn_ctx(instance);
    call_wasi_nn_func(res, load, wasi_nn_ctx->backend_ctx, &builder_native,
                      encoding, target, g);
    if (res != success)
        goto fail;

    wasi_nn_ctx->current_encoding = encoding;
    wasi_nn_ctx->is_model_loaded = true;

fail:
    // XXX: Free intermediate structure pointers
    if (builder_native.buf)
        wasm_runtime_free(builder_native.buf);

    return res;
}

wasi_nn_error
wasi_nn_load_by_name(wasm_exec_env_t exec_env, char *name, uint32_t name_len,
                     graph *g)
{
    NN_DBG_PRINTF("[WASI NN] LOAD_BY_NAME %s...", name);

    wasm_module_inst_t instance = wasm_runtime_get_module_inst(exec_env);
    if (!instance) {
        return runtime_error;
    }

    if (!wasm_runtime_validate_native_addr(instance, name, name_len)) {
        return invalid_argument;
    }

    if (!wasm_runtime_validate_native_addr(instance, g,
                                           (uint64)sizeof(graph))) {
        return invalid_argument;
    }

    WASINNContext *wasi_nn_ctx = wasm_runtime_get_wasi_nn_ctx(instance);
    wasi_nn_error res;
    call_wasi_nn_func(res, load_by_name, wasi_nn_ctx->backend_ctx, name,
                      name_len, g);
    if (res != success)
        return res;

    wasi_nn_ctx->current_encoding = autodetect;
    wasi_nn_ctx->is_model_loaded = true;
    return success;
}

wasi_nn_error
wasi_nn_init_execution_context(wasm_exec_env_t exec_env, graph g,
                               graph_execution_context *ctx)
{
    NN_DBG_PRINTF("[WASI NN] INIT_EXECUTION_CONTEXT...");

    wasm_module_inst_t instance = wasm_runtime_get_module_inst(exec_env);
    if (!instance) {
        return runtime_error;
    }

    WASINNContext *wasi_nn_ctx = wasm_runtime_get_wasi_nn_ctx(instance);

    wasi_nn_error res;
    if (success != (res = is_model_initialized(wasi_nn_ctx)))
        return res;

    if (!wasm_runtime_validate_native_addr(
            instance, ctx, (uint64)sizeof(graph_execution_context))) {
        NN_ERR_PRINTF("ctx is invalid");
        return invalid_argument;
    }

    call_wasi_nn_func(res, init_execution_context, wasi_nn_ctx->backend_ctx, g,
                      ctx);
    return res;
}

wasi_nn_error
wasi_nn_set_input(wasm_exec_env_t exec_env, graph_execution_context ctx,
                  uint32_t index, tensor_wasm *input_tensor)
{
    NN_DBG_PRINTF("[WASI NN] SET_INPUT [ctx=%d, index=%d]...", ctx, index);

    wasm_module_inst_t instance = wasm_runtime_get_module_inst(exec_env);
    if (!instance) {
        return runtime_error;
    }

    WASINNContext *wasi_nn_ctx = wasm_runtime_get_wasi_nn_ctx(instance);

    wasi_nn_error res;
    if (success != (res = is_model_initialized(wasi_nn_ctx)))
        return res;

    tensor input_tensor_native = { 0 };
    if (success
        != (res = tensor_app_native(instance, input_tensor,
                                    &input_tensor_native)))
        return res;

    call_wasi_nn_func(res, set_input, wasi_nn_ctx->backend_ctx, ctx, index,
                      &input_tensor_native);
    // XXX: Free intermediate structure pointers
    if (input_tensor_native.dimensions)
        wasm_runtime_free(input_tensor_native.dimensions);

    return res;
}

wasi_nn_error
wasi_nn_compute(wasm_exec_env_t exec_env, graph_execution_context ctx)
{
    NN_DBG_PRINTF("[WASI NN] COMPUTE [ctx=%d]...", ctx);

    wasm_module_inst_t instance = wasm_runtime_get_module_inst(exec_env);
    if (!instance) {
        return runtime_error;
    }

    WASINNContext *wasi_nn_ctx = wasm_runtime_get_wasi_nn_ctx(instance);

    wasi_nn_error res;
    if (success != (res = is_model_initialized(wasi_nn_ctx)))
        return res;

    call_wasi_nn_func(res, compute, wasi_nn_ctx->backend_ctx, ctx);
    return res;
}

#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
wasi_nn_error
wasi_nn_get_output(wasm_exec_env_t exec_env, graph_execution_context ctx,
                   uint32_t index, tensor_data output_tensor,
                   uint32_t output_tensor_len, uint32_t *output_tensor_size)
#else  /* WASM_ENABLE_WASI_EPHEMERAL_NN == 0 */
wasi_nn_error
wasi_nn_get_output(wasm_exec_env_t exec_env, graph_execution_context ctx,
                   uint32_t index, tensor_data output_tensor,
                   uint32_t *output_tensor_size)
#endif /* WASM_ENABLE_WASI_EPHEMERAL_NN != 0 */
{
    NN_DBG_PRINTF("[WASI NN] GET_OUTPUT [ctx=%d, index=%d]...", ctx, index);

    wasm_module_inst_t instance = wasm_runtime_get_module_inst(exec_env);
    if (!instance) {
        return runtime_error;
    }

    WASINNContext *wasi_nn_ctx = wasm_runtime_get_wasi_nn_ctx(instance);

    wasi_nn_error res;
    if (success != (res = is_model_initialized(wasi_nn_ctx)))
        return res;

    if (!wasm_runtime_validate_native_addr(instance, output_tensor_size,
                                           (uint64)sizeof(uint32_t))) {
        NN_ERR_PRINTF("output_tensor_size is invalid");
        return invalid_argument;
    }

#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
    call_wasi_nn_func(res, get_output, wasi_nn_ctx->backend_ctx, ctx, index,
                      output_tensor, &output_tensor_len);
    *output_tensor_size = output_tensor_len;
#else  /* WASM_ENABLE_WASI_EPHEMERAL_NN == 0 */
    call_wasi_nn_func(res, get_output, wasi_nn_ctx->backend_ctx, ctx, index,
                      output_tensor, output_tensor_size);
#endif /* WASM_ENABLE_WASI_EPHEMERAL_NN != 0 */
    return res;
}

/* Register WASI-NN in WAMR */

/* clang-format off */
#define REG_NATIVE_FUNC(func_name, signature) \
    { #func_name, wasi_nn_##func_name, signature, NULL }
/* clang-format on */

static NativeSymbol native_symbols_wasi_nn[] = {
#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
    REG_NATIVE_FUNC(load, "(*iii*)i"),
    REG_NATIVE_FUNC(load_by_name, "(*i*)i"),
    REG_NATIVE_FUNC(init_execution_context, "(i*)i"),
    REG_NATIVE_FUNC(set_input, "(ii*)i"),
    REG_NATIVE_FUNC(compute, "(i)i"),
    REG_NATIVE_FUNC(get_output, "(ii*i*)i"),
#else  /* WASM_ENABLE_WASI_EPHEMERAL_NN == 0 */
    REG_NATIVE_FUNC(load, "(*ii*)i"),
    REG_NATIVE_FUNC(init_execution_context, "(i*)i"),
    REG_NATIVE_FUNC(set_input, "(ii*)i"),
    REG_NATIVE_FUNC(compute, "(i)i"),
    REG_NATIVE_FUNC(get_output, "(ii**)i"),
#endif /* WASM_ENABLE_WASI_EPHEMERAL_NN != 0 */
};

uint32_t
get_wasi_nn_export_apis(NativeSymbol **p_native_symbols)
{
    *p_native_symbols = native_symbols_wasi_nn;
    return sizeof(native_symbols_wasi_nn) / sizeof(NativeSymbol);
}

__attribute__((used)) uint32_t
get_native_lib(char **p_module_name, NativeSymbol **p_native_symbols)
{
    NN_DBG_PRINTF("[Native Register] get_native_lib");

#if WASM_ENABLE_WASI_EPHEMERAL_NN != 0
    *p_module_name = "wasi_ephemeral_nn";
#else  /* WASM_ENABLE_WASI_EPHEMERAL_NN == 0 */
    *p_module_name = "wasi_nn";
#endif /* WASM_ENABLE_WASI_EPHEMERAL_NN != 0 */

    return get_wasi_nn_export_apis(p_native_symbols);
}

__attribute__((used)) int
init_native_lib()
{
    NN_DBG_PRINTF("[Native Register] init_native_lib");

    if (!wasi_nn_initialize())
        return 1;

    return 0;
}

__attribute__((used)) void
deinit_native_lib()
{
    NN_DBG_PRINTF("[Native Register] deinit_native_lib");

    wasi_nn_destroy();
}

__attribute__((used)) bool
wasi_nn_register_backend(api_function apis)
{
    NN_DBG_PRINTF("[Native Register] wasi_nn_register_backend");
    lookup = apis;
    return true;
}
