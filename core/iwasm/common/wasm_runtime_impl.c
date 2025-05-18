/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_runtime_common.h"
#include "wasm_memory.h"

typedef struct WASMRuntime {
    // TODO: do we really need this?
    int32 ref_count;
    WASMRuntimeAllocator allocator;
} WASMRuntime;

/* SHOULD BE the only global variable */
static korp_key runtime_key;

static WASMRuntime *
wasm_runtime_init_prologue(void)
{
    WASMRuntimeAllocator allocator;
    bool ret;
    int ret_i;
    WASMRuntime *runtime;

    LOG_DEBUG("WASMRuntime init prologue");

    // TODO: Enable loging system
    LOG_DEBUG("WASMRuntime init setup log");

    /* Initialize memory allocator firstly */
    LOG_DEBUG("WASMRuntime init setup memory allocator");
    ret = wasm_runtime_memory_init2(Alloc_With_Pool, NULL, &allocator);
    if (!ret) {
        /* Failed to initialize memory allocator */
        LOG_ERROR("Failed to initialize memory allocator");
        return NULL;
    }

    /* Now it is able to use allocation */
    LOG_DEBUG("WASMRuntime init allocate memory for runtime");
    runtime = wasm_runtime_malloc2(&allocator, sizeof(WASMRuntime));
    if (!runtime) {
        /* Failed to allocate memory for runtime */
        LOG_ERROR("Failed to allocate memory for runtime");
        wasm_runtime_memory_destroy2(&allocator);
        return NULL;
    }

    // TODO: remove me if not needed like wasm_runtime_malloc2() will do this
    LOG_DEBUG("WASMRuntime init initialize runtime");
    memset(runtime, 0, sizeof(WASMRuntime));
    runtime->allocator = allocator;

    LOG_DEBUG("WASMRuntime init set thread local key");
    ret_i = os_thread_setspecific(runtime_key, runtime);
    if (ret_i != 0) {
        /* Failed to set thread local key */
        LOG_ERROR("Failed to set thread local key");
        wasm_runtime_free2(&allocator, runtime);
        wasm_runtime_memory_destroy2(&allocator);
        return NULL;
    }

    return runtime;
}

static void
wasm_runtime_destroy_epilogue(WASMRuntime *runtime)
{
    WASMRuntimeAllocator allocator;

    LOG_DEBUG("WASMRuntime destroy epilogue(%d)", runtime->ref_count);
    bh_assert(runtime->ref_count == 0);

    allocator = runtime->allocator;

    /* Destroy thread local key */
    LOG_DEBUG("WASMRuntime destroy thread local key");
    os_thread_key_delete(runtime_key);

    /* Free the runtime */
    LOG_DEBUG("WASMRuntime destroy memory for runtime");
    wasm_runtime_free2(&allocator, runtime);
    wasm_runtime_memory_destroy2(&allocator);
}

/* =========================== Exported Functions =========================== */

/*
 * make sure you call this function in the main thread at the beginning
 * of the program
 */
int
wasm_runtime_create_runtime_key()
{
    return os_thread_key_create(&runtime_key, wasm_runtime_destroy2);
}

WASMRuntime *
wasm_runtime_get_local_runtime()
{
    WASMRuntime *runtime;

    runtime = os_thread_getspecific(runtime_key);
    if (!runtime) {
        return NULL;
    }

    LOG_DEBUG("WASMRuntime ref_count(%d)", runtime->ref_count);
    runtime->ref_count++;
    return runtime;
}

WASMRuntime *
wasm_runtime_init2(void)
{
    WASMRuntime *runtime;
    bool ret;

    LOG_DEBUG("WASMRuntime init");

    runtime = wasm_runtime_init_prologue();
    if (!runtime) {
        /* Failed to initialize runtime */
        LOG_ERROR("Failed to prepare runtime");
        return NULL;
    }

    /*TODO: goes to wasm_runtime_full_init_internal(RuntimeInitArgs*) with
     * default args*/
    ret = wasm_runtime_env_init();
    if (!ret) {
        /* Failed to initialize runtime environment */
        LOG_ERROR("Failed to initialize runtime environment");
        wasm_runtime_destroy_epilogue(runtime);
        return NULL;
    }

    return runtime;
}

WASMRuntime *
wasm_runtime_full_init2(RuntimeInitArgs *init_args)
{
    WASMRuntime *runtime;
    bool ret;

    LOG_DEBUG("WASMRuntime full init");

    runtime = wasm_runtime_init_prologue();
    if (!runtime) {
        /* Failed to initialize runtime */
        LOG_ERROR("Failed to prepare runtime");
        return NULL;
    }

    ret = wasm_runtime_full_init_internal(init_args);
    if (!ret) {
        /* Failed to fully initialize runtime */
        LOG_ERROR("Failed to fully initialize runtime with args");
        wasm_runtime_destroy_epilogue(runtime);
        return NULL;
    }

    return runtime;
}

void
wasm_runtime_destroy2(void *data)
{
    CHECK_NULL_AND_RETURN_VOID(data);

    int ret;
    WASMRuntime *runtime;
    WASMRuntimeAllocator allocator;

    runtime = (WASMRuntime *)data;
    allocator = runtime->allocator;

    LOG_DEBUG("WASMRuntime destroy(%d)", runtime->ref_count);

    if (runtime->ref_count > 0) {
        /* Not the last one, just return */
        LOG_DEBUG("WASMRuntime ref_count(%d) > 0", runtime->ref_count);
        runtime->ref_count--;
        return;
    }

    if (runtime->ref_count < 0) {
        /* This is an error, should not happen */
        LOG_WARNING("WASMRuntime ref_count < 0");
        return;
    }

    LOG_DEBUG("WASMRuntime ref_count = 0, free it");

    wasm_runtime_destroy_internal();
    wasm_runtime_destroy_epilogue(runtime);
}

WASMRuntimeAllocator *
wasm_runtime_get_local_allocator(WASMRuntime *runtime)
{
    CHECK_NULL_AND_RETURN(runtime, NULL);
    return &runtime->allocator;
}
