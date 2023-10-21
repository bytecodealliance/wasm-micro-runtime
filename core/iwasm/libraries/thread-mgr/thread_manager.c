/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "thread_manager.h"

#if WASM_ENABLE_INTERP != 0
#include "../interpreter/wasm_runtime.h"
#endif
#if WASM_ENABLE_AOT != 0
#include "../aot/aot_runtime.h"
#endif

#if WASM_ENABLE_DEBUG_INTERP != 0
#include "debug_engine.h"
#endif

typedef struct {
    bh_list_link l;
    void (*destroy_cb)(WASMCluster *);
} DestroyCallBackNode;

typedef struct GlobalExecEnvNode {
    struct GlobalExecEnvNode *next;
    WASMExecEnv *exec_env;
} GlobalExecEnvNode;

static bh_list destroy_callback_list_head;
static bh_list *const destroy_callback_list = &destroy_callback_list_head;

static bh_list cluster_list_head;
static bh_list *const cluster_list = &cluster_list_head;
static korp_mutex cluster_list_lock;

static korp_mutex _exception_lock;

typedef void (*list_visitor)(void *, void *);

static uint32 cluster_max_thread_num = CLUSTER_MAX_THREAD_NUM;

/* Set the maximum thread number, if this function is not called,
    the max thread num is defined by CLUSTER_MAX_THREAD_NUM */
void
wasm_cluster_set_max_thread_num(uint32 num)
{
    if (num > 0)
        cluster_max_thread_num = num;
}

static korp_mutex global_exec_env_list_lock;
static GlobalExecEnvNode *global_exec_env_list = NULL;

static bool
global_exec_env_list_add(WASMExecEnv *exec_env)
{
    GlobalExecEnvNode *exec_env_node;

    exec_env_node = wasm_runtime_malloc(sizeof(GlobalExecEnvNode));
    if (!exec_env_node) {
        return false;
    }

    os_mutex_lock(&global_exec_env_list_lock);

    exec_env_node->exec_env = exec_env;
    exec_env_node->next = global_exec_env_list;
    global_exec_env_list = exec_env_node;

    os_mutex_unlock(&global_exec_env_list_lock);

    return true;
}

static void
global_exec_env_list_del(WASMExecEnv *exec_env)
{
    GlobalExecEnvNode *exec_env_node, *prev = NULL;

    os_mutex_lock(&global_exec_env_list_lock);

    exec_env_node = global_exec_env_list;

    while (exec_env_node) {
        if (exec_env_node->exec_env == exec_env) {
            if (prev)
                prev->next = exec_env_node->next;
            else
                global_exec_env_list = exec_env_node->next;
            wasm_runtime_free(exec_env_node);
            break;
        }
        prev = exec_env_node;
        exec_env_node = exec_env_node->next;
    }

    os_mutex_unlock(&global_exec_env_list_lock);
}

static void
global_exec_env_list_del_exec_envs(WASMCluster *cluster)
{
    GlobalExecEnvNode *exec_env_node, *prev = NULL, *next;

    os_mutex_lock(&global_exec_env_list_lock);

    exec_env_node = global_exec_env_list;

    while (exec_env_node) {
        if (exec_env_node->exec_env->cluster == cluster) {
            if (prev)
                prev->next = next = exec_env_node->next;
            else
                global_exec_env_list = next = exec_env_node->next;

            wasm_runtime_free(exec_env_node);
            exec_env_node = next;
        }
        else {
            prev = exec_env_node;
            exec_env_node = exec_env_node->next;
        }
    }

    os_mutex_unlock(&global_exec_env_list_lock);
}

static WASMExecEnv *
global_exec_env_list_find_with_inst(WASMModuleInstanceCommon *module_inst)
{
    GlobalExecEnvNode *exec_env_node;
    WASMExecEnv *exec_env;

    os_mutex_lock(&global_exec_env_list_lock);

    exec_env_node = global_exec_env_list;

    while (exec_env_node) {
        if (exec_env_node->exec_env->module_inst == module_inst) {
            exec_env = exec_env_node->exec_env;
            os_mutex_unlock(&global_exec_env_list_lock);
            return exec_env;
        }
        exec_env_node = exec_env_node->next;
    }

    os_mutex_unlock(&global_exec_env_list_lock);

    return NULL;
}

static WASMExecEnv *
global_exec_env_list_find_with_tid(korp_tid handle)
{
    GlobalExecEnvNode *exec_env_node;
    WASMExecEnv *exec_env;

    os_mutex_lock(&global_exec_env_list_lock);

    exec_env_node = global_exec_env_list;

    while (exec_env_node) {
        if (exec_env_node->exec_env->handle == handle) {
            exec_env = exec_env_node->exec_env;
            os_mutex_unlock(&global_exec_env_list_lock);
            return exec_env;
        }
        exec_env_node = exec_env_node->next;
    }

    os_mutex_unlock(&global_exec_env_list_lock);

    return NULL;
}

static WASMExecEnv *
get_exec_env_of_current_thread()
{
    korp_tid handle = os_self_thread();
    return global_exec_env_list_find_with_tid(handle);
}

static bool
global_exec_env_list_has_exec_env(WASMExecEnv *exec_env)
{
    GlobalExecEnvNode *exec_env_node;

    os_mutex_lock(&global_exec_env_list_lock);

    exec_env_node = global_exec_env_list;

    while (exec_env_node) {
        if (exec_env_node->exec_env == exec_env) {
            os_mutex_unlock(&global_exec_env_list_lock);
            return true;
        }
        exec_env_node = exec_env_node->next;
    }

    os_mutex_unlock(&global_exec_env_list_lock);

    return false;
}

static void
global_exec_env_list_destroy()
{
    GlobalExecEnvNode *exec_env_node, *next;

    /* Destroy global_exec_env_list */
    exec_env_node = global_exec_env_list;
    while (exec_env_node) {
        next = exec_env_node->next;
        wasm_runtime_free(exec_env_node);
        exec_env_node = next;
    }

    global_exec_env_list = NULL;
}

bool
thread_manager_init()
{
    if (bh_list_init(cluster_list) != 0)
        return false;

    if (os_mutex_init(&cluster_list_lock) != 0)
        return false;

    if (os_mutex_init(&_exception_lock) != 0)
        goto fail1;

    if (os_mutex_init(&global_exec_env_list_lock) != 0)
        goto fail2;

    return true;

fail2:
    os_mutex_destroy(&_exception_lock);
fail1:
    os_mutex_destroy(&cluster_list_lock);
    return false;
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

    global_exec_env_list_destroy();
    os_mutex_destroy(&global_exec_env_list_lock);

    os_mutex_destroy(&_exception_lock);
    os_mutex_destroy(&cluster_list_lock);
}

/* Safely change us to RUNNING state with pending suspensions
   checking, i.e. current state (-> SUSPENDED) -> RUNNING.  */
ThreadRunningState
wasm_thread_change_to_running(WASMExecEnv *self)
{
    WASMCluster *cluster = self->cluster;
    ThreadRunningState old_state;

    os_mutex_lock(&cluster->thread_state_lock);

    old_state = self->current_status.running_state;

    /* Suspend us while there are pending requests from other threads */
    if (self->suspend_count > 0) {
        self->current_status.running_state = WASM_THREAD_SUSPENDED;

        os_cond_broadcast(&cluster->thread_safe_cond);

        do {
            os_cond_wait(&cluster->thread_resume_cond,
                         &cluster->thread_state_lock);
        } while (self->suspend_count > 0);
    }

    self->current_status.running_state = WASM_THREAD_RUNNING;

    os_mutex_unlock(&cluster->thread_state_lock);

    return old_state;
}

/* Change us to a safe state and notify threads that are waiting for
   this condition (through thread_safed_cond) */
static ThreadRunningState
wasm_thread_change_to_safe(WASMExecEnv *self, ThreadRunningState state)
{
    WASMCluster *cluster = self->cluster;
    ThreadRunningState old_state;

    /* This lock can act as a memory barrier to ensure all results have
       been committed to memory before chaning our state to a safe
       state. */
    os_mutex_lock(&cluster->thread_state_lock);

    bh_assert(state != WASM_THREAD_RUNNING);
    old_state = self->current_status.running_state;
    self->current_status.running_state = state;

    os_cond_broadcast(&cluster->thread_safe_cond);

    os_mutex_unlock(&cluster->thread_state_lock);

    return old_state;
}

static void
cluster_lock_thread_list(WASMCluster *cluster, WASMExecEnv *self)
{
    if (self) {
        bh_assert(self->current_status.running_state == WASM_THREAD_RUNNING);
    }

    /* If we are a thread of cluster, we must avoid dead lock between us
       and another thread of cluster who is suspending all or resuming all,
       which also requires the cluster lock. */
    for (;;) {
        if (os_mutex_trylock(&cluster->lock)) {
            /* The trylock failed, go the slow path to get the lock.  */

            /* The lock may be held by a thread that is suspending us,
               so we must change to a safe state before grabbing the
               lock with blocking approach. */
            if (self)
                wasm_thread_change_to_safe(self, WASM_THREAD_VMWAIT);

            /* Grab the lock with blocking approach since we have been
               in a safe state. */
            os_mutex_lock(&cluster->lock);
        }

        if (!self) {
            break;
        }

        /* Now, we've got the lock, if there are pending suspensions,
           we must release the lock, suspend us and retry to grab the
           lock because we may have been suspended by a thread that
           has suspended all threads, which needs this lock to do
           resume-all. If we suspend us when return to RUNNING state
           while holding the lock, deadlock may occur. We don't need
           to lock the thread_state_lock because our suspend_count can
           only be increased by a thread holding thread_list_lock.
           Decreasing this count in this period (can only be done by
           wasm_thread_resume) is safe since it may just make us loop
           again. */
        if (self->suspend_count == 0) {
            /* We are not suspended, so changing back to RUNNING state
               with holding the thread list lock is safe. */
            self->current_status.running_state = WASM_THREAD_RUNNING;
            break;
        }
        else {
            /* Unlock the thread list lock, suspend us and retry. */
            os_mutex_unlock(&cluster->lock);
            wasm_thread_change_to_running(self);
        }
    }
}

static inline void
cluster_unlock_thread_list(WASMCluster *cluster)
{
    os_mutex_unlock(&cluster->lock);
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

/* Assumes cluster->lock is locked */
static bool
safe_traverse_exec_env_list(WASMCluster *cluster, WASMExecEnv *self,
                            list_visitor visitor, void *user_data)
{
    Vector proc_nodes;
    void *node;
    bool ret = true;

    if (!bh_vector_init(&proc_nodes, cluster->exec_env_list.len, sizeof(void *),
                        false)) {
        ret = false;
        goto final;
    }

    node = bh_list_first_elem(&cluster->exec_env_list);

    while (node) {
        bool already_processed = false;
        void *proc_node;
        uint32 i;
        for (i = 0; i < (uint32)bh_vector_size(&proc_nodes); i++) {
            if (!bh_vector_get(&proc_nodes, i, &proc_node)) {
                ret = false;
                goto final;
            }
            if (proc_node == node) {
                already_processed = true;
                break;
            }
        }
        if (already_processed) {
            node = bh_list_elem_next(node);
            continue;
        }

        cluster_unlock_thread_list(cluster);
        visitor(node, user_data);
        cluster_lock_thread_list(cluster, self);
        if (!bh_vector_append(&proc_nodes, &node)) {
            ret = false;
            goto final;
        }

        node = bh_list_first_elem(&cluster->exec_env_list);
    }

final:
    bh_vector_destroy(&proc_nodes);

    return ret;
}

/* The caller must lock cluster->lock */
static bool
allocate_aux_stack(WASMExecEnv *exec_env, uint32 *start, uint32 *size)
{
    WASMCluster *cluster = wasm_exec_env_get_cluster(exec_env);
#if WASM_ENABLE_HEAP_AUX_STACK_ALLOCATION != 0
    WASMModuleInstanceCommon *module_inst =
        wasm_exec_env_get_module_inst(exec_env);
    uint32 stack_end;

    stack_end = wasm_runtime_module_malloc_internal(module_inst, exec_env,
                                                    cluster->stack_size, NULL);
    *start = stack_end + cluster->stack_size;
    *size = cluster->stack_size;

    return stack_end != 0;
#else
    uint32 i;

    /* If the module doesn't have aux stack info,
        it can't create any threads */
    if (!cluster->stack_segment_occupied)
        return false;

    for (i = 0; i < cluster_max_thread_num; i++) {
        if (!cluster->stack_segment_occupied[i]) {
            if (start)
                *start = cluster->stack_tops[i];
            if (size)
                *size = cluster->stack_size;
            cluster->stack_segment_occupied[i] = true;
            return true;
        }
    }

    return false;
#endif
}

/* The caller must lock cluster->lock */
static bool
free_aux_stack(WASMExecEnv *exec_env, uint32 start)
{
    WASMCluster *cluster = wasm_exec_env_get_cluster(exec_env);

#if WASM_ENABLE_HEAP_AUX_STACK_ALLOCATION != 0
    WASMModuleInstanceCommon *module_inst =
        wasm_exec_env_get_module_inst(exec_env);

    if (!wasm_exec_env_is_aux_stack_managed_by_runtime(exec_env)) {
        return true;
    }

    bh_assert(start >= cluster->stack_size);

    wasm_runtime_module_free_internal(module_inst, exec_env,
                                      start - cluster->stack_size);

    return true;
#else
    uint32 i;

    for (i = 0; i < cluster_max_thread_num; i++) {
        if (start == cluster->stack_tops[i]) {
            cluster->stack_segment_occupied[i] = false;
            return true;
        }
    }
    return false;
#endif
}

WASMCluster *
wasm_cluster_create(WASMExecEnv *exec_env)
{
    WASMCluster *cluster;
    uint32 aux_stack_start, aux_stack_size;
    bh_list_status ret;

    bh_assert(exec_env->cluster == NULL);
    if (!(cluster = wasm_runtime_malloc(sizeof(WASMCluster)))) {
        LOG_ERROR("thread manager error: failed to allocate memory");
        return NULL;
    }
    memset(cluster, 0, sizeof(WASMCluster));

    exec_env->cluster = cluster;

    bh_list_init(&cluster->exec_env_list);
    ret = bh_list_insert(&cluster->exec_env_list, exec_env);
    bh_assert(ret == BH_LIST_SUCCESS);
    (void)ret;

    if (!global_exec_env_list_add(exec_env))
        goto fail1;

    if (os_mutex_init(&cluster->lock) != 0) {
        LOG_ERROR("thread manager error: failed to init cluster lock");
        goto fail2;
    }

    if (os_mutex_init(&cluster->thread_state_lock) != 0) {
        LOG_ERROR("thread manager error: failed to init thread_state_lock");
        goto fail3;
    }

    if (os_cond_init(&cluster->thread_safe_cond) != 0) {
        LOG_ERROR("thread manager error: failed to init thread_safe_cond");
        goto fail4;
    }

    if (os_cond_init(&cluster->thread_resume_cond) != 0) {
        LOG_ERROR("thread manager error: failed to init thread_resume_cond");
        goto fail5;
    }

    /* Prepare the aux stack top and size for every thread */
    if (!wasm_exec_env_get_aux_stack(exec_env, &aux_stack_start,
                                     &aux_stack_size)) {
#if WASM_ENABLE_LIB_WASI_THREADS == 0
        LOG_VERBOSE("No aux stack info for this module, can't create thread");
#endif

        /* If the module don't have aux stack info, don't throw error here,
            but remain stack_tops and stack_segment_occupied as NULL */
        os_mutex_lock(&cluster_list_lock);
        ret = bh_list_insert(cluster_list, cluster);
        bh_assert(ret == BH_LIST_SUCCESS);
        (void)ret;
        os_mutex_unlock(&cluster_list_lock);

        return cluster;
    }

#if WASM_ENABLE_HEAP_AUX_STACK_ALLOCATION != 0
    cluster->stack_size = aux_stack_size;
#else
    cluster->stack_size = aux_stack_size / (cluster_max_thread_num + 1);
    if (cluster->stack_size < WASM_THREAD_AUX_STACK_SIZE_MIN) {
        goto fail6;
    }
    /* Make stack size 16-byte aligned */
    cluster->stack_size = cluster->stack_size & (~15);
#endif

    /* Set initial aux stack top to the instance and
        aux stack boundary to the main exec_env */
    if (!wasm_exec_env_set_aux_stack(exec_env, aux_stack_start,
                                     cluster->stack_size))
        goto fail6;

#if WASM_ENABLE_HEAP_AUX_STACK_ALLOCATION == 0
    if (cluster_max_thread_num != 0) {
        uint64 total_size = cluster_max_thread_num * sizeof(uint32);
        uint32 i;
        if (total_size >= UINT32_MAX
            || !(cluster->stack_tops =
                     wasm_runtime_malloc((uint32)total_size))) {
            goto fail6;
        }
        memset(cluster->stack_tops, 0, (uint32)total_size);

        if (!(cluster->stack_segment_occupied =
                  wasm_runtime_malloc(cluster_max_thread_num * sizeof(bool)))) {
            wasm_runtime_free(cluster->stack_tops);
            goto fail6;
        }
        memset(cluster->stack_segment_occupied, 0,
               cluster_max_thread_num * sizeof(bool));

        /* Reserve space for main instance */
        aux_stack_start -= cluster->stack_size;

        for (i = 0; i < cluster_max_thread_num; i++) {
            cluster->stack_tops[i] = aux_stack_start - cluster->stack_size * i;
        }
    }
#endif

    os_mutex_lock(&cluster_list_lock);
    ret = bh_list_insert(cluster_list, cluster);
    bh_assert(ret == BH_LIST_SUCCESS);
    (void)ret;
    os_mutex_unlock(&cluster_list_lock);

    return cluster;

fail6:
    os_cond_destroy(&cluster->thread_resume_cond);
fail5:
    os_cond_destroy(&cluster->thread_safe_cond);
fail4:
    os_mutex_destroy(&cluster->thread_state_lock);
fail3:
    os_mutex_destroy(&cluster->lock);
fail2:
    global_exec_env_list_del(exec_env);
fail1:
    wasm_runtime_free(cluster);

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
    traverse_list(destroy_callback_list, destroy_cluster_visitor,
                  (void *)cluster);

    /* Destroy exec_envs of this cluster from global_exec_env_list */
    global_exec_env_list_del_exec_envs(cluster);

    /* Remove the cluster from the cluster list */
    os_mutex_lock(&cluster_list_lock);
    bh_list_remove(cluster_list, cluster);
    os_mutex_unlock(&cluster_list_lock);

    os_cond_destroy(&cluster->thread_resume_cond);
    os_cond_destroy(&cluster->thread_safe_cond);
    os_mutex_destroy(&cluster->thread_state_lock);
    os_mutex_destroy(&cluster->lock);

#if WASM_ENABLE_HEAP_AUX_STACK_ALLOCATION == 0
    if (cluster->stack_tops)
        wasm_runtime_free(cluster->stack_tops);
    if (cluster->stack_segment_occupied)
        wasm_runtime_free(cluster->stack_segment_occupied);
#endif

#if WASM_ENABLE_DEBUG_INTERP != 0
    wasm_debug_instance_destroy(cluster);
#endif

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
    bh_list_init(destroy_callback_list);
}

WASMCluster *
wasm_exec_env_get_cluster(WASMExecEnv *exec_env)
{
    return exec_env->cluster;
}

/* The caller must lock cluster->lock */
static bool
wasm_cluster_add_exec_env(WASMCluster *cluster, WASMExecEnv *exec_env)
{
    bh_list_status ret;

    exec_env->cluster = cluster;

    if (cluster->exec_env_list.len == cluster_max_thread_num + 1) {
        LOG_ERROR("thread manager error: "
                  "maximum number of threads exceeded");
        return false;
    }

    if (!global_exec_env_list_add(exec_env)) {
        return false;
    }

    ret = bh_list_insert(&cluster->exec_env_list, exec_env);
    bh_assert(ret == BH_LIST_SUCCESS);
    (void)ret;

    return true;
}

/* The caller should lock cluster->lock for thread safety */
static void
wasm_cluster_del_exec_env_internal(WASMCluster *cluster, WASMExecEnv *exec_env,
                                   bool can_destroy_cluster)
{
    bh_assert(exec_env->cluster == cluster);

#if WASM_ENABLE_DEBUG_INTERP != 0
    /* Wait for debugger control thread to process the
       stop event of this thread */
    if (cluster->debug_inst) {
        /* lock the debug_inst->wait_lock so
           other threads can't fire stop events */
        os_mutex_lock(&cluster->debug_inst->wait_lock);
        while (cluster->debug_inst->stopped_thread == exec_env) {
            /* either wakes up by signal or by 1-second timeout */
            os_cond_reltimedwait(&cluster->debug_inst->wait_cond,
                                 &cluster->debug_inst->wait_lock, 1000000);
        }
        os_mutex_unlock(&cluster->debug_inst->wait_lock);
    }
#endif

    global_exec_env_list_del(exec_env);

    bh_list_remove(&cluster->exec_env_list, exec_env);

    if (can_destroy_cluster) {
        if (cluster->exec_env_list.len == 0) {
            /* exec_env_list empty, destroy the cluster */
            wasm_cluster_destroy(cluster);
        }
    }
    else {
        /* Don't destroy cluster as cluster->lock is being used */
    }
}

/* The caller should lock cluster->lock for thread safety */
void
wasm_cluster_del_exec_env(WASMCluster *cluster, WASMExecEnv *exec_env)
{
    wasm_cluster_del_exec_env_internal(cluster, exec_env, true);
}

/* Search the global exec_env list to find if the given
   module instance has a corresponding exec_env */
WASMExecEnv *
wasm_clusters_search_exec_env(WASMModuleInstanceCommon *module_inst)
{
    return global_exec_env_list_find_with_inst(module_inst);
}

WASMExecEnv *
wasm_cluster_spawn_exec_env(WASMExecEnv *exec_env)
{
    WASMCluster *cluster = wasm_exec_env_get_cluster(exec_env);
    wasm_module_inst_t module_inst = get_module_inst(exec_env);
    wasm_module_t module;
    wasm_module_inst_t new_module_inst;
    WASMExecEnv *new_exec_env;
    uint32 aux_stack_start, aux_stack_size;
    uint32 stack_size = 8192;

    if (!module_inst || !(module = wasm_exec_env_get_module(exec_env))) {
        return NULL;
    }

    cluster_lock_thread_list(cluster, exec_env);

    if (cluster->has_exception || cluster->processing) {
        goto fail1;
    }

#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        stack_size =
            ((WASMModuleInstance *)module_inst)->default_wasm_stack_size;
    }
#endif

#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT) {
        stack_size =
            ((AOTModuleInstance *)module_inst)->default_wasm_stack_size;
    }
#endif

    if (!(new_module_inst = wasm_runtime_instantiate_internal(
              module, module_inst, exec_env, stack_size, 0, NULL, 0))) {
        goto fail1;
    }

    /* Set custom_data to new module instance */
    wasm_runtime_set_custom_data_internal(
        new_module_inst, wasm_runtime_get_custom_data(module_inst));

    wasm_native_inherit_contexts(new_module_inst, module_inst);

    new_exec_env = wasm_exec_env_create_internal(new_module_inst,
                                                 exec_env->wasm_stack_size);
    if (!new_exec_env)
        goto fail2;

    if (!allocate_aux_stack(exec_env, &aux_stack_start, &aux_stack_size)) {
        LOG_ERROR("thread manager error: "
                  "failed to allocate aux stack space for new thread");
        goto fail3;
    }

    /* Set aux stack for current thread */
    if (!wasm_exec_env_set_aux_stack(new_exec_env, aux_stack_start,
                                     aux_stack_size)) {
        goto fail4;
    }

    /* Inherit suspend_flags of parent thread, no need to acquire
       thread_state_lock as the thread list has been locked */
    new_exec_env->suspend_flags.flags = exec_env->suspend_flags.flags;

    if (!wasm_cluster_add_exec_env(cluster, new_exec_env))
        goto fail4;

    cluster_unlock_thread_list(cluster);

    return new_exec_env;

fail4:
    /* Free the allocated aux stack space */
    free_aux_stack(exec_env, aux_stack_start);
fail3:
    wasm_exec_env_destroy_internal(new_exec_env);
fail2:
    wasm_runtime_deinstantiate_internal(new_module_inst, true);
fail1:
    cluster_unlock_thread_list(cluster);

    return NULL;
}

void
wasm_cluster_destroy_spawned_exec_env(WASMExecEnv *exec_env)
{
    WASMCluster *cluster = wasm_exec_env_get_cluster(exec_env);
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    WASMExecEnv *self = get_exec_env_of_current_thread();

    bh_assert(cluster != NULL);

    cluster_lock_thread_list(cluster, self);

    /* Free aux stack space */
    free_aux_stack(exec_env, exec_env->aux_stack_bottom.bottom);
    /* Remove exec_env */
    wasm_cluster_del_exec_env_internal(cluster, exec_env, false);
    /* Destroy exec_env */
    wasm_exec_env_destroy_internal(exec_env);
    /* Routine exit, destroy instance */
    wasm_runtime_deinstantiate_internal(module_inst, true);

    cluster_unlock_thread_list(cluster);
}

/* start routine of thread manager */
static void *
thread_manager_start_routine(void *arg)
{
    void *ret;
    WASMExecEnv *exec_env = (WASMExecEnv *)arg;
    WASMCluster *cluster = wasm_exec_env_get_cluster(exec_env);
    WASMModuleInstanceCommon *module_inst =
        wasm_exec_env_get_module_inst(exec_env);

    bh_assert(cluster != NULL);
    bh_assert(module_inst != NULL);

    os_mutex_lock(&exec_env->wait_lock);
    exec_env->handle = os_self_thread();
    /* Notify the parent thread to continue running */
    os_cond_signal(&exec_env->wait_cond);
    os_mutex_unlock(&exec_env->wait_lock);

    /* Check the pending suspensions */
    wasm_thread_change_to_running(exec_env);

    ret = exec_env->thread_start_routine(exec_env);

#ifdef OS_ENABLE_HW_BOUND_CHECK
    os_mutex_lock(&cluster->thread_state_lock);
    if (exec_env->suspend_flags.flags & WASM_SUSPEND_FLAG_EXIT)
        ret = exec_env->thread_ret_value;
    os_mutex_unlock(&cluster->thread_state_lock);
#endif

    /* Routine exit */

    cluster_lock_thread_list(cluster, exec_env);

    exec_env->current_status.running_state = WASM_THREAD_EXITED;

#if WASM_ENABLE_DEBUG_INTERP != 0
    wasm_cluster_thread_exited(exec_env);
#endif

    os_mutex_lock(&exec_env->wait_lock);
    /* Detach the native thread here to ensure the resources are freed */
    if (exec_env->wait_count == 0 && !exec_env->thread_is_detached) {
        /* Only detach current thread when there is no other thread
           joining it, otherwise let the system resources for the
           thread be released after joining */
        os_thread_detach(exec_env->handle);
        /* No need to set exec_env->thread_is_detached to true here
           since we will exit soon */
    }
    os_mutex_unlock(&exec_env->wait_lock);

    /* Free aux stack space */
    free_aux_stack(exec_env, exec_env->aux_stack_bottom.bottom);
    /* Remove exec_env */
    wasm_cluster_del_exec_env_internal(cluster, exec_env, false);
    /* Destroy exec_env */
    wasm_exec_env_destroy_internal(exec_env);
    /* Routine exit, destroy instance */
    wasm_runtime_deinstantiate_internal(module_inst, true);

    cluster_unlock_thread_list(cluster);

    os_thread_exit(ret);
    return ret;
}

int32
wasm_cluster_create_thread(WASMExecEnv *exec_env,
                           wasm_module_inst_t module_inst, bool alloc_aux_stack,
                           void *(*thread_routine)(void *), void *arg)
{
    WASMCluster *cluster;
    WASMExecEnv *new_exec_env;
    uint32 aux_stack_start = 0, aux_stack_size;
    korp_tid tid;

    cluster = wasm_exec_env_get_cluster(exec_env);
    bh_assert(cluster);

    cluster_lock_thread_list(cluster, exec_env);

    if (cluster->has_exception || cluster->processing) {
        goto fail1;
    }

    new_exec_env =
        wasm_exec_env_create_internal(module_inst, exec_env->wasm_stack_size);
    if (!new_exec_env)
        goto fail1;

    if (alloc_aux_stack) {
        if (!allocate_aux_stack(exec_env, &aux_stack_start, &aux_stack_size)) {
            LOG_ERROR("thread manager error: "
                      "failed to allocate aux stack space for new thread");
            goto fail2;
        }

        /* Set aux stack for current thread */
        if (!wasm_exec_env_set_aux_stack(new_exec_env, aux_stack_start,
                                         aux_stack_size)) {
            goto fail3;
        }
    }
    else {
        /* Disable aux stack */
        new_exec_env->aux_stack_boundary.boundary = 0;
        new_exec_env->aux_stack_bottom.bottom = UINT32_MAX;
    }

    /* Inherit suspend_flags of parent thread, no need to acquire
       thread_state_lock as the thread list has been locked */
    new_exec_env->suspend_flags.flags = exec_env->suspend_flags.flags;

    if (!wasm_cluster_add_exec_env(cluster, new_exec_env))
        goto fail3;

    new_exec_env->thread_start_routine = thread_routine;
    new_exec_env->thread_arg = arg;

    os_mutex_lock(&new_exec_env->wait_lock);

    if (0
        != os_thread_create(&tid, thread_manager_start_routine,
                            (void *)new_exec_env,
                            APP_THREAD_STACK_SIZE_DEFAULT)) {
        os_mutex_unlock(&new_exec_env->wait_lock);
        goto fail4;
    }

    /* Wait until the new_exec_env->handle is set to avoid it is
       illegally accessed after unlocking cluster->lock */
    os_cond_wait(&new_exec_env->wait_cond, &new_exec_env->wait_lock);
    os_mutex_unlock(&new_exec_env->wait_lock);

    cluster_unlock_thread_list(cluster);

    return 0;

fail4:
    wasm_cluster_del_exec_env_internal(cluster, new_exec_env, false);
fail3:
    /* Free the allocated aux stack space */
    if (alloc_aux_stack)
        free_aux_stack(exec_env, aux_stack_start);
fail2:
    wasm_exec_env_destroy_internal(new_exec_env);
fail1:
    cluster_unlock_thread_list(cluster);

    return -1;
}

bool
wasm_cluster_dup_c_api_imports(WASMModuleInstanceCommon *module_inst_dst,
                               const WASMModuleInstanceCommon *module_inst_src)
{
    /* workaround about passing instantiate-linking information */
    CApiFuncImport **new_c_api_func_imports = NULL;
    CApiFuncImport *c_api_func_imports;
    uint32 import_func_count = 0;
    uint32 size_in_bytes = 0;

#if WASM_ENABLE_INTERP != 0
    if (module_inst_src->module_type == Wasm_Module_Bytecode) {
        new_c_api_func_imports = &(((WASMModuleInstance *)module_inst_dst)
                                       ->e->common.c_api_func_imports);
        c_api_func_imports = ((const WASMModuleInstance *)module_inst_src)
                                 ->e->common.c_api_func_imports;
        import_func_count =
            ((WASMModule *)(((const WASMModuleInstance *)module_inst_src)
                                ->module))
                ->import_function_count;
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst_src->module_type == Wasm_Module_AoT) {
        AOTModuleInstanceExtra *e =
            (AOTModuleInstanceExtra *)((AOTModuleInstance *)module_inst_dst)->e;
        new_c_api_func_imports = &(e->common.c_api_func_imports);

        e = (AOTModuleInstanceExtra *)((AOTModuleInstance *)module_inst_src)->e;
        c_api_func_imports = e->common.c_api_func_imports;

        import_func_count =
            ((AOTModule *)(((AOTModuleInstance *)module_inst_src)->module))
                ->import_func_count;
    }
#endif

    if (import_func_count != 0 && c_api_func_imports) {
        size_in_bytes = sizeof(CApiFuncImport) * import_func_count;
        *new_c_api_func_imports = wasm_runtime_malloc(size_in_bytes);
        if (!(*new_c_api_func_imports))
            return false;

        bh_memcpy_s(*new_c_api_func_imports, size_in_bytes, c_api_func_imports,
                    size_in_bytes);
    }
    return true;
}

#if WASM_ENABLE_DEBUG_INTERP != 0
inline static bool
wasm_cluster_thread_is_running(WASMExecEnv *exec_env)
{
    return exec_env->current_status.running_state == WASM_THREAD_RUNNING
           || exec_env->current_status.running_state == WASM_THREAD_STEP;
}

void
wasm_cluster_clear_thread_signal(WASMExecEnv *exec_env)
{
    exec_env->current_status.signal_flag = 0;
}

void
wasm_cluster_thread_send_signal(WASMExecEnv *exec_env, uint32 signo)
{
    exec_env->current_status.signal_flag = signo;
}

static void
notify_debug_instance(WASMExecEnv *exec_env)
{
    WASMCluster *cluster;

    cluster = wasm_exec_env_get_cluster(exec_env);
    bh_assert(cluster);

    if (!cluster->debug_inst) {
        return;
    }

    on_thread_stop_event(cluster->debug_inst, exec_env);
}

static void
notify_debug_instance_exit(WASMExecEnv *exec_env)
{
    WASMCluster *cluster;

    cluster = wasm_exec_env_get_cluster(exec_env);
    bh_assert(cluster);

    if (!cluster->debug_inst) {
        return;
    }

    on_thread_exit_event(cluster->debug_inst, exec_env);
}

void
wasm_cluster_thread_waiting_run(WASMExecEnv *exec_env)
{
    exec_env->current_status.running_state = WASM_THREAD_STOP;
    notify_debug_instance(exec_env);

    while (!wasm_cluster_thread_is_running(exec_env)) {
        os_cond_wait(&exec_env->wait_cond, &exec_env->wait_lock);
    }
}

void
wasm_cluster_send_signal_all(WASMCluster *cluster, uint32 signo)
{
    WASMExecEnv *exec_env = bh_list_first_elem(&cluster->exec_env_list);
    while (exec_env) {
        wasm_cluster_thread_send_signal(exec_env, signo);
        exec_env = bh_list_elem_next(exec_env);
    }
}

void
wasm_cluster_thread_exited(WASMExecEnv *exec_env)
{
    /* TODO */
    exec_env->current_status.running_state = WASM_THREAD_EXITED;
    notify_debug_instance_exit(exec_env);
}

void
wasm_cluster_thread_continue(WASMExecEnv *exec_env)
{
    os_mutex_lock(&exec_env->wait_lock);
    wasm_cluster_clear_thread_signal(exec_env);
    /* TODO */
    exec_env->current_status.running_state = WASM_THREAD_RUNNING;
    os_cond_signal(&exec_env->wait_cond);
    os_mutex_unlock(&exec_env->wait_lock);
}

void
wasm_cluster_thread_step(WASMExecEnv *exec_env)
{
    os_mutex_lock(&exec_env->wait_lock);
    /* TODO */
    exec_env->current_status.running_state = WASM_THREAD_STEP;
    os_cond_signal(&exec_env->wait_cond);
    os_mutex_unlock(&exec_env->wait_lock);
}

void
wasm_cluster_set_debug_inst(WASMCluster *cluster, WASMDebugInstance *inst)
{
    cluster->debug_inst = inst;
}
#endif /* end of WASM_ENABLE_DEBUG_INTERP */

/* Check whether the exec_env is in one of all clusters */
static bool
clusters_have_exec_env(WASMExecEnv *exec_env)
{
    return global_exec_env_list_has_exec_env(exec_env);
}

int32
wasm_cluster_join_thread(WASMExecEnv *exec_env, WASMExecEnv *self,
                         void **ret_val)
{
    korp_tid handle;
    int32 ret;

    if (!clusters_have_exec_env(exec_env)) {
        /* Invalid thread or thread has exited */
        if (ret_val)
            *ret_val = NULL;
        return 0;
    }

    os_mutex_lock(&exec_env->wait_lock);

    if (exec_env->thread_is_detached) {
        /* Thread has been detached */
        if (ret_val)
            *ret_val = NULL;
        os_mutex_unlock(&exec_env->wait_lock);
        return 0;
    }

    exec_env->wait_count++;
    handle = exec_env->handle;

    os_mutex_unlock(&exec_env->wait_lock);

    if (self)
        wasm_thread_change_to_safe(self, WASM_THREAD_VMWAIT);

    ret = os_thread_join(handle, ret_val);

    if (self)
        wasm_thread_change_to_running(self);

    return ret;
}

int32
wasm_cluster_detach_thread(WASMExecEnv *exec_env)
{
    int32 ret = 0;

    if (!clusters_have_exec_env(exec_env)) {
        /* Invalid thread or the thread has exited */
        return 0;
    }

    os_mutex_lock(&exec_env->wait_lock);
    if (exec_env->wait_count == 0 && !exec_env->thread_is_detached) {
        /* Only detach current thread when there is no other thread
           joining it, otherwise let the system resources for the
           thread be released after joining */
        ret = os_thread_detach(exec_env->handle);
        exec_env->thread_is_detached = true;

        os_mutex_lock(&exec_env->cluster->thread_state_lock);
        exec_env->current_status.running_state = WASM_THREAD_EXITED;
        os_mutex_unlock(&exec_env->cluster->thread_state_lock);
    }
    os_mutex_unlock(&exec_env->wait_lock);

    return ret;
}

void
wasm_cluster_exit_thread(WASMExecEnv *exec_env, void *retval)
{
    WASMCluster *cluster;
    WASMModuleInstanceCommon *module_inst;

    cluster = wasm_exec_env_get_cluster(exec_env);
    bh_assert(cluster);

#ifdef OS_ENABLE_HW_BOUND_CHECK
    if (exec_env->jmpbuf_stack_top) {
        os_mutex_lock(&cluster->thread_state_lock);
        /* Store the return value in exec_env */
        exec_env->thread_ret_value = retval;
        exec_env->suspend_flags.flags |= WASM_SUSPEND_FLAG_EXIT;
        os_mutex_unlock(&cluster->thread_state_lock);

#ifndef BH_PLATFORM_WINDOWS
        /* Pop all jmpbuf_node except the last one */
        while (exec_env->jmpbuf_stack_top->prev) {
            wasm_exec_env_pop_jmpbuf(exec_env);
        }
        os_longjmp(exec_env->jmpbuf_stack_top->jmpbuf, 1);
        return;
#endif
    }
#endif

    /* App exit the thread, free the resources before exit native thread */

    cluster_lock_thread_list(cluster, exec_env);

    exec_env->current_status.running_state = WASM_THREAD_EXITED;

#if WASM_ENABLE_DEBUG_INTERP != 0
    wasm_cluster_clear_thread_signal(exec_env);
    wasm_cluster_thread_exited(exec_env);
#endif

    os_mutex_lock(&exec_env->wait_lock);
    /* Detach the native thread here to ensure the resources are freed */
    if (exec_env->wait_count == 0 && !exec_env->thread_is_detached) {
        /* Only detach current thread when there is no other thread
           joining it, otherwise let the system resources for the
           thread be released after joining */
        os_thread_detach(exec_env->handle);
        /* No need to set exec_env->thread_is_detached to true here
           since we will exit soon */
    }
    os_mutex_unlock(&exec_env->wait_lock);

    module_inst = exec_env->module_inst;

    /* Free aux stack space */
    free_aux_stack(exec_env, exec_env->aux_stack_bottom.bottom);
    /* Remove exec_env */
    wasm_cluster_del_exec_env_internal(cluster, exec_env, false);
    /* Destroy exec_env */
    wasm_exec_env_destroy_internal(exec_env);
    /* Routine exit, destroy instance */
    wasm_runtime_deinstantiate_internal(module_inst, true);

    cluster_unlock_thread_list(cluster);

    os_thread_exit(retval);
}

static void
set_thread_cancel_flags(WASMExecEnv *exec_env)
{
    bh_assert(exec_env->cluster);

    os_mutex_lock(&exec_env->cluster->thread_state_lock);

#if WASM_ENABLE_DEBUG_INTERP != 0
    wasm_cluster_thread_send_signal(exec_env, WAMR_SIG_TERM);
#endif
    exec_env->suspend_flags.flags |= WASM_SUSPEND_FLAG_TERMINATE;

    os_mutex_unlock(&exec_env->cluster->thread_state_lock);

#ifdef OS_ENABLE_WAKEUP_BLOCKING_OP
    wasm_runtime_interrupt_blocking_op(exec_env);
#endif
}

static void
clear_thread_cancel_flags(WASMExecEnv *exec_env)
{
    bh_assert(exec_env->cluster);

    os_mutex_lock(&exec_env->cluster->thread_state_lock);

    exec_env->suspend_flags.flags &= ~WASM_SUSPEND_FLAG_TERMINATE;

    os_mutex_unlock(&exec_env->cluster->thread_state_lock);
}

int32
wasm_cluster_cancel_thread(WASMExecEnv *exec_env)
{
    if (!clusters_have_exec_env(exec_env) || !exec_env->cluster) {
        /* Invalid thread or the thread has exited */
        return 0;
    }

    set_thread_cancel_flags(exec_env);

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
    wasm_cluster_join_thread(curr_exec_env, exec_env, NULL);
}

void
wasm_cluster_terminate_all(WASMCluster *cluster)
{
    WASMExecEnv *self = get_exec_env_of_current_thread();

    cluster_lock_thread_list(cluster, self);
    cluster->processing = true;

    safe_traverse_exec_env_list(cluster, self, terminate_thread_visitor, self);

    cluster->processing = false;
    cluster_unlock_thread_list(cluster);
}

void
wasm_cluster_terminate_all_except_self(WASMCluster *cluster, WASMExecEnv *self)
{
    cluster_lock_thread_list(cluster, self);
    cluster->processing = true;

    safe_traverse_exec_env_list(cluster, self, terminate_thread_visitor,
                                (void *)self);

    cluster->processing = false;
    cluster_unlock_thread_list(cluster);
}

static void
wait_for_thread_visitor(void *node, void *user_data)
{
    WASMExecEnv *curr_exec_env = (WASMExecEnv *)node;
    WASMExecEnv *exec_env = (WASMExecEnv *)user_data;

    if (curr_exec_env == exec_env)
        return;

    wasm_cluster_join_thread(curr_exec_env, exec_env, NULL);
}

void
wams_cluster_wait_for_all(WASMCluster *cluster)
{
    WASMExecEnv *self = get_exec_env_of_current_thread();

    cluster_lock_thread_list(cluster, self);
    cluster->processing = true;

    safe_traverse_exec_env_list(cluster, self, wait_for_thread_visitor, self);

    cluster->processing = false;
    cluster_unlock_thread_list(cluster);
}

void
wasm_cluster_wait_for_all_except_self(WASMCluster *cluster, WASMExecEnv *self)
{
    cluster_lock_thread_list(cluster, self);
    cluster->processing = true;

    safe_traverse_exec_env_list(cluster, self, wait_for_thread_visitor,
                                (void *)self);

    cluster->processing = false;
    cluster_unlock_thread_list(cluster);
}

bool
wasm_cluster_register_destroy_callback(void (*callback)(WASMCluster *))
{
    DestroyCallBackNode *node;
    bh_list_status ret;

    if (!(node = wasm_runtime_malloc(sizeof(DestroyCallBackNode)))) {
        LOG_ERROR("thread manager error: failed to allocate memory");
        return false;
    }

    node->destroy_cb = callback;
    ret = bh_list_insert(destroy_callback_list, node);
    bh_assert(ret == BH_LIST_SUCCESS);
    (void)ret;

    return true;
}

/* Wait for the given thread, which may not be us, to suspend at a
   safe point, i.e. to get to a safe state.  */
static void
wait_for_thread_suspend(WASMExecEnv *exec_env)
{
    WASMCluster *cluster = exec_env->cluster;

    os_mutex_lock(&cluster->thread_state_lock);

    while (exec_env->current_status.running_state == WASM_THREAD_RUNNING)
        os_cond_wait(&cluster->thread_safe_cond, &cluster->thread_state_lock);

    os_mutex_unlock(&cluster->thread_state_lock);
}

void
wasm_cluster_suspend_thread(WASMExecEnv *exec_env, WASMExecEnv *self)
{
    WASMCluster *cluster = exec_env->cluster;

    /* Hold cluster lock until the target thread is suspended. */
    cluster_lock_thread_list(cluster, self);

    os_mutex_lock(&cluster->thread_state_lock);
    exec_env->suspend_count++;
    /* Set the suspend flag */
    exec_env->suspend_flags.flags |= WASM_SUSPEND_FLAG_SUSPEND;
    os_mutex_unlock(&cluster->thread_state_lock);

    wait_for_thread_suspend(exec_env);

    /* Don't release cluster lock until the target thread is
       suspended successfully. Otherwise, it may suspend us and
       cause deadlock. */
    cluster_unlock_thread_list(cluster);
}

void
wasm_cluster_suspend_all_except_self(WASMCluster *cluster, WASMExecEnv *self)
{
    WASMExecEnv *exec_env;

    /* Hold thread_list_lock until target threads are suspended */
    cluster_lock_thread_list(cluster, self);

    os_mutex_lock(&cluster->thread_state_lock);

    /* Increase suspend count for all threads except us */
    exec_env = bh_list_first_elem(&cluster->exec_env_list);
    while (exec_env) {
        if (exec_env != self) {
            exec_env->suspend_count++;
            exec_env->suspend_flags.flags |= WASM_SUSPEND_FLAG_SUSPEND;
        }
        exec_env = bh_list_elem_next(exec_env);
    }

    os_mutex_unlock(&cluster->thread_state_lock);

    exec_env = bh_list_first_elem(&cluster->exec_env_list);
    while (exec_env) {
        if (exec_env != self)
            wait_for_thread_suspend(exec_env);
        exec_env = bh_list_elem_next(exec_env);
    }

    /* Hold thread_list_lock until target threads are suspended */
    cluster_unlock_thread_list(cluster);
}

void
wasm_cluster_suspend_all(WASMCluster *cluster)
{
    WASMExecEnv *self = get_exec_env_of_current_thread();

    wasm_cluster_suspend_all_except_self(cluster, self);
}

void
wasm_cluster_resume_thread(WASMExecEnv *exec_env)
{
    WASMCluster *cluster = exec_env->cluster;

    os_mutex_lock(&cluster->thread_state_lock);

    if (exec_env->suspend_count > 0) {
        exec_env->suspend_count--;

        if (exec_env->suspend_count == 0) {
            exec_env->suspend_flags.flags &= ~WASM_SUSPEND_FLAG_SUSPEND;
            os_cond_broadcast(&cluster->thread_resume_cond);
        }
    }

    os_mutex_unlock(&cluster->thread_state_lock);
}

void
wasm_cluster_resume_all_except_self(WASMCluster *cluster, WASMExecEnv *self)
{
    WASMExecEnv *exec_env;

    cluster_lock_thread_list(cluster, self);

    /* No need to acquire the thread_state_lock
       as the thread list has been locked */

    /* Decrease suspend count for all threads except us. */
    exec_env = bh_list_first_elem(&cluster->exec_env_list);
    while (exec_env) {
        if (exec_env != self) {
            exec_env->suspend_count--;

            if (exec_env->suspend_count == 0) {
                exec_env->suspend_flags.flags &= ~WASM_SUSPEND_FLAG_SUSPEND;
                os_cond_broadcast(&cluster->thread_resume_cond);
            }
        }
        exec_env = bh_list_elem_next(exec_env);
    }

    cluster_unlock_thread_list(cluster);

    os_mutex_lock(&cluster->thread_state_lock);
    os_cond_broadcast(&cluster->thread_resume_cond);
    os_mutex_unlock(&cluster->thread_state_lock);
}

void
wasm_cluster_resume_all(WASMCluster *cluster)
{
    WASMExecEnv *self = get_exec_env_of_current_thread();

    wasm_cluster_resume_all_except_self(cluster, self);
}

struct spread_exception_data {
    WASMExecEnv *skip;
    const char *exception;
};

static void
set_exception_visitor(void *node, void *user_data)
{
    const struct spread_exception_data *data = user_data;
    WASMExecEnv *exec_env = (WASMExecEnv *)node;

    if (exec_env != data->skip) {
        WASMModuleInstance *wasm_inst =
            (WASMModuleInstance *)get_module_inst(exec_env);

        exception_lock(wasm_inst);
        if (data->exception != NULL) {
            snprintf(wasm_inst->cur_exception, sizeof(wasm_inst->cur_exception),
                     "Exception: %s", data->exception);
        }
        else {
            wasm_inst->cur_exception[0] = '\0';
        }
        exception_unlock(wasm_inst);

        /* Terminate the thread so it can exit from dead loops */
        if (data->exception != NULL) {
            set_thread_cancel_flags(exec_env);
        }
        else {
            clear_thread_cancel_flags(exec_env);
        }
    }
}

void
wasm_cluster_set_exception(WASMExecEnv *exec_env, const char *exception)
{
    const bool has_exception = exception != NULL;
    WASMCluster *cluster = wasm_exec_env_get_cluster(exec_env);
    bh_assert(cluster);

    struct spread_exception_data data;
    data.skip = NULL;
    data.exception = exception;

    cluster_lock_thread_list(cluster, exec_env);
    cluster->has_exception = has_exception;
    traverse_list(&cluster->exec_env_list, set_exception_visitor, &data);
    cluster_unlock_thread_list(cluster);
}

static void
set_custom_data_visitor(void *node, void *user_data)
{
    WASMExecEnv *curr_exec_env = (WASMExecEnv *)node;
    WASMModuleInstanceCommon *module_inst = get_module_inst(curr_exec_env);

    wasm_runtime_set_custom_data_internal(module_inst, user_data);
}

void
wasm_cluster_spread_custom_data(WASMModuleInstanceCommon *module_inst,
                                void *custom_data)
{
    WASMExecEnv *exec_env = wasm_clusters_search_exec_env(module_inst);

    if (exec_env == NULL) {
        /* Maybe threads have not been started yet. */
        wasm_runtime_set_custom_data_internal(module_inst, custom_data);
    }
    else {
        WASMCluster *cluster;

        cluster = wasm_exec_env_get_cluster(exec_env);
        bh_assert(cluster);

        cluster_lock_thread_list(cluster, exec_env);
        traverse_list(&cluster->exec_env_list, set_custom_data_visitor,
                      custom_data);
        cluster_unlock_thread_list(cluster);
    }
}

#if WASM_ENABLE_MODULE_INST_CONTEXT != 0
struct inst_set_context_data {
    void *key;
    void *ctx;
};

static void
set_context_visitor(void *node, void *user_data)
{
    WASMExecEnv *curr_exec_env = (WASMExecEnv *)node;
    WASMModuleInstanceCommon *module_inst = get_module_inst(curr_exec_env);
    const struct inst_set_context_data *data = user_data;

    wasm_runtime_set_context(module_inst, data->key, data->ctx);
}

void
wasm_cluster_set_context(WASMModuleInstanceCommon *module_inst, void *key,
                         void *ctx)
{
    WASMExecEnv *exec_env = wasm_clusters_search_exec_env(module_inst);

    if (exec_env == NULL) {
        /* Maybe threads have not been started yet. */
        wasm_runtime_set_context(module_inst, key, ctx);
    }
    else {
        WASMCluster *cluster;
        struct inst_set_context_data data;
        data.key = key;
        data.ctx = ctx;

        cluster = wasm_exec_env_get_cluster(exec_env);
        bh_assert(cluster);

        cluster_lock_thread_list(cluster, exec_env);
        traverse_list(&cluster->exec_env_list, set_context_visitor, &data);
        cluster_unlock_thread_list(cluster);
    }
}
#endif /* WASM_ENABLE_MODULE_INST_CONTEXT != 0 */

bool
wasm_cluster_is_thread_terminated(WASMExecEnv *exec_env)
{
    bool is_thread_terminated;

    if (!clusters_have_exec_env(exec_env)) {
        return true;
    }

    os_mutex_lock(&exec_env->cluster->thread_state_lock);
    is_thread_terminated =
        (exec_env->suspend_flags.flags & WASM_SUSPEND_FLAG_TERMINATE) ? true
                                                                      : false;
    os_mutex_unlock(&exec_env->cluster->thread_state_lock);

    return is_thread_terminated;
}

void
wasm_cluster_change_curr_thread_to_running()
{
    WASMExecEnv *self = get_exec_env_of_current_thread();

    bh_assert(self);
    wasm_thread_change_to_running(self);
}

void
wasm_cluster_change_curr_thread_to_safe()
{
    WASMExecEnv *self = get_exec_env_of_current_thread();

    bh_assert(self);
    wasm_thread_change_to_safe(self, WASM_THREAD_VMWAIT);
}

void
exception_lock(WASMModuleInstance *module_inst)
{
    /*
     * Note: this lock could be per module instance if desirable.
     * We can revisit on AOT version bump.
     * It probably doesn't matter though because the exception handling
     * logic should not be executed too frequently anyway.
     */
    os_mutex_lock(&_exception_lock);
}

void
exception_unlock(WASMModuleInstance *module_inst)
{
    os_mutex_unlock(&_exception_lock);
}
