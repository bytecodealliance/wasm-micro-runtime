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

#ifndef MVVM_WAMR_MEMORY_INSTANCE_H
#define MVVM_WAMR_MEMORY_INSTANCE_H
#include "wamr_serializer.h"
#include "wasm_runtime.h"
#include <memory>
#include <vector>
struct WAMRMemoryInstance {
    /* Module type */
    uint32 module_type;
    /* Shared memory flag */
    uint16 ref_count;
    /* Shared memory flag */
    uint8 is_shared_memory;
    /* Number bytes per page */
    uint32 num_bytes_per_page;
    /* Current page count */
    uint32 cur_page_count;
    /* Maximum page count */
    uint32 max_page_count;
    /*
     * Memory data begin address, Note:
     *   the app-heap might be inserted in to the linear memory,
     *   when memory is re-allocated, the heap data and memory data
     *   must be copied to new memory also
     */
    std::vector<uint8_t> memory_data;

    /* Heap data base address */
    std::vector<uint8> heap_data;

    void dump_impl(WASMMemoryInstance *env)
    {
        module_type = env->module_type;
        ref_count = env->ref_count;
        num_bytes_per_page = env->num_bytes_per_page;
        cur_page_count = env->cur_page_count;
        max_page_count = env->max_page_count;
        is_shared_memory = env->is_shared_memory;
        memory_data.resize(env->memory_data_size);
        // memory_data = std::vector<uint8_t>(env->memory_data,
        // env->memory_data_size);
        memcpy(memory_data.data(), env->memory_data, env->memory_data_size);
        heap_data = std::vector<uint8>(env->heap_data, env->heap_data_end);
    };
    void restore_impl(WASMMemoryInstance *env);
};
#if __has_include(<expected>) && __cplusplus > 202002L
template<SerializerTrait<WASMMemoryInstance *> T>
void
dump(T t, WASMMemoryInstance *env)
{
    t->dump_impl(env);
};
template<SerializerTrait<WASMMemoryInstance *> T>
void
restore(T t, WASMMemoryInstance *env)
{
    t->restore_impl(env);
};
#else
void
dump(WAMRMemoryInstance *t, WASMMemoryInstance *env);
void
restore(WAMRMemoryInstance *t, WASMMemoryInstance *env);
#endif
#endif // MVVM_WAMR_MEMORY_INSTANCE_H
