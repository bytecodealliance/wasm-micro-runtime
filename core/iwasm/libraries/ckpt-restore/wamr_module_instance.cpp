/*
 * The WebAssembly Live Migration Project
 *
 *  By: Aibo Hu
 *      Yiwei Yang
 *      Brian Zhao
 *      Andrew Quinn
 *
 *  Copyright 2024 Regents of the Univeristy of California
 *  UC Santa Cruz Sluglab.
 */

#include "wamr_module_instance.h"
#include "aot_runtime.h"
#include "wamr.h"

extern WAMRInstance *wamr;

void
WAMRModuleInstance::dump_impl(WASMModuleInstance *env)
{
    // The first thread will dump the memory
    if (((WAMRExecEnv *)this)->cur_count
        == ((uint64_t)wamr->exec_env->handle)) {
        for (int i = 0; i < env->memory_count; i++) {
            auto local_mem = WAMRMemoryInstance();
            dump(&local_mem, env->memories[i]);
            memories.push_back(local_mem);
        }
        for (int i = 0; i < env->table_count; i++) {
            LOG_DEBUG("Dumping table {}", env->tables[i]->cur_size);
            auto table =
                WAMRTableInstance{ .elem_type = (*env->tables[i]).elem_type,
                                   .cur_size = (*env->tables[i]).cur_size,
                                   .max_size = (*env->tables[i]).max_size,
                                   .elems = { (*env->tables[i]).elems[0] } };
            tables.push_back(table);
        }
    }
    global_data = std::vector<uint8>(env->global_data,
                                     env->global_data + env->global_data_size);
    dump(&wasi_ctx, &env->module->wasi_args);
    LOG_DEBUG("Dumped global data ptr: {}", ((void *)env->global_data));
    if (wamr->is_aot) {
        auto module = (AOTModule *)env->module;
        aux_data_end_global_index = module->aux_data_end_global_index;
        aux_data_end = module->aux_data_end;
        aux_heap_base_global_index = module->aux_heap_base_global_index;
        aux_heap_base = module->aux_heap_base;
        aux_stack_top_global_index = module->aux_stack_top_global_index;
        aux_stack_bottom = module->aux_stack_bottom;
        aux_stack_size = module->aux_stack_size;
    }
    else {
        auto module = env->module;
        aux_data_end_global_index = module->aux_data_end_global_index;
        aux_data_end = module->aux_data_end;
        aux_heap_base_global_index = module->aux_heap_base_global_index;
        aux_heap_base = module->aux_heap_base;
        aux_stack_top_global_index = module->aux_stack_top_global_index;
        aux_stack_bottom = module->aux_stack_bottom;
        aux_stack_size = module->aux_stack_size;
    }
    dump(&global_table_data, env->global_table_data.memory_instances);
}

void
WAMRModuleInstance::restore_impl(WASMModuleInstance *env)
{
    if (!wamr->tmp_buf) {
        env->memory_count = memories.size();
        for (int i = 0; i < env->memory_count; i++) {
            restore(&memories[i], env->memories[i]);
        }
        wamr->tmp_buf = env->memories;
        wamr->tmp_buf_size = env->memory_count;

        env->global_data_size = global_data.size();
        memcpy(env->global_data, global_data.data(), global_data.size());
        for (int i = 0; i < env->table_count; i++) {
            env->tables[i] =
                new WASMTableInstance{ .elem_type = tables[i].elem_type,
                                       .cur_size = tables[i].cur_size,
                                       .max_size = tables[i].max_size,
                                       .elems = { tables[i].elems[0] } };
        }
        env->table_count = tables.size();
    }
    else {
        env->memory_count = wamr->tmp_buf_size;
        env->memories = wamr->tmp_buf;
        memcpy(env->global_data, global_data.data(), global_data.size());
        env->global_data_size = global_data.size();
        for (int i = 0; i < env->table_count; i++) {
            env->tables[i] = new WASMTableInstance{
                .elem_type = (*env->tables[i]).elem_type,
                .cur_size = (*env->tables[i]).cur_size,
                .max_size = (*env->tables[i]).max_size,
                .elems = { (*env->tables[i]).elems[0] }
            };
        }
        env->table_count = tables.size();
    }
    env->global_table_data.memory_instances[0] = **env->memories;
    if (wamr->is_aot) {
        auto module = (AOTModule *)env->module;
        module->aux_data_end_global_index = aux_data_end_global_index;
        module->aux_data_end = aux_data_end;
        module->aux_heap_base_global_index = aux_heap_base_global_index;
        module->aux_heap_base = aux_heap_base;
        module->aux_stack_top_global_index = aux_stack_top_global_index;
        module->aux_stack_bottom = aux_stack_bottom;
        module->aux_stack_size = aux_stack_size;
    }
    else {
        auto module = env->module;
        module->aux_data_end_global_index = aux_data_end_global_index;
        module->aux_data_end = aux_data_end;
        module->aux_heap_base_global_index = aux_heap_base_global_index;
        module->aux_heap_base = aux_heap_base;
        module->aux_stack_top_global_index = aux_stack_top_global_index;
        module->aux_stack_bottom = aux_stack_bottom;
        module->aux_stack_size = aux_stack_size;
    }
    restore(&wasi_ctx, &env->module->wasi_args);
}

#if !__has_include(<expected>) || __cplusplus <= 202002L
void
dump(WAMRModuleInstance *t, WASMModuleInstance *env)
{
    t->dump_impl(env);
};
void
restore(WAMRModuleInstance *t, WASMModuleInstance *env)
{
    t->restore_impl(env);
};
#endif