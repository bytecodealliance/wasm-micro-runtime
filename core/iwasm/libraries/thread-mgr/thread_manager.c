/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "thread_manager.h"

typedef struct {
    bh_list_link l;

    void (*destroy_cb)(WASMCluster *);
} DestroyCallBackNode;

static bh_list destroy_callback_list_head;
static bh_list *const destroy_callback_list = &destroy_callback_list_head;

static bh_list cluster_list_head;
static bh_list *const cluster_list = &cluster_list_head;
static korp_mutex cluster_list_lock;

typedef void (*list_visitor)(void *, void *);

static uint32 cluster_max_thread_num = CLUSTER_MAX_THREAD_NUM;

/* Set the maximum thread number, if this function is not called,
    the max thread num is defined by CLUSTER_MAX_THREAD_NUM */
void
wasm_cluster_set_max_thread_num(uint32 num)
{
    cluster_max_thread_num = num;
}

bool
thread_manager_init()
{
    if (bh_list_init(cluster_list) != 0)
        return false;
    if (os_mutex_init(&cluster_list_lock) != 0)
        return false;
    return true;
}

void
thread_manager_destroy()
{
    WASMCluster *cluster = bh_list_first_elem(cluster_list);
    WASMCluster *next;
    while (cluster) {
        next = bh_list_elem_next(cluster);
        wasm_cluster_destroy(cluster);
        cluster = next;
    }
    wasm_cluster_cancel_all_callbacks();
    os_mutex_destroy(&cluster_list_lock);
}

static void
traverse_list(bh_list *l, list_visitor visitor, void *user_data)
{
    void *next, *node = bh_list_first_elem(l);
    while (node) {
        next = bh_list_elem_next(node);
        visitor(node, user_data);
        node = next;
    }
}

static bool
allocate_aux_stack(WASMCluster *cluster, uint32 *start, uint32 *size)
{
    uint32 i;

    /* If the module doesn't have aux stack info,
        it can't create any threads */
    if (!cluster->stack_segment_occupied)
        return false;

    os_mutex_lock(&cluster->lock);
    for (i = 0; i < cluster_max_thread_num; i++) {
        if (!cluster->stack_segment_occupied[i]) {
            if (start)
                *start = cluster->stack_tops[i];
            if (size)
                *size = cluster->stack_size;
            cluster->stack_segment_occupied[i] = true;
            os_mutex_unlock(&cluster->lock);
            return true;
        }
    }
    os_mutex_unlock(&cluster->lock);
    return false;
}

static bool
free_aux_stack(WASMCluster *cluster, uint32 start)
{
    uint32 i;

    for (i = 0; i < cluster_max_thread_num; i++) {
        if (start == cluster->stack_tops[i]) {
            os_mutex_lock(&cluster->lock);
            cluster->stack_segment_occupied[i] = false;
            os_mutex_unlock(&cluster->lock);
            return true;
        }
    }
    return false;
}

WASMCluster *
wasm_cluster_create(WASMExecEnv *exec_env)
{
    WASMCluster *cluster;
    uint64 total_size;
    uint32 aux_stack_start, aux_stack_size, i;

    bh_assert(exec_env->cluster == NULL);
    if (!(cluster = wasm_runtime_malloc(sizeof(WASMCluster)))) {
        LOG_ERROR("thread manager error: failed to allocate memory");
        return NULL;
    }
    memset(cluster, 0, sizeof(WASMCluster));

    exec_env->cluster = cluster;

    bh_list_init(&cluster->exec_env_list);
    bh_list_insert(&cluster->exec_env_list, exec_env);
    if (os_mutex_init(&cluster->lock) != 0) {
        wasm_runtime_free(cluster);
        LOG_ERROR("thread manager error: failed to init mutex");
        return NULL;
    }

    /* Prepare the aux stack top and size for every thread */
    if (!wasm_exec_env_get_aux_stack(exec_env,
                                     &aux_stack_start,
                                     &aux_stack_size)) {
        LOG_VERBOSE("No aux stack info for this module, can't create thread");

        /* If the module don't have aux stack info, don't throw error here,
            but remain stack_tops and stack_segment_occupied as NULL */
        os_mutex_lock(&cluster_list_lock);
        if (bh_list_insert(cluster_list, cluster) != 0) {
            os_mutex_unlock(&cluster_list_lock);
            goto fail;
        }
        os_mutex_unlock(&cluster_list_lock);

        return cluster;
    }

    cluster->stack_size = aux_stack_size / (cluster_max_thread_num + 1);
    if (cluster->stack_size == 0) {
        goto fail;
    }

    /* Set initial aux stack top to the instance and
        aux stack boundary to the main exec_env */
    if (!wasm_exec_env_set_aux_stack(exec_env, aux_stack_start,
                                     cluster->stack_size))
        goto fail;

    if (cluster_max_thread_num != 0) {
        total_size = cluster_max_thread_num * sizeof(uint32);
        if (total_size >= UINT32_MAX
            || !(cluster->stack_tops =
                        wasm_runtime_malloc((uint32)total_size))) {
            goto fail;
        }
        memset(cluster->stack_tops, 0, (uint32)total_size);

        if (!(cluster->stack_segment_occupied =
            wasm_runtime_malloc(cluster_max_thread_num * sizeof(bool)))) {
            goto fail;
        }
        memset(cluster->stack_segment_occupied, 0,
               cluster_max_thread_num * sizeof(bool));

        /* Reserve space for main instance */
        aux_stack_start -= cluster->stack_size;

        for (i = 0; i < cluster_max_thread_num; i++) {
            cluster->stack_tops[i] = aux_stack_start - cluster->stack_size * i;
        }
    }

    os_mutex_lock(&cluster_list_lock);
    if (bh_list_insert(cluster_list, cluster) != 0) {
        os_mutex_unlock(&cluster_list_lock);
        goto fail;
    }
    os_mutex_unlock(&cluster_list_lock);

    return cluster;

fail:
    if (cluster)
        wasm_cluster_destroy(cluster);

    return NULL;
}

static void
destroy_cluster_visitor(void *node, void *user_data)
{
    DestroyCallBackNode *destroy_node = (DestroyCallBackNode *)node;
    WASMCluster *cluster = (WASMCluster *)user_data;

    destroy_node->destroy_cb(cluster);
}

void
wasm_cluster_destroy(WASMCluster *cluster)
{
    traverse_list(destroy_callback_list,
                  destroy_cluster_visitor, (void *)cluster);

    /* Remove the cluster from the cluster list */
    os_mutex_lock(&cluster_list_lock);
    bh_list_remove(cluster_list, cluster);
    os_mutex_unlock(&cluster_list_lock);

    os_mutex_destroy(&cluster->lock);

    if (cluster->stack_tops)
        wasm_runtime_free(cluster->stack_tops);
    if (cluster->stack_segment_occupied)
        wasm_runtime_free(cluster->stack_segment_occupied);
    wasm_runtime_free(cluster);
}

static void
free_node_visitor(void *node, void *user_data)
{
    wasm_runtime_free(node);
}

void
wasm_cluster_cancel_all_callbacks()
{
    traverse_list(destroy_callback_list, free_node_visitor, NULL);
}

WASMCluster *
wasm_exec_env_get_cluster(WASMExecEnv *exec_env)
{
    return exec_env->cluster;
}

bool
wasm_cluster_add_exec_env(WASMCluster *cluster, WASMExecEnv *exec_env)
{
    bool ret = true;

    exec_env->cluster = cluster;

    os_mutex_lock(&cluster->lock);
    if (bh_list_insert(&cluster->exec_env_list, exec_env) != 0)
        ret = false;
    os_mutex_unlock(&cluster->lock);
    return ret;
}

bool
wasm_cluster_del_exec_env(WASMCluster *cluster, WASMExecEnv *exec_env)
{
    bool ret = true;
    bh_assert(exec_env->cluster == cluster);
    os_mutex_lock(&cluster->lock);
    if (bh_list_remove(&cluster->exec_env_list, exec_env) != 0)
        ret = false;
    os_mutex_unlock(&cluster->lock);

    if (cluster->exec_env_list.len == 0) {
        /* exec_env_list empty, destroy the cluster */
        wasm_cluster_destroy(cluster);
    }
    return ret;
}

/* start routine of thread manager */
static void*
thread_manager_start_routine(void *arg)
{
    void *ret;
    WASMExecEnv *exec_env = (WASMExecEnv *)arg;
    WASMCluster *cluster = wasm_exec_env_get_cluster(exec_env);
    bh_assert(cluster != NULL);

    exec_env->handle = os_self_thread();
    ret = exec_env->thread_start_routine(exec_env);

    /* Routine exit */
    /* Free aux stack space */
    free_aux_stack(cluster,
                   exec_env->aux_stack_boundary + cluster->stack_size);
    /* Detach the native thread here to ensure the resources are freed */
    wasm_cluster_detach_thread(exec_env);
    /* Remove and destroy exec_env */
    wasm_cluster_del_exec_env(cluster, exec_env);
    wasm_exec_env_destroy_internal(exec_env);

    os_thread_exit(ret);
    return ret;
}

int32
wasm_cluster_create_thread(WASMExecEnv *exec_env,
                           wasm_module_inst_t module_inst,
                           void* (*thread_routine)(void *),
                           void *arg)
{
    WASMCluster *cluster;
    WASMExecEnv *new_exec_env;
    uint32 aux_stack_start, aux_stack_size;
    korp_tid tid;

    cluster = wasm_exec_env_get_cluster(exec_env);
    bh_assert(cluster);

    new_exec_env = wasm_exec_env_create_internal(
                        module_inst, exec_env->wasm_stack_size);
    if (!new_exec_env)
        return -1;

    if (!allocate_aux_stack(cluster, &aux_stack_start, &aux_stack_size)) {
        LOG_ERROR("thread manager error: "
                  "failed to allocate aux stack space for new thread");
        goto fail1;
    }

    /* Set aux stack for current thread */
    if (!wasm_exec_env_set_aux_stack(new_exec_env, aux_stack_start,
                                     aux_stack_size)) {
        goto fail2;
    }

    if (!wasm_cluster_add_exec_env(cluster, new_exec_env))
        goto fail2;

    new_exec_env->thread_start_routine = thread_routine;
    new_exec_env->thread_arg = arg;

    if (0 != os_thread_create(&tid, thread_manager_start_routine,
                              (void *)new_exec_env,
                              APP_THREAD_STACK_SIZE_DEFAULT)) {
        goto fail3;
    }

    return 0;

fail3:
    wasm_cluster_del_exec_env(cluster, new_exec_env);
fail2:
    /* free the allocated aux stack space */
    free_aux_stack(cluster, aux_stack_start);
fail1:
    wasm_exec_env_destroy(new_exec_env);
    return -1;
}

int32
wasm_cluster_join_thread(WASMExecEnv *exec_env, void **ret_val)
{
    return os_thread_join(exec_env->handle, ret_val);
}

int32
wasm_cluster_detach_thread(WASMExecEnv *exec_env)
{
    return os_thread_detach(exec_env->handle);
}

void
wasm_cluster_exit_thread(WASMExecEnv *exec_env, void *retval)
{
    WASMCluster *cluster;

    cluster = wasm_exec_env_get_cluster(exec_env);
    bh_assert(cluster);

    /* App exit the thread, free the resources before exit native thread */
    /* Free aux stack space */
    free_aux_stack(cluster,
                   exec_env->aux_stack_boundary + cluster->stack_size);
    /* Detach the native thread here to ensure the resources are freed */
    wasm_cluster_detach_thread(exec_env);
    /* Remove and destroy exec_env */
    wasm_cluster_del_exec_env(cluster, exec_env);
    wasm_exec_env_destroy_internal(exec_env);

    os_thread_exit(retval);
}

int32
wasm_cluster_cancel_thread(WASMExecEnv *exec_env)
{
    /* Set the termination flag */
    exec_env->suspend_flags |= 0x01;
    return 0;
}

static void
terminate_thread_visitor(void *node, void *user_data)
{
    WASMExecEnv *curr_exec_env = (WASMExecEnv *)node;
    WASMExecEnv *exec_env = (WASMExecEnv *)user_data;

    if (curr_exec_env == exec_env)
        return;

    wasm_cluster_cancel_thread(curr_exec_env);
    wasm_cluster_join_thread(curr_exec_env, NULL);
}

void
wasm_cluster_terminate_all(WASMCluster *cluster)
{
    traverse_list(&cluster->exec_env_list,
                  terminate_thread_visitor, NULL);
}

void
wasm_cluster_terminate_all_except_self(WASMCluster *cluster,
                                       WASMExecEnv *exec_env)
{
    traverse_list(&cluster->exec_env_list,
                  terminate_thread_visitor, (void *)exec_env);
}

bool
wasm_cluster_register_destroy_callback(void (*callback)(WASMCluster *))
{
    DestroyCallBackNode *node;

    if (!(node = wasm_runtime_malloc(sizeof(DestroyCallBackNode)))) {
        LOG_ERROR("thread manager error: failed to allocate memory");
        return false;
    }
    node->destroy_cb = callback;
    bh_list_insert(destroy_callback_list, node);
    return true;
}

void
wasm_cluster_suspend_thread(WASMExecEnv *exec_env)
{
    /* Set the suspend flag */
    exec_env->suspend_flags |= 0x02;
}

static void
suspend_thread_visitor(void *node, void *user_data)
{
    WASMExecEnv *curr_exec_env = (WASMExecEnv *)node;
    WASMExecEnv *exec_env = (WASMExecEnv *)user_data;

    if (curr_exec_env == exec_env)
        return;

    wasm_cluster_suspend_thread(curr_exec_env);
}

void
wasm_cluster_suspend_all(WASMCluster *cluster)
{
    traverse_list(&cluster->exec_env_list,
                  suspend_thread_visitor, NULL);
}

void
wasm_cluster_suspend_all_except_self(WASMCluster *cluster,
                                     WASMExecEnv *exec_env)
{
    traverse_list(&cluster->exec_env_list,
                  suspend_thread_visitor, (void *)exec_env);
}

void
wasm_cluster_resume_thread(WASMExecEnv *exec_env)
{
    exec_env->suspend_flags &= ~0x02;
}

static void
resume_thread_visitor(void *node, void *user_data)
{
    WASMExecEnv *curr_exec_env = (WASMExecEnv *)node;

    wasm_cluster_resume_thread(curr_exec_env);
}

void
wasm_cluster_resume_all(WASMCluster *cluster)
{
    traverse_list(&cluster->exec_env_list, resume_thread_visitor, NULL);
}

static void
set_exception_visitor(void *node, void *user_data)
{
    WASMExecEnv *curr_exec_env = (WASMExecEnv *)node;
    WASMExecEnv *exec_env = (WASMExecEnv *)user_data;
    WASMModuleInstanceCommon *module_inst = get_module_inst(exec_env);
    WASMModuleInstanceCommon *curr_module_inst =
                                    get_module_inst(curr_exec_env);
    const char *exception = wasm_runtime_get_exception(module_inst);
    /* skip "Exception: " */
    exception += 11;

    if (curr_exec_env != exec_env) {
        curr_module_inst = get_module_inst(curr_exec_env);
        wasm_runtime_set_exception(curr_module_inst, exception);
    }
}

void
wasm_cluster_spread_exception(WASMExecEnv *exec_env)
{
    WASMCluster *cluster = wasm_exec_env_get_cluster(exec_env);

    traverse_list(&cluster->exec_env_list, set_exception_visitor, exec_env);
}
