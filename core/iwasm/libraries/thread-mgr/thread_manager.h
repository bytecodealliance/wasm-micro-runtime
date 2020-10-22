/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _THREAD_MANAGER_H
#define _THREAD_MANAGER_H

#include "bh_common.h"
#include "bh_log.h"
#include "wasm_export.h"
#include "../interpreter/wasm.h"
#include "../common/wasm_runtime_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WASMCluster
{
    struct WASMCluster *next;

    korp_mutex lock;
    bh_list exec_env_list;

    /* The aux stack of a module with shared memory will be
        divided into several segments. This array store the
        stack top of different segments */
    uint32 *stack_tops;
    /* Size of every stack segment */
    uint32 stack_size;
    /* Record which segments are occupied */
    bool *stack_segment_occupied;
} WASMCluster;

void wasm_cluster_set_max_thread_num(uint32 num);

bool
thread_manager_init();

void
thread_manager_destroy();

/* Create cluster */
WASMCluster *
wasm_cluster_create(WASMExecEnv *exec_env);

/* Destroy cluster */
void
wasm_cluster_destroy(WASMCluster *cluster);

/* Get the cluster of the current exec_env */
WASMCluster*
wasm_exec_env_get_cluster(WASMExecEnv *exec_env);

int32
wasm_cluster_create_thread(WASMExecEnv *exec_env,
                           wasm_module_inst_t module_inst,
                           void* (*thread_routine)(void *),
                           void *arg);

int32
wasm_cluster_join_thread(WASMExecEnv *exec_env, void **ret_val);

int32
wasm_cluster_detach_thread(WASMExecEnv *exec_env);

int32
wasm_cluster_cancel_thread(WASMExecEnv *exec_env);

void
wasm_cluster_exit_thread(WASMExecEnv *exec_env, void *retval);

bool
wasm_cluster_register_destroy_callback(void (*callback)(WASMCluster *));

void
wasm_cluster_cancel_all_callbacks();

void
wasm_cluster_suspend_all(WASMCluster *cluster);

void
wasm_cluster_suspend_all_except_self(WASMCluster *cluster,
                                     WASMExecEnv *exec_env);

void
wasm_cluster_suspend_thread(WASMExecEnv *exec_env);

void
wasm_cluster_resume_thread(WASMExecEnv *exec_env);

void
wasm_cluster_resume_all(WASMCluster *cluster);

void
wasm_cluster_terminate_all(WASMCluster *cluster);

void
wasm_cluster_terminate_all_except_self(WASMCluster *cluster,
                                       WASMExecEnv *exec_env);

bool
wasm_cluster_add_exec_env(WASMCluster *cluster, WASMExecEnv *exec_env);

bool
wasm_cluster_del_exec_env(WASMCluster *cluster, WASMExecEnv *exec_env);

void
wasm_cluster_spread_exception(WASMExecEnv *exec_env);

WASMExecEnv *
wasm_cluster_spawn_exec_env(WASMExecEnv *exec_env);

void
wasm_cluster_destroy_spawned_exec_env(WASMExecEnv *exec_env);

#ifdef __cplusplus
}
#endif

#endif /* end of _THREAD_MANAGER_H */
