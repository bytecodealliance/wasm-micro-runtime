/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_common.h"
#include "bh_log.h"
#include "wasm_export.h"
#include "../interpreter/wasm.h"
#include "../common/wasm_runtime_common.h"
#include "thread_manager.h"

#define get_module(exec_env) \
    wasm_exec_env_get_module(exec_env)

#define get_module_inst(exec_env) \
    wasm_runtime_get_module_inst(exec_env)

#define get_thread_arg(exec_env)    \
    wasm_exec_env_get_thread_arg(exec_env)

#define get_wasi_ctx(module_inst) \
    wasm_runtime_get_wasi_ctx(module_inst)

#define validate_app_addr(offset, size) \
    wasm_runtime_validate_app_addr(module_inst, offset, size)

#define validate_native_addr(addr, size) \
    wasm_runtime_validate_native_addr(module_inst, addr, size)

#define addr_app_to_native(offset) \
    wasm_runtime_addr_app_to_native(module_inst, offset)

#define addr_native_to_app(ptr) \
    wasm_runtime_addr_native_to_app(module_inst, ptr)

extern bool
wasm_runtime_call_indirect(wasm_exec_env_t exec_env,
                           uint32 element_indices,
                           uint32 argc, uint32 argv[]);

enum {
    T_THREAD,
    T_MUTEX,
    T_COND,
};

enum thread_status_t {
    THREAD_INIT,
    THREAD_RUNNING,
    THREAD_CANCELLED,
    THREAD_EXIT,
};

enum mutex_status_t {
    MUTEX_CREATED,
    MUTEX_DESTROYED,
};

enum cond_status_t {
    COND_CREATED,
    COND_DESTROYED,
};

typedef struct ClusterInfoNode {
    bh_list_link l;
    WASMCluster *cluster;
    HashMap *thread_info_map;
} ClusterInfoNode;

typedef struct ThreadInfoNode {
    wasm_exec_env_t parent_exec_env;
    wasm_exec_env_t exec_env;
    /* the id returned to app */
    uint32 handle;
    /* type can be [THREAD | MUTEX | CONDITION] */
    uint32 type;
    /* Thread status, this variable should be volatile
        as its value may be changed in different threads */
    volatile uint32 status;
    union {
        korp_tid thread;
        korp_mutex *mutex;
        korp_cond *cond;
    } u;
} ThreadInfoNode;

typedef struct {
    ThreadInfoNode *info_node;
    /* table elem index of the app's entry function */
    uint32 elem_index;
    /* arg of the app's entry function */
    void *arg;
    wasm_module_inst_t module_inst;
} ThreadRoutineArgs;

static bh_list cluster_info_list;
static korp_mutex pthread_global_lock;
static uint32 handle_id = 1;

static void
lib_pthread_destroy_callback(WASMCluster *cluster);

static uint32
thread_handle_hash(void *handle)
{
    return (uint32)(uintptr_t)handle;
}

static bool
thread_handle_equal(void *h1, void *h2)
{
    return (uint32)(uintptr_t)h1 == (uint32)(uintptr_t)h2 ? true : false;
}

static void
thread_info_destroy(void *node)
{
    ThreadInfoNode *info_node = (ThreadInfoNode *)node;
    ThreadRoutineArgs *args;

    pthread_mutex_lock(&pthread_global_lock);
    if (info_node->type == T_THREAD) {
        args = get_thread_arg(info_node->exec_env);
        if (args) {
            wasm_runtime_free(args);
        }
    }
    else if (info_node->type == T_MUTEX) {
        if (info_node->status != MUTEX_DESTROYED)
            os_mutex_destroy(info_node->u.mutex);
        wasm_runtime_free(info_node->u.mutex);
    }
    else if (info_node->type == T_COND) {
        if (info_node->status != COND_DESTROYED)
            os_cond_destroy(info_node->u.cond);
        wasm_runtime_free(info_node->u.cond);
    }
    wasm_runtime_free(info_node);
    pthread_mutex_unlock(&pthread_global_lock);
}

bool
lib_pthread_init()
{
    if (0 != os_mutex_init(&pthread_global_lock))
        return false;
    bh_list_init(&cluster_info_list);
    if (!wasm_cluster_register_destroy_callback(
            lib_pthread_destroy_callback)) {
        os_mutex_destroy(&pthread_global_lock);
        return false;
    }
    return true;
}

void
lib_pthread_destroy()
{
    os_mutex_destroy(&pthread_global_lock);
}

static ClusterInfoNode*
get_cluster_info(WASMCluster *cluster)
{
    ClusterInfoNode *node;

    os_mutex_lock(&pthread_global_lock);
    node = bh_list_first_elem(&cluster_info_list);

    while (node) {
        if (cluster == node->cluster) {
            os_mutex_unlock(&pthread_global_lock);
            return node;
        }
        node = bh_list_elem_next(node);
    }
    os_mutex_unlock(&pthread_global_lock);

    return NULL;
}

static ClusterInfoNode*
create_cluster_info(WASMCluster *cluster)
{
    ClusterInfoNode *node;
    bh_list_status ret;

    if (!(node = wasm_runtime_malloc(sizeof(ClusterInfoNode)))) {
        return NULL;
    }

    node->cluster = cluster;
    if (!(node->thread_info_map =
            bh_hash_map_create(32, true,
                               (HashFunc)thread_handle_hash,
                               (KeyEqualFunc)thread_handle_equal,
                               NULL,
                               thread_info_destroy))) {
        wasm_runtime_free(node);
        return NULL;
    }
    os_mutex_lock(&pthread_global_lock);
    ret = bh_list_insert(&cluster_info_list, node);
    bh_assert(ret == 0);
    os_mutex_unlock(&pthread_global_lock);

    (void)ret;
    return node;
}

static bool
destroy_cluster_info(WASMCluster *cluster)
{
    ClusterInfoNode *node = get_cluster_info(cluster);
    if (node) {
        bh_hash_map_destroy(node->thread_info_map);
        os_mutex_lock(&pthread_global_lock);
        bh_list_remove(&cluster_info_list, node);
        wasm_runtime_free(node);
        os_mutex_unlock(&pthread_global_lock);
        return true;
    }
    return false;
}

static void
lib_pthread_destroy_callback(WASMCluster *cluster)
{
    destroy_cluster_info(cluster);
}

static void
delete_thread_info_node(ThreadInfoNode *thread_info)
{
    ClusterInfoNode *node;
    bool ret;
    WASMCluster *cluster =
        wasm_exec_env_get_cluster(thread_info->exec_env);

    if ((node = get_cluster_info(cluster))) {
        ret = bh_hash_map_remove(node->thread_info_map,
                                 (void *)(uintptr_t)thread_info->handle,
                                 NULL, NULL);
        (void)ret;
    }

    thread_info_destroy(thread_info);
}

static bool
append_thread_info_node(ThreadInfoNode *thread_info)
{
    ClusterInfoNode *node;
    WASMCluster *cluster =
        wasm_exec_env_get_cluster(thread_info->exec_env);

    if (!(node = get_cluster_info(cluster))) {
        if (!(node = create_cluster_info(cluster))) {
            return false;
        }
    }

    if (!bh_hash_map_insert(node->thread_info_map,
                            (void *)(uintptr_t)thread_info->handle,
                            thread_info)) {
        return false;
    }

    return true;
}

static ThreadInfoNode*
get_thread_info(wasm_exec_env_t exec_env, uint32 handle)
{
    WASMCluster *cluster = wasm_exec_env_get_cluster(exec_env);
    ClusterInfoNode *info = get_cluster_info(cluster);
    return bh_hash_map_find(info->thread_info_map, (void *)(uintptr_t)handle);
}

static uint32
allocate_handle()
{
    uint32 id;
    os_mutex_lock(&pthread_global_lock);
    id = handle_id++;
    os_mutex_unlock(&pthread_global_lock);
    return id;
}

static void*
pthread_start_routine(void *arg)
{
    wasm_exec_env_t exec_env = (wasm_exec_env_t)arg;
    wasm_exec_env_t parent_exec_env;
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    ThreadRoutineArgs *routine_args = exec_env->thread_arg;
    ThreadInfoNode *info_node = routine_args->info_node;
    uint32 argv[1];

    parent_exec_env = info_node->parent_exec_env;
    os_mutex_lock(&parent_exec_env->wait_lock);
    info_node->exec_env = exec_env;
    info_node->u.thread = exec_env->handle;
    if (!append_thread_info_node(info_node)) {
        wasm_runtime_deinstantiate_internal(module_inst, true);
        delete_thread_info_node(info_node);
        os_cond_signal(&parent_exec_env->wait_cond);
        os_mutex_unlock(&parent_exec_env->wait_lock);
        return NULL;
    }

    info_node->status = THREAD_RUNNING;
    os_cond_signal(&parent_exec_env->wait_cond);
    os_mutex_unlock(&parent_exec_env->wait_lock);

    if (!validate_native_addr(routine_args->arg, sizeof(uint32))) {
        /* If there are exceptions, copy the exception to
            all other instance in this cluster */
        wasm_cluster_spread_exception(exec_env);
        wasm_runtime_deinstantiate_internal(module_inst, true);
        delete_thread_info_node(info_node);
        return NULL;
    }

    argv[0] = addr_native_to_app(routine_args->arg);

    if(!wasm_runtime_call_indirect(exec_env,
                                   routine_args->elem_index,
                                   1, argv)) {
        wasm_cluster_spread_exception(exec_env);
    }

    /* routine exit, destroy instance */
    wasm_runtime_deinstantiate_internal(module_inst, true);

    info_node->status = THREAD_EXIT;

    delete_thread_info_node(info_node);

    return (void *)(uintptr_t)argv[0];
}

static int
pthread_create_wrapper(wasm_exec_env_t exec_env,
                       uint32 *thread,      /* thread_handle */
                       const void *attr,    /* not supported */
                       uint32 elem_index,   /* entry function */
                       void *arg)           /* arguments buffer */
{
    wasm_module_t module = get_module(exec_env);
    wasm_module_inst_t new_module_inst = NULL;
    ThreadInfoNode *info_node = NULL;
    ThreadRoutineArgs *routine_args = NULL;
    uint32 thread_handle;
    int32 ret = -1;
#if WASM_ENABLE_LIBC_WASI != 0
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    WASIContext *wasi_ctx = get_wasi_ctx(module_inst);
#endif

    if (!(new_module_inst =
            wasm_runtime_instantiate_internal(module, true, 8192, 0,
                                              NULL, 0)))
        return -1;

#if WASM_ENABLE_LIBC_WASI != 0
    if (wasi_ctx)
        wasm_runtime_set_wasi_ctx(new_module_inst, wasi_ctx);
#endif

    if (!(info_node = wasm_runtime_malloc(sizeof(ThreadInfoNode))))
        goto fail;

    memset(info_node, 0, sizeof(ThreadInfoNode));
    thread_handle = allocate_handle();
    info_node->parent_exec_env = exec_env;
    info_node->handle = thread_handle;
    info_node->type = T_THREAD;
    info_node->status = THREAD_INIT;

    if (!(routine_args = wasm_runtime_malloc(sizeof(ThreadRoutineArgs))))
        goto fail;

    routine_args->arg = arg;
    routine_args->elem_index = elem_index;
    routine_args->info_node = info_node;
    routine_args->module_inst = new_module_inst;

    os_mutex_lock(&exec_env->wait_lock);
    ret = wasm_cluster_create_thread(exec_env, new_module_inst,
                                     pthread_start_routine,
                                     (void *)routine_args);
    if (ret != 0) {
        os_mutex_unlock(&exec_env->wait_lock);
        goto fail;
    }

    /* Wait for the thread routine to assign the exec_env to
        thread_info_node, otherwise the exec_env in the thread
        info node may be NULL in the next pthread API call */
    os_cond_wait(&exec_env->wait_cond, &exec_env->wait_lock);
    os_mutex_unlock(&exec_env->wait_lock);

    if (thread)
        *thread = thread_handle;

    return 0;

fail:
    if (new_module_inst)
        wasm_runtime_deinstantiate_internal(new_module_inst, true);
    if (info_node)
        wasm_runtime_free(info_node);
    if (routine_args)
        wasm_runtime_free(routine_args);
    return ret;
}

static int32
pthread_join_wrapper(wasm_exec_env_t exec_env, uint32 thread,
                     int32 retval_offset) /* void **retval */
{
    uint32 *ret;
    int32 join_ret;
    void **retval;
    ThreadInfoNode *node;
    wasm_module_inst_t module_inst;
    wasm_exec_env_t target_exec_env;

    node = get_thread_info(exec_env, thread);
    if (!node) {
        /* The thread has exited, return 0 to app */
        return 0;
    }

    target_exec_env = node->exec_env;
    bh_assert(target_exec_env != NULL);
    module_inst = get_module_inst(target_exec_env);

    /* validate addr before join thread, otherwise
        the module_inst may be freed */
    if (!validate_app_addr(retval_offset, sizeof(uint32))) {
        /* Join failed, but we don't want to terminate all threads,
            do not spread exception here */
        wasm_runtime_set_exception(module_inst, NULL);
        return -1;
    }
    retval = (void **)addr_app_to_native(retval_offset);

    join_ret = wasm_cluster_join_thread(target_exec_env, (void **)&ret);

    if (retval_offset != 0)
        *retval = (void*)ret;

    return join_ret;
}

static int32
pthread_detach_wrapper(wasm_exec_env_t exec_env, uint32 thread)
{
    ThreadInfoNode *node;
    wasm_exec_env_t target_exec_env;

    node = get_thread_info(exec_env, thread);
    if (!node)
        return 0;

    target_exec_env = node->exec_env;
    bh_assert(target_exec_env != NULL);

    return wasm_cluster_detach_thread(target_exec_env);
}

static int32
pthread_cancel_wrapper(wasm_exec_env_t exec_env, uint32 thread)
{
    ThreadInfoNode *node;
    wasm_exec_env_t target_exec_env;

    node = get_thread_info(exec_env, thread);
    if (!node)
        return 0;

    target_exec_env = node->exec_env;
    bh_assert(target_exec_env != NULL);

    return wasm_cluster_cancel_thread(target_exec_env);
}

static int32
pthread_self_wrapper(wasm_exec_env_t exec_env)
{
    ThreadRoutineArgs *args = get_thread_arg(exec_env);
    /* If thread_arg is NULL, it's the exec_env of the main thread,
        return id 0 to app */
    if (!args)
        return 0;

    return args->info_node->handle;
}

static void
pthread_exit_wrapper(wasm_exec_env_t exec_env, int32 retval_offset)
{
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    ThreadRoutineArgs *args = get_thread_arg(exec_env);
    /* Currently exit main thread is not allowed */
    if (!args)
        return;

    /* routine exit, destroy instance */
    wasm_runtime_deinstantiate_internal(module_inst, true);

    delete_thread_info_node(args->info_node);

    wasm_cluster_exit_thread(exec_env, (void *)(uintptr_t)retval_offset);
}

static int32
pthread_mutex_init_wrapper(wasm_exec_env_t exec_env, uint32 *mutex, void *attr)
{
    korp_mutex *pmutex;
    ThreadInfoNode *info_node;

    if (!(pmutex = wasm_runtime_malloc(sizeof(korp_mutex)))) {
        return -1;
    }

    if (os_mutex_init(pmutex) != 0) {
        goto fail1;
    }

    if (!(info_node = wasm_runtime_malloc(sizeof(ThreadInfoNode))))
        goto fail2;

    memset(info_node, 0, sizeof(ThreadInfoNode));
    info_node->exec_env = exec_env;
    info_node->handle = allocate_handle();
    info_node->type = T_MUTEX;
    info_node->u.mutex = pmutex;
    info_node->status = MUTEX_CREATED;

    if (!append_thread_info_node(info_node))
        goto fail3;

    /* Return the mutex handle to app */
    if (mutex)
        *(uint32*)mutex = info_node->handle;

    return 0;

fail3:
    delete_thread_info_node(info_node);
fail2:
    os_mutex_destroy(pmutex);
fail1:
    wasm_runtime_free(pmutex);

    return -1;
}

static int32
pthread_mutex_lock_wrapper(wasm_exec_env_t exec_env, uint32 *mutex)
{
    ThreadInfoNode* info_node = get_thread_info(exec_env, *mutex);
    if (!info_node || info_node->type != T_MUTEX)
        return -1;

    return os_mutex_lock(info_node->u.mutex);
}

static int32
pthread_mutex_unlock_wrapper(wasm_exec_env_t exec_env, uint32 *mutex)
{
    ThreadInfoNode* info_node = get_thread_info(exec_env, *mutex);
    if (!info_node || info_node->type != T_MUTEX)
        return -1;

    return os_mutex_unlock(info_node->u.mutex);
}

static int32
pthread_mutex_destroy_wrapper(wasm_exec_env_t exec_env, uint32 *mutex)
{
    int32 ret_val;
    ThreadInfoNode* info_node = get_thread_info(exec_env, *mutex);
    if (!info_node || info_node->type != T_MUTEX)
        return -1;

    ret_val = os_mutex_destroy(info_node->u.mutex);

    info_node->status = MUTEX_DESTROYED;
    delete_thread_info_node(info_node);

    return ret_val;
}

static int32
pthread_cond_init_wrapper(wasm_exec_env_t exec_env, uint32 *cond, void *attr)
{
    korp_cond *pcond;
    ThreadInfoNode *info_node;

    if (!(pcond = wasm_runtime_malloc(sizeof(korp_cond)))) {
        return -1;
    }

    if (os_cond_init(pcond) != 0) {
        goto fail1;
    }

    if (!(info_node = wasm_runtime_malloc(sizeof(ThreadInfoNode))))
        goto fail2;

    memset(info_node, 0, sizeof(ThreadInfoNode));
    info_node->exec_env = exec_env;
    info_node->handle = allocate_handle();
    info_node->type = T_COND;
    info_node->u.cond = pcond;
    info_node->status = COND_CREATED;

    if (!append_thread_info_node(info_node))
        goto fail3;

    /* Return the cond handle to app */
    if (cond)
        *(uint32*)cond = info_node->handle;

    return 0;

fail3:
    delete_thread_info_node(info_node);
fail2:
    os_cond_destroy(pcond);
fail1:
    wasm_runtime_free(pcond);

    return -1;
}

static int32
pthread_cond_wait_wrapper(wasm_exec_env_t exec_env, uint32 *cond, uint32 *mutex)
{
    ThreadInfoNode *cond_info_node, *mutex_info_node;

    cond_info_node = get_thread_info(exec_env, *cond);
    if (!cond_info_node || cond_info_node->type != T_COND)
        return -1;

    mutex_info_node = get_thread_info(exec_env, *mutex);
    if (!mutex_info_node || mutex_info_node->type != T_MUTEX)
        return -1;

    return os_cond_wait(cond_info_node->u.cond, mutex_info_node->u.mutex);
}

/* Currently we don't support struct timespec in built-in libc,
    so the pthread_cond_timedwait use useconds instead
*/
static int32
pthread_cond_timedwait_wrapper(wasm_exec_env_t exec_env, uint32 *cond,
                               uint32 *mutex, uint32 useconds)
{
    ThreadInfoNode *cond_info_node, *mutex_info_node;

    cond_info_node = get_thread_info(exec_env, *cond);
    if (!cond_info_node || cond_info_node->type != T_COND)
        return -1;

    mutex_info_node = get_thread_info(exec_env, *mutex);
    if (!mutex_info_node || mutex_info_node->type != T_MUTEX)
        return -1;

    return os_cond_reltimedwait(cond_info_node->u.cond,
                                mutex_info_node->u.mutex, useconds);
}

static int32
pthread_cond_signal_wrapper(wasm_exec_env_t exec_env, uint32 *cond)
{
    ThreadInfoNode* info_node = get_thread_info(exec_env, *cond);
    if (!info_node || info_node->type != T_COND)
        return -1;

    return os_cond_signal(info_node->u.cond);
}

static int32
pthread_cond_destroy_wrapper(wasm_exec_env_t exec_env, uint32 *cond)
{
    int32 ret_val;
    ThreadInfoNode* info_node = get_thread_info(exec_env, *cond);
    if (!info_node || info_node->type != T_COND)
        return -1;

    ret_val = os_cond_destroy(info_node->u.cond);

    info_node->status = COND_DESTROYED;
    delete_thread_info_node(info_node);

    return ret_val;
}

#define REG_NATIVE_FUNC(func_name, signature)  \
    { #func_name, func_name##_wrapper, signature, NULL }

static NativeSymbol native_symbols_lib_pthread[] = {
    REG_NATIVE_FUNC(pthread_create,         "(**i*)i"),
    REG_NATIVE_FUNC(pthread_join,           "(ii)i"),
    REG_NATIVE_FUNC(pthread_detach,         "(i)i"),
    REG_NATIVE_FUNC(pthread_cancel,         "(i)i"),
    REG_NATIVE_FUNC(pthread_self,           "()i"),
    REG_NATIVE_FUNC(pthread_exit,           "(i)"),
    REG_NATIVE_FUNC(pthread_mutex_init,     "(**)i"),
    REG_NATIVE_FUNC(pthread_mutex_lock,     "(*)i"),
    REG_NATIVE_FUNC(pthread_mutex_unlock,   "(*)i"),
    REG_NATIVE_FUNC(pthread_mutex_destroy,  "(*)i"),
    REG_NATIVE_FUNC(pthread_cond_init,      "(**)i"),
    REG_NATIVE_FUNC(pthread_cond_wait,      "(**)i"),
    REG_NATIVE_FUNC(pthread_cond_timedwait, "(**i)i"),
    REG_NATIVE_FUNC(pthread_cond_signal,    "(*)i"),
    REG_NATIVE_FUNC(pthread_cond_destroy,   "(*)i"),
};

uint32
get_lib_pthread_export_apis(NativeSymbol **p_lib_pthread_apis)
{
    *p_lib_pthread_apis = native_symbols_lib_pthread;
    return sizeof(native_symbols_lib_pthread) / sizeof(NativeSymbol);
}