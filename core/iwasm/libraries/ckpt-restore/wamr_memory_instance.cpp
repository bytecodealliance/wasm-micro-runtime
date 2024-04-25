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

#include "wamr.h"
#include "wamr_memory_instance.h"
extern WAMRInstance *wamr;
void
WAMRMemoryInstance::restore_impl(WASMMemoryInstance *env)
{
    env->module_type = module_type;
    env->ref_count = ref_count + 1;
    LOG_DEBUG("ref_count:{}", env->ref_count);
    env->is_shared_memory = true;
    env->num_bytes_per_page = num_bytes_per_page;
    env->cur_page_count = cur_page_count;
    env->max_page_count = max_page_count;
    env->memory_data_size = memory_data.size();
#if !defined(BH_PLATFORM_WINDOWS)
    if (env->ref_count > 0) // shared memory
        env->memory_data =
            (uint8 *)mmap(NULL, wamr->heap_size, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    else
#endif
        env->memory_data = (uint8 *)malloc(env->memory_data_size);
    memcpy(env->memory_data, memory_data.data(), env->memory_data_size);
    env->memory_data_end = env->memory_data + (memory_data.size());
    env->heap_data = (uint8 *)malloc(heap_data.size());
    memcpy(env->heap_data, heap_data.data(), heap_data.size());
    env->heap_data_end = env->heap_data + heap_data.size();
};

#if !__has_include(<expected>) || __cplusplus <= 202002L
void
dump(WAMRMemoryInstance *t, WASMMemoryInstance *env)
{
    t->dump_impl(env);
};
void
restore(WAMRMemoryInstance *t, WASMMemoryInstance *env)
{
    t->restore_impl(env);
};
#endif