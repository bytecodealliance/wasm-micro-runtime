/*
 * Copyright (C) 2021 Ant Group.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "debug_engine.h"

#include "bh_log.h"
#include "gdbserver.h"
#include "platform_api_extension.h"
#include "wasm_interp.h"
#include "wasm_opcode.h"
#include "wasm_runtime.h"

static uint8 break_instr[] = { DEBUG_OP_BREAK };

typedef struct WASMDebugEngine {
    struct WASMDebugEngine *next;
    WASMDebugControlThread *control_thread;
    char ip_addr[128];
    int platform_port;
    int process_base_port;
    bh_list debug_instance_list;
    bool active;
} WASMDebugEngine;

static WASMDebugEngine *g_debug_engine;

static bool
should_stop(WASMDebugControlThread *control_thread)
{
    return control_thread->status != RUNNING;
}

static void *
control_thread_routine(void *arg)
{
    WASMDebugInstance *debug_inst = (WASMDebugInstance *)arg;
    WASMDebugControlThread *control_thread = NULL;
    WASMCluster *cluster = NULL;
    WASMExecEnv *exec_env;
    bh_assert(debug_inst);

    control_thread = debug_inst->control_thread;
    bh_assert(control_thread);

    cluster = debug_inst->cluster;
    bh_assert(cluster);

    exec_env = bh_list_first_elem(&cluster->exec_env_list);
    bh_assert(exec_env);

    os_mutex_lock(&exec_env->wait_lock);

    control_thread->status = RUNNING;

    debug_inst->id = g_debug_engine->debug_instance_list.len + 1;

    control_thread->debug_engine = g_debug_engine;
    control_thread->debug_instance = debug_inst;
    strcpy(control_thread->ip_addr, g_debug_engine->ip_addr);
    control_thread->port =
        g_debug_engine->process_base_port + debug_inst->id;

    LOG_WARNING("control thread of debug object %p start at %s:%d\n",
                debug_inst, control_thread->ip_addr, control_thread->port);

    control_thread->server =
      wasm_launch_gdbserver(control_thread->ip_addr, control_thread->port);
    if (!control_thread->server) {
        LOG_ERROR("Failed to create debug server\n");
        os_cond_signal(&exec_env->wait_cond);
        os_mutex_unlock(&exec_env->wait_lock);
        return NULL;
    }

    control_thread->server->thread = control_thread;

    /* control thread ready, notify main thread */
    os_cond_signal(&exec_env->wait_cond);
    os_mutex_unlock(&exec_env->wait_lock);

    while (true) {
        os_mutex_lock(&control_thread->wait_lock);
        if (!should_stop(control_thread)) {
            if (!wasm_gdbserver_handle_packet(control_thread->server)) {
                control_thread->status = STOPPED;
            }
        }
        else {
            os_mutex_unlock(&control_thread->wait_lock);
            break;
        }
        os_mutex_unlock(&control_thread->wait_lock);
    }

    LOG_VERBOSE("control thread of debug object %p stop\n", debug_inst);
    return NULL;
}

static WASMDebugControlThread *
wasm_debug_control_thread_create(WASMDebugInstance *debug_instance)
{
    WASMDebugControlThread *control_thread;
    WASMCluster *cluster = debug_instance->cluster;
    WASMExecEnv *exec_env;
    bh_assert(cluster);

    exec_env = bh_list_first_elem(&cluster->exec_env_list);
    bh_assert(exec_env);

    if (!(control_thread =
            wasm_runtime_malloc(sizeof(WASMDebugControlThread)))) {
        LOG_ERROR("WASM Debug Engine error: failed to allocate memory");
        return NULL;
    }
    memset(control_thread, 0, sizeof(WASMDebugControlThread));

    if (os_mutex_init(&control_thread->wait_lock) != 0)
        goto fail;

    debug_instance->control_thread = control_thread;

    os_mutex_lock(&exec_env->wait_lock);

    if (0 != os_thread_create(&control_thread->tid, control_thread_routine,
                              debug_instance, APP_THREAD_STACK_SIZE_MAX)) {
        os_mutex_unlock(&control_thread->wait_lock);
        goto fail1;
    }

    /* wait until the debug control thread ready */
    os_cond_wait(&exec_env->wait_cond, &exec_env->wait_lock);
    os_mutex_unlock(&exec_env->wait_lock);
    if (!control_thread->server)
        goto fail1;

    /* create control thread success, append debug instance to debug engine */
    bh_list_insert(&g_debug_engine->debug_instance_list, debug_instance);
    wasm_cluster_send_signal_all(debug_instance->cluster, WAMR_SIG_STOP);

    return control_thread;

fail1:
    os_mutex_destroy(&control_thread->wait_lock);
fail:
    wasm_runtime_free(control_thread);
    return NULL;
}

static void
wasm_debug_control_thread_destroy(WASMDebugInstance *debug_instance)
{
    WASMDebugControlThread *control_thread = debug_instance->control_thread;
    LOG_VERBOSE("control thread of debug object %p stop at %s:%d\n",
                debug_instance, control_thread->ip_addr,
                control_thread->port);
    control_thread->status = STOPPED;
    os_mutex_lock(&control_thread->wait_lock);
    wasm_close_gdbserver(control_thread->server);
    os_mutex_unlock(&control_thread->wait_lock);
    os_thread_join(control_thread->tid, NULL);
    wasm_runtime_free(control_thread->server);

    os_mutex_destroy(&control_thread->wait_lock);
    wasm_runtime_free(control_thread);
}

static WASMDebugEngine *
wasm_debug_engine_create()
{
    WASMDebugEngine *engine;

    if (!(engine = wasm_runtime_malloc(sizeof(WASMDebugEngine)))) {
        LOG_ERROR("WASM Debug Engine error: failed to allocate memory");
        return NULL;
    }
    memset(engine, 0, sizeof(WASMDebugEngine));

    /* TODO: support Wasm platform in LLDB */
    /*
    engine->control_thread =
        wasm_debug_control_thread_create((WASMDebugObject *)engine);
    engine->control_thread->debug_engine = (WASMDebugObject *)engine;
    engine->control_thread->debug_instance = NULL;
    sprintf(engine->control_thread->ip_addr, "127.0.0.1");
    engine->control_thread->port = 1234;
    */

    bh_list_init(&engine->debug_instance_list);
    return engine;
}

bool
wasm_debug_engine_init(char *ip_addr, int platform_port, int process_port)
{
    if (g_debug_engine == NULL)
        g_debug_engine = wasm_debug_engine_create();

    if (g_debug_engine) {
        process_port -= 1;
        g_debug_engine->platform_port =
          platform_port > 0 ? platform_port : 1234;
        g_debug_engine->process_base_port =
          process_port > 0 ? process_port : 6169;
        if (ip_addr)
            sprintf(g_debug_engine->ip_addr, "%s", ip_addr);
        else
            sprintf(g_debug_engine->ip_addr, "%s", "127.0.0.1");
        g_debug_engine->active = true;
    }

    return g_debug_engine != NULL ? true : false;
}

void
wasm_debug_set_engine_active(bool active)
{
    if (g_debug_engine) {
        g_debug_engine->active = active;
    }
}

bool
wasm_debug_get_engine_active(void)
{
    if (g_debug_engine) {
        return g_debug_engine->active;
    }
    return false;
}

void
wasm_debug_engine_destroy()
{
    if (g_debug_engine) {
        wasm_runtime_free(g_debug_engine);
        g_debug_engine = NULL;
    }
}

/* A debug Instance is a debug "process" in gdb remote protocol
   and bound to a runtime cluster */
WASMDebugInstance *
wasm_debug_instance_create(WASMCluster *cluster)
{
    WASMDebugInstance *instance;
    WASMExecEnv *exec_env;

    if (!g_debug_engine || !g_debug_engine->active) {
        return NULL;
    }

    if (!(instance = wasm_runtime_malloc(sizeof(WASMDebugInstance)))) {
        LOG_ERROR("WASM Debug Engine error: failed to allocate memory");
        return NULL;
    }
    memset(instance, 0, sizeof(WASMDebugInstance));
    bh_list_init(&instance->break_point_list);

    instance->cluster = cluster;
    exec_env = bh_list_first_elem(&cluster->exec_env_list);
    bh_assert(exec_env);

    instance->current_tid = exec_env->handle;

    if (!wasm_debug_control_thread_create(instance)) {
        LOG_ERROR("WASM Debug Engine error: failed to create control thread");
        wasm_runtime_free(instance);
        return NULL;
    }

    return instance;
}

static WASMDebugInstance *
wasm_cluster_get_debug_instance(WASMDebugEngine *engine, WASMCluster *cluster)
{
    WASMDebugInstance *instance =
      bh_list_first_elem(&engine->debug_instance_list);
    while (instance) {
        if (instance->cluster == cluster)
            return instance;
        instance = bh_list_elem_next(instance);
    }
    return instance;
}

static void
wasm_debug_instance_destroy_breakpoints(WASMDebugInstance *instance)
{
    WASMDebugBreakPoint *breakpoint, *next_bp;

    breakpoint = bh_list_first_elem(&instance->break_point_list);
    while (breakpoint) {
        next_bp = bh_list_elem_next(breakpoint);

        bh_list_remove(&instance->break_point_list, breakpoint);
        wasm_runtime_free(breakpoint);

        breakpoint = next_bp;
    }
}

void
wasm_debug_instance_destroy(WASMCluster *cluster)
{
    WASMDebugInstance *instance = NULL;

    if (!g_debug_engine) {
        return;
    }

    instance = wasm_cluster_get_debug_instance(g_debug_engine, cluster);
    if (instance) {
        /* destroy control thread */
        wasm_debug_control_thread_destroy(instance);
        bh_list_remove(&g_debug_engine->debug_instance_list, instance);

        /* destroy all breakpoints */
        wasm_debug_instance_destroy_breakpoints(instance);

        wasm_runtime_free(instance);
    }
}

static WASMExecEnv *
wasm_debug_instance_get_current_env(WASMDebugInstance *instance)
{
    WASMExecEnv *exec_env = NULL;

    if (instance) {
        exec_env = bh_list_first_elem(&instance->cluster->exec_env_list);
        while (exec_env) {
            if (exec_env->handle == instance->current_tid)
                break;
            exec_env = bh_list_elem_next(exec_env);
        }
    }
    return exec_env;
}

#if WASM_ENABLE_LIBC_WASI != 0
bool
wasm_debug_instance_get_current_object_name(WASMDebugInstance *instance,
                                            char name_buffer[],
                                            int len)
{
    WASMExecEnv *exec_env;
    WASIArguments *wasi_args;
    WASMModuleInstance *module_inst;

    if (!instance)
        return false;

    exec_env = bh_list_first_elem(&instance->cluster->exec_env_list);
    if (!exec_env)
        return false;

    module_inst = (WASMModuleInstance *)exec_env->module_inst;
    wasi_args = &module_inst->module->wasi_args;
    if (wasi_args && wasi_args->argc > 0) {
        char *argv_name = wasi_args->argv[0];
        int name_len = strlen(argv_name);
        printf("the module name is %s\n", argv_name);
        if (len - 1 >= name_len)
            strcpy(name_buffer, argv_name);
        else
            strcpy(name_buffer, argv_name + (name_len + 1 - len));
        return true;
    }
    return false;
}
#endif

uint64
wasm_debug_instance_get_pid(WASMDebugInstance *instance)
{
    if (instance != NULL) {
        return (uint64)instance->id;
    }
    return (uint64)0;
}

uint64
wasm_debug_instance_get_tid(WASMDebugInstance *instance)
{
    if (instance != NULL) {
        return (uint64)instance->current_tid;
    }
    return (uint64)0;
}

int
wasm_debug_instance_get_tids(WASMDebugInstance *instance,
                             uint64 tids[], int len)
{
    WASMExecEnv *exec_env;
    int i = 0;

    if (!instance)
        return 0;

    exec_env = bh_list_first_elem(&instance->cluster->exec_env_list);
    while (exec_env && i < len) {
        tids[i++] = exec_env->handle;
        exec_env = bh_list_elem_next(exec_env);
    }
    LOG_VERBOSE("find %d tids\n", i);
    return i;
}

uint64
wasm_debug_instance_wait_thread(WASMDebugInstance *instance,
                                uint64 tid, uint32 *status)
{
    WASMExecEnv *exec_env;
    WASMExecEnv *last_exec_env = NULL;

    exec_env = bh_list_first_elem(&instance->cluster->exec_env_list);
    while (exec_env) {
        last_exec_env = exec_env;
        if (instance->current_tid != 0
            && last_exec_env->handle == instance->current_tid) {
            break;
        }
        exec_env = bh_list_elem_next(exec_env);
    }

    if (last_exec_env) {
        wasm_cluster_wait_thread_status(last_exec_env, status);
        if (instance->current_tid == 0)
            instance->current_tid = last_exec_env->handle;
        return last_exec_env->handle;
    }
    else {
        *status = ~0;
        return 0;
    }
}

void
wasm_debug_instance_set_cur_thread(WASMDebugInstance *instance, uint64 tid)
{
    instance->current_tid = tid;
}

uint64
wasm_debug_instance_get_pc(WASMDebugInstance *instance)
{
    WASMExecEnv *exec_env;

    if (!instance)
        return 0;

    exec_env = wasm_debug_instance_get_current_env(instance);
    if ((exec_env->cur_frame != NULL)
        && (exec_env->cur_frame->ip != NULL)) {
        WASMModuleInstance *module_inst =
          (WASMModuleInstance *)exec_env->module_inst;
        return WASM_ADDR(
          WasmObj, instance->id,
          (exec_env->cur_frame->ip - module_inst->module->load_addr));
    }
    return 0;
}

uint64
wasm_debug_instance_get_load_addr(WASMDebugInstance *instance)
{
    WASMExecEnv *exec_env;

    if (!instance)
        return WASM_ADDR(WasmInvalid, 0, 0);

    exec_env = bh_list_first_elem(&instance->cluster->exec_env_list);
    if (exec_env) {
        return WASM_ADDR(WasmObj, instance->id, 0);
    }

    return WASM_ADDR(WasmInvalid, 0, 0);
}

WASMDebugMemoryInfo *
wasm_debug_instance_get_memregion(WASMDebugInstance *instance, uint64 addr)
{
    WASMDebugMemoryInfo *mem_info;
    WASMExecEnv *exec_env;
    WASMModuleInstance *module_inst;
    WASMMemoryInstance *memory;
    uint32 num_bytes_per_page;
    uint32 linear_mem_size = 0;

    if (!instance)
        return NULL;

    exec_env = bh_list_first_elem(&instance->cluster->exec_env_list);
    if (!exec_env)
        return NULL;

    if (!(mem_info = wasm_runtime_malloc(sizeof(WASMDebugMemoryInfo)))) {
        LOG_ERROR("WASM Debug Engine error: failed to allocate memory");
        return NULL;
    }
    memset(mem_info, 0, sizeof(WASMDebugMemoryInfo));
    mem_info->start = WASM_ADDR(WasmInvalid, 0, 0);
    mem_info->size = 0;
    mem_info->name[0] = '\0';
    mem_info->permisson[0] = '\0';

    module_inst = (WASMModuleInstance *)exec_env->module_inst;

    switch (WASM_ADDR_TYPE(addr)) {
        case WasmObj:
            if (WASM_ADDR_OFFSET(addr) < module_inst->module->load_size) {
                mem_info->start = WASM_ADDR(WasmObj, instance->id, 0);
                mem_info->size = module_inst->module->load_size;
                sprintf(mem_info->name, "%s", "module");
                sprintf(mem_info->permisson, "%s", "rx");
            }
            break;
        case WasmMemory: {
            memory = module_inst->default_memory;

            if (memory) {
                num_bytes_per_page = memory->num_bytes_per_page;
                linear_mem_size = num_bytes_per_page * memory->cur_page_count;
            }
            if (WASM_ADDR_OFFSET(addr) < linear_mem_size) {
                mem_info->start = WASM_ADDR(WasmMemory, instance->id, 0);
                mem_info->size = linear_mem_size;
                sprintf(mem_info->name, "%s", "memory");
                sprintf(mem_info->permisson, "%s", "rw");
            }
            break;
        }
        default:
            mem_info->start = WASM_ADDR(WasmInvalid, 0, 0);
            mem_info->size = 0;
    }
    return mem_info;
}

void
wasm_debug_instance_destroy_memregion(WASMDebugInstance *instance,
                                      WASMDebugMemoryInfo *mem_info)
{
    wasm_runtime_free(mem_info);
}

bool
wasm_debug_instance_get_obj_mem(WASMDebugInstance *instance,
                                uint64 offset, char *buf, uint64 *size)
{
    WASMExecEnv *exec_env;
    WASMModuleInstance *module_inst;

    if (!instance)
        return false;

    exec_env = bh_list_first_elem(&instance->cluster->exec_env_list);
    if (!exec_env)
        return false;

    module_inst = (WASMModuleInstance *)exec_env->module_inst;

    if (offset + *size > module_inst->module->load_size) {
        LOG_VERBOSE("wasm_debug_instance_get_data_mem size over flow!\n");
        *size = module_inst->module->load_size >= offset
                  ? module_inst->module->load_size - offset
                  : 0;
    }

    bh_memcpy_s(buf, *size, module_inst->module->load_addr + offset, *size);

    WASMDebugBreakPoint *breakpoint =
      bh_list_first_elem(&instance->break_point_list);

    while (breakpoint) {
        if (offset <= breakpoint->addr && breakpoint->addr < offset + *size) {
            bh_memcpy_s(buf + (breakpoint->addr - offset), sizeof(break_instr),
                        &breakpoint->orignal_data, sizeof(break_instr));
        }
        breakpoint = bh_list_elem_next(breakpoint);
    }

    WASMFastOPCodeNode *fast_opcode =
      bh_list_first_elem(&module_inst->module->fast_opcode_list);
    while (fast_opcode) {
        if (offset <= fast_opcode->offset
            && fast_opcode->offset < offset + *size) {
            *(uint8 *)(buf + (fast_opcode->offset - offset)) =
              fast_opcode->orig_op;
        }
        fast_opcode = bh_list_elem_next(fast_opcode);
    }

    return true;
}

bool
wasm_debug_instance_get_linear_mem(WASMDebugInstance *instance,
                                   uint64 offset, char *buf, uint64 *size)
{
    WASMExecEnv *exec_env;
    WASMModuleInstance *module_inst;
    WASMMemoryInstance *memory;
    uint32 num_bytes_per_page;
    uint32 linear_mem_size;

    if (!instance)
        return false;

    exec_env = wasm_debug_instance_get_current_env(instance);
    if (!exec_env)
        return false;

    module_inst = (WASMModuleInstance *)exec_env->module_inst;
    memory = module_inst->default_memory;
    if (memory) {
        num_bytes_per_page = memory->num_bytes_per_page;
        linear_mem_size = num_bytes_per_page * memory->cur_page_count;
        if (offset + *size > linear_mem_size) {
            LOG_VERBOSE(
              "wasm_debug_instance_get_linear_mem size over flow!\n");
            *size = linear_mem_size >= offset ? linear_mem_size - offset : 0;
        }
        bh_memcpy_s(buf, *size, memory->memory_data + offset, *size);
        return true;
    }
    return false;
}

bool
wasm_debug_instance_set_linear_mem(WASMDebugInstance *instance,
                                   uint64 offset, char *buf, uint64 *size)
{
    WASMExecEnv *exec_env;
    WASMModuleInstance *module_inst;
    WASMMemoryInstance *memory;
    uint32 num_bytes_per_page;
    uint32 linear_mem_size;

    if (!instance)
        return false;

    exec_env = wasm_debug_instance_get_current_env(instance);
    if (!exec_env)
        return false;

    module_inst = (WASMModuleInstance *)exec_env->module_inst;
    memory = module_inst->default_memory;
    if (memory) {
        num_bytes_per_page = memory->num_bytes_per_page;
        linear_mem_size = num_bytes_per_page * memory->cur_page_count;
        if (offset + *size > linear_mem_size) {
            LOG_VERBOSE(
              "wasm_debug_instance_get_linear_mem size over flow!\n");
            *size = linear_mem_size >= offset ? linear_mem_size - offset : 0;
        }
        bh_memcpy_s(memory->memory_data + offset, *size, buf, *size);
        return true;
    }
    return false;
}

bool
wasm_debug_instance_get_mem(WASMDebugInstance *instance,
                            uint64 addr, char *buf, uint64 *size)
{
    switch (WASM_ADDR_TYPE(addr)) {
        case WasmMemory:
            return wasm_debug_instance_get_linear_mem(
              instance, WASM_ADDR_OFFSET(addr), buf, size);
            break;
        case WasmObj:
            return wasm_debug_instance_get_obj_mem(
              instance, WASM_ADDR_OFFSET(addr), buf, size);
            break;
        default:
            return false;
    }
}

bool
wasm_debug_instance_set_mem(WASMDebugInstance *instance,
                            uint64 addr, char *buf, uint64 *size)
{
    switch (WASM_ADDR_TYPE(addr)) {
        case WasmMemory:
            return wasm_debug_instance_set_linear_mem(
              instance, WASM_ADDR_OFFSET(addr), buf, size);
            break;
        case WasmObj:
        default:
            return false;
    }
}

WASMDebugInstance *
wasm_exec_env_get_instance(WASMExecEnv *exec_env)
{
    WASMDebugInstance *instance = NULL;
    bh_assert(g_debug_engine);

    instance = bh_list_first_elem(&g_debug_engine->debug_instance_list);
    while (instance) {
        if (instance->cluster == exec_env->cluster)
            break;
        instance = bh_list_elem_next(instance);
    }
    return instance;
}

int
wasm_debug_instance_get_call_stack_pcs(WASMDebugInstance *instance,
                                       uint64 tid, uint64 buf[], uint64 size)
{
    WASMExecEnv *exec_env;
    struct WASMInterpFrame *frame;
    uint64 i = 0;

    if (!instance)
        return 0;

    exec_env = bh_list_first_elem(&instance->cluster->exec_env_list);
    while (exec_env) {
        if (exec_env->handle == tid) {
            WASMModuleInstance *module_inst =
              (WASMModuleInstance *)exec_env->module_inst;
            frame = exec_env->cur_frame;
            while (frame && i < size) {
                if (frame->ip != NULL) {
                    buf[i++] =
                      WASM_ADDR(WasmObj, instance->id,
                                (frame->ip - module_inst->module->load_addr));
                }
                frame = frame->prev_frame;
            }
            return i;
        }
        exec_env = bh_list_elem_next(exec_env);
    }
    return 0;
}

bool
wasm_debug_instance_add_breakpoint(WASMDebugInstance *instance,
                                   uint64 addr, uint64 length)
{
    WASMExecEnv *exec_env;
    WASMModuleInstance *module_inst;
    uint64 offset;

    if (!instance)
        return false;

    exec_env = bh_list_first_elem(&instance->cluster->exec_env_list);
    if (!exec_env)
        return false;

    module_inst = (WASMModuleInstance *)exec_env->module_inst;
    if (WASM_ADDR_TYPE(addr) != WasmObj)
        return false;

    offset = WASM_ADDR_OFFSET(addr);

    if (length >= sizeof(break_instr)) {
        if (offset + sizeof(break_instr) <= module_inst->module->load_size) {
            WASMDebugBreakPoint *breakpoint;
            if (!(breakpoint =
                    wasm_runtime_malloc(sizeof(WASMDebugBreakPoint)))) {
                LOG_ERROR(
                  "WASM Debug Engine error: failed to allocate memory");
                return false;
            }
            memset(breakpoint, 0, sizeof(WASMDebugBreakPoint));
            breakpoint->addr = offset;
            /* TODO: how to if more than one breakpoints are set
                     at the same addr? */
            bh_memcpy_s(&breakpoint->orignal_data,
                        (uint32)sizeof(break_instr),
                        module_inst->module->load_addr + offset,
                        (uint32)sizeof(break_instr));

            bh_memcpy_s(module_inst->module->load_addr + offset,
                        (uint32)sizeof(break_instr),
                        break_instr,
                        (uint32)sizeof(break_instr));

            bh_list_insert(&instance->break_point_list, breakpoint);
            return true;
        }
    }
    return false;
}

bool
wasm_debug_instance_remove_breakpoint(WASMDebugInstance *instance,
                                      uint64 addr, uint64 length)
{
    WASMExecEnv *exec_env;
    WASMModuleInstance *module_inst;
    uint64 offset;

    if (!instance)
        return false;

    exec_env = bh_list_first_elem(&instance->cluster->exec_env_list);
    if (!exec_env)
        return false;

    module_inst = (WASMModuleInstance *)exec_env->module_inst;

    if (WASM_ADDR_TYPE(addr) != WasmObj)
        return false;
    offset = WASM_ADDR_OFFSET(addr);

    if (length >= sizeof(break_instr)) {
        if (offset + sizeof(break_instr) <= module_inst->module->load_size) {
            WASMDebugBreakPoint *breakpoint =
              bh_list_first_elem(&instance->break_point_list);
            while (breakpoint) {
                WASMDebugBreakPoint *next_break =
                  bh_list_elem_next(breakpoint);
                if (breakpoint->addr == offset) {
                    /* TODO: how to if more than one breakpoints are set
                       at the same addr? */
                    bh_memcpy_s(module_inst->module->load_addr + offset,
                                (uint32)sizeof(break_instr),
                                &breakpoint->orignal_data,
                                (uint32)sizeof(break_instr));
                    bh_list_remove(&instance->break_point_list, breakpoint);
                    wasm_runtime_free(breakpoint);
                }
                breakpoint = next_break;
            }
        }
    }
    return true;
}

bool
wasm_debug_instance_continue(WASMDebugInstance *instance)
{
    WASMExecEnv *exec_env;

    if (!instance)
        return false;

    exec_env = bh_list_first_elem(&instance->cluster->exec_env_list);
    if (!exec_env)
        return false;

    while (exec_env) {
        wasm_cluster_thread_continue(exec_env);
        exec_env = bh_list_elem_next(exec_env);
    }
    return true;
}

bool
wasm_debug_instance_kill(WASMDebugInstance *instance)
{
    WASMExecEnv *exec_env;

    if (!instance)
        return false;

    exec_env = bh_list_first_elem(&instance->cluster->exec_env_list);
    if (!exec_env)
        return false;

    while (exec_env) {
        wasm_cluster_thread_send_signal(exec_env, WAMR_SIG_TERM);
        exec_env = bh_list_elem_next(exec_env);
    }
    return true;
}

bool
wasm_debug_instance_singlestep(WASMDebugInstance *instance, uint64 tid)
{
    WASMExecEnv *exec_env;

    if (!instance)
        return false;

    exec_env = bh_list_first_elem(&instance->cluster->exec_env_list);
    if (!exec_env)
        return false;

    while (exec_env) {
        if (exec_env->handle == tid || tid == (uint64)~0) {
            wasm_cluster_thread_send_signal(exec_env, WAMR_SIG_SINGSTEP);
            wasm_cluster_thread_step(exec_env);
        }
        exec_env = bh_list_elem_next(exec_env);
    }
    return true;
}

bool
wasm_debug_instance_get_local(WASMDebugInstance *instance,
                              int frame_index, int local_index,
                              char buf[], int *size)
{
    WASMExecEnv *exec_env;
    struct WASMInterpFrame *frame;
    WASMFunctionInstance *cur_func;
    uint8 local_type = 0xFF;
    uint32 local_offset;
    int param_count;
    int fi = 0;

    if (!instance)
        return false;

    exec_env = wasm_debug_instance_get_current_env(instance);
    if (!exec_env)
        return false;

    frame = exec_env->cur_frame;
    while (frame && fi++ != frame_index) {
        frame = frame->prev_frame;
    }

    if (!frame)
        return false;
    cur_func = frame->function;
    if (!cur_func)
        return false;

    param_count = cur_func->param_count;

    if (local_index >= param_count + cur_func->local_count)
        return false;

    local_offset = cur_func->local_offsets[local_index];
    if (local_index < param_count)
        local_type = cur_func->param_types[local_index];
    else if (local_index < cur_func->local_count + param_count)
        local_type = cur_func->local_types[local_index - param_count];

    switch (local_type) {
        case VALUE_TYPE_I32:
        case VALUE_TYPE_F32:
            *size = 4;
            bh_memcpy_s(buf, 4, (char *)(frame->lp + local_offset), 4);
            break;
        case VALUE_TYPE_I64:
        case VALUE_TYPE_F64:
            *size = 8;
            bh_memcpy_s(buf, 8, (char *)(frame->lp + local_offset), 8);
            break;
        default:
            *size = 0;
            break;
    }
    return true;
}

bool
wasm_debug_instance_get_global(WASMDebugInstance *instance,
                               int frame_index, int global_index,
                               char buf[], int *size)
{
    WASMExecEnv *exec_env;
    struct WASMInterpFrame *frame;
    WASMModuleInstance *module_inst;
    WASMGlobalInstance *globals, *global;
    uint8 *global_addr;
    uint8 global_type = 0xFF;
    uint8 *global_data;
    int fi = 0;

    if (!instance)
        return false;

    exec_env = wasm_debug_instance_get_current_env(instance);
    if (!exec_env)
        return false;

    frame = exec_env->cur_frame;
    while (frame && fi++ != frame_index) {
        frame = frame->prev_frame;
    }

    if (!frame)
        return false;

    module_inst = (WASMModuleInstance *)exec_env->module_inst;
    global_data = module_inst->global_data;
    globals = module_inst->globals;

    if ((global_index < 0)
        || ((uint32)global_index >= module_inst->global_count)) {
        return false;
    }
    global = globals + global_index;

#if WASM_ENABLE_MULTI_MODULE == 0
    global_addr = global_data + global->data_offset;
#else
    global_addr = global->import_global_inst
                    ? global->import_module_inst->global_data
                        + global->import_global_inst->data_offset
                    : global_data + global->data_offset;
#endif
    global_type = global->type;

    switch (global_type) {
        case VALUE_TYPE_I32:
        case VALUE_TYPE_F32:
            *size = 4;
            bh_memcpy_s(buf, 4, (char *)(global_addr), 4);
            break;
        case VALUE_TYPE_I64:
        case VALUE_TYPE_F64:
            *size = 8;
            bh_memcpy_s(buf, 8, (char *)(global_addr), 8);
            break;
        default:
            *size = 0;
            break;
    }
    return true;
}

uint64
wasm_debug_instance_mmap(WASMDebugInstance *instance,
                         uint32 size, int map_port)
{
    WASMExecEnv *exec_env;
    WASMModuleInstance *module_inst;
    uint32 offset;
    void *native_addr;
    (void)map_port;

    if (!instance)
        return 0;

    exec_env = wasm_debug_instance_get_current_env(instance);
    if (!exec_env)
        return 0;

    module_inst = (WASMModuleInstance *)exec_env->module_inst;

    /* TODO: malloc in wasi libc maybe not be thread safe, we hope LLDB will
             always ask for memory when threads stopped */
    offset = wasm_runtime_module_malloc((wasm_module_inst_t)module_inst, size,
                                        &native_addr);
    if (!offset)
        LOG_WARNING("the memory may be not enough for debug, try use larger "
                    "--heap-size");
    return WASM_ADDR(WasmMemory, 0, offset);
}

bool
wasm_debug_instance_ummap(WASMDebugInstance *instance, uint64 addr)
{
    WASMExecEnv *exec_env;
    WASMModuleInstance *module_inst;
    uint32 offset;

    if (!instance)
        return false;

    exec_env = wasm_debug_instance_get_current_env(instance);
    if (!exec_env)
        return false;

    module_inst = (WASMModuleInstance *)exec_env->module_inst;
    if (WASM_ADDR_TYPE(addr) == WasmMemory) {
        offset = WASM_ADDR_OFFSET(addr);
        wasm_runtime_module_free((wasm_module_inst_t)module_inst, offset);
        return true;
    }
    return false;
}
