/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_runtime.h"
#include "bh_log.h"
#include "mem_alloc.h"
#if WASM_ENABLE_SHARED_MEMORY != 0
#include "../common/wasm_shared_memory.h"
#endif

static void
set_error_buf(char *error_buf, uint32 error_buf_size, const char *string)
{
    if (error_buf != NULL) {
        snprintf(error_buf, error_buf_size,
                 "AOT module instantiate failed: %s", string);
    }
}

static void *
runtime_malloc(uint64 size, char *error_buf, uint32 error_buf_size)
{
    void *mem;

    if (size >= UINT32_MAX
        || !(mem = wasm_runtime_malloc((uint32)size))) {
        set_error_buf(error_buf, error_buf_size,
                      "allocate memory failed");
        return NULL;
    }

    memset(mem, 0, (uint32)size);
    return mem;
}

static bool
global_instantiate(AOTModuleInstance *module_inst, AOTModule *module,
                   char *error_buf, uint32 error_buf_size)
{
    uint32 i;
    InitializerExpression *init_expr;
    uint8 *p = (uint8*)module_inst->global_data.ptr;
    AOTImportGlobal *import_global = module->import_globals;
    AOTGlobal *global = module->globals;

    /* Initialize import global data */
    for (i = 0; i < module->import_global_count; i++, import_global++) {
        bh_assert(import_global->data_offset ==
                  (uint32)(p - (uint8*)module_inst->global_data.ptr));
        memcpy(p, &import_global->global_data_linked, import_global->size);
        p += import_global->size;
    }

    /* Initialize defined global data */
    for (i = 0; i < module->global_count; i++, global++) {
        bh_assert(global->data_offset ==
                  (uint32)(p - (uint8*)module_inst->global_data.ptr));
        init_expr = &global->init_expr;
        switch (init_expr->init_expr_type) {
            case INIT_EXPR_TYPE_GET_GLOBAL:
                if (init_expr->u.global_index >= module->import_global_count + i) {
                    set_error_buf(error_buf, error_buf_size, "unknown global");
                    return false;
                }
                memcpy(p,
                       &module->import_globals[init_expr->u.global_index].global_data_linked,
                       global->size);
                break;
            default:
                /* TODO: check whether global type and init_expr type are matching */
                memcpy(p, &init_expr->u, global->size);
                break;
        }
        p += global->size;
    }

    bh_assert(module_inst->global_data_size ==
              (uint32)(p - (uint8*)module_inst->global_data.ptr));
    return true;
}

static bool
table_instantiate(AOTModuleInstance *module_inst, AOTModule *module,
                  char *error_buf, uint32 error_buf_size)
{
    uint32 i, global_index, global_data_offset, base_offset, length;
    AOTTableInitData *table_seg;

    for (i = 0; i < module->table_init_data_count; i++) {
        table_seg = module->table_init_data_list[i];
        bh_assert(table_seg->offset.init_expr_type ==
                        INIT_EXPR_TYPE_I32_CONST
                    || table_seg->offset.init_expr_type ==
                        INIT_EXPR_TYPE_GET_GLOBAL);

        /* Resolve table data base offset */
        if (table_seg->offset.init_expr_type == INIT_EXPR_TYPE_GET_GLOBAL) {
            global_index = table_seg->offset.u.global_index;
            bh_assert(global_index <
                        module->import_global_count + module->global_count);
            /* TODO: && globals[table_seg->offset.u.global_index].type ==
                        VALUE_TYPE_I32*/
            if (global_index < module->import_global_count)
                global_data_offset =
                    module->import_globals[global_index].data_offset;
            else
                global_data_offset =
                    module->globals[global_index - module->import_global_count]
                            .data_offset;

            base_offset = *(uint32*)
                ((uint8*)module_inst->global_data.ptr + global_data_offset);
        }
        else
            base_offset = (uint32)table_seg->offset.u.i32;

        /* Copy table data */
        bh_assert(module_inst->table_data.ptr);
        /* base_offset only since length might negative */
        if (base_offset > module_inst->table_size) {
            LOG_DEBUG("base_offset(%d) > table_size(%d)", base_offset,
                      module_inst->table_size);
            set_error_buf(error_buf, error_buf_size,
                          "elements segment does not fit");
            return false;
        }

        /* base_offset + length(could be zero) */
        length = table_seg->func_index_count;
        if (base_offset + length > module_inst->table_size) {
            LOG_DEBUG("base_offset(%d) + length(%d) > table_size(%d)",
                      base_offset, length, module_inst->table_size);
            set_error_buf(error_buf, error_buf_size,
                          "elements segment does not fit");
            return false;
        }

        /**
         * Check function index in the current module inst for now.
         * will check the linked table inst owner in future
         */
        memcpy((uint32 *)module_inst->table_data.ptr + base_offset,
               table_seg->func_indexes,
               length * sizeof(uint32));
    }

    return true;
}

static void
memories_deinstantiate(AOTModuleInstance *module_inst)
{
    uint32 i;
    AOTMemoryInstance *memory_inst;

    for (i = 0; i < module_inst->memory_count; i++) {
        memory_inst = ((AOTMemoryInstance **)module_inst->memories.ptr)[i];
        if (memory_inst) {
#if WASM_ENABLE_SHARED_MEMORY != 0
            if (memory_inst->is_shared) {
                int32 ref_count =
                    shared_memory_dec_reference(
                        (WASMModuleCommon *)module_inst->aot_module.ptr);
                bh_assert(ref_count >= 0);

                /* if the reference count is not zero,
                    don't free the memory */
                if (ref_count > 0)
                    continue;
            }
#endif
            if (memory_inst->heap_handle.ptr)
                mem_allocator_destroy(memory_inst->heap_handle.ptr);

            if (memory_inst->heap_data.ptr) {
#ifndef OS_ENABLE_HW_BOUND_CHECK
                wasm_runtime_free(memory_inst->memory_data.ptr);
#else
                os_munmap((uint8*)memory_inst->memory_data.ptr,
                          8 * (uint64)BH_GB);
#endif
            }
        }
    }
    wasm_runtime_free(module_inst->memories.ptr);
}

static AOTMemoryInstance*
memory_instantiate(AOTModuleInstance *module_inst, AOTModule *module,
                   AOTMemoryInstance *memory_inst, AOTMemory *memory,
                   uint32 heap_size, char *error_buf, uint32 error_buf_size)
{
    void *heap_handle;
    uint32 num_bytes_per_page = memory->num_bytes_per_page;
    uint32 init_page_count = memory->mem_init_page_count;
    uint32 max_page_count = memory->mem_max_page_count;
    uint32 inc_page_count, aux_heap_base, global_idx;
    uint32 bytes_of_last_page, bytes_to_page_end;
    uint32 heap_offset = num_bytes_per_page *init_page_count;
    uint64 total_size;
    uint8 *p, *global_addr;
#ifdef OS_ENABLE_HW_BOUND_CHECK
    uint8 *mapped_mem;
    uint64 map_size = 8 * (uint64)BH_GB;
    uint64 page_size = os_getpagesize();
#endif

#if WASM_ENABLE_SHARED_MEMORY != 0
    bool is_shared_memory = memory->memory_flags & 0x02 ? true : false;

    /* Shared memory */
    if (is_shared_memory) {
        AOTMemoryInstance *shared_memory_instance;
        WASMSharedMemNode *node =
            wasm_module_get_shared_memory((WASMModuleCommon *)module);
        /* If the memory of this module has been instantiated,
            return the memory instance directly */
        if (node) {
            uint32 ref_count;
            ref_count = shared_memory_inc_reference(
                                (WASMModuleCommon *)module);
            bh_assert(ref_count > 0);
            shared_memory_instance =
                    (AOTMemoryInstance *)shared_memory_get_memory_inst(node);
            bh_assert(shared_memory_instance);

            (void)ref_count;
            return shared_memory_instance;
        }
    }
#endif

    if (heap_size > 0
        && module->malloc_func_index != (uint32)-1
        && module->free_func_index != (uint32)-1) {
        /* Disable app heap, use malloc/free function exported
           by wasm app to allocate/free memory instead */
        heap_size = 0;
    }

    if (init_page_count == max_page_count && init_page_count == 1) {
        /* If only one page and at most one page, we just append
           the app heap to the end of linear memory, enlarge the
           num_bytes_per_page, and don't change the page count*/
        heap_offset = num_bytes_per_page;
        num_bytes_per_page += heap_size;
        if (num_bytes_per_page < heap_size) {
            set_error_buf(error_buf, error_buf_size,
                          "memory size must be at most 65536 pages (4GiB)");
            return NULL;
        }
    }
    else if (heap_size > 0) {
        if (module->aux_heap_base_global_index != (uint32)-1
            && module->aux_heap_base < num_bytes_per_page
                                       * init_page_count) {
            /* Insert app heap before __heap_base */
            aux_heap_base = module->aux_heap_base;
            bytes_of_last_page = aux_heap_base % num_bytes_per_page;
            if (bytes_of_last_page == 0)
                bytes_of_last_page = num_bytes_per_page;
            bytes_to_page_end = num_bytes_per_page - bytes_of_last_page;
            inc_page_count = (heap_size - bytes_to_page_end
                              + num_bytes_per_page - 1) / num_bytes_per_page;
            heap_offset = aux_heap_base;
            aux_heap_base += heap_size;

            bytes_of_last_page = aux_heap_base % num_bytes_per_page;
            if (bytes_of_last_page == 0)
                bytes_of_last_page = num_bytes_per_page;
            bytes_to_page_end = num_bytes_per_page - bytes_of_last_page;
            if (bytes_to_page_end < 1 * BH_KB) {
                aux_heap_base += 1 * BH_KB;
                inc_page_count++;
            }

            /* Adjust __heap_base global value */
            global_idx = module->aux_heap_base_global_index
                         - module->import_global_count;
            global_addr = (uint8*)module_inst->global_data.ptr +
                          module->globals[global_idx].data_offset;
            *(uint32 *)global_addr = aux_heap_base;
            LOG_VERBOSE("Reset __heap_base global to %u", aux_heap_base);
        }
        else {
            /* Insert app heap before new page */
            inc_page_count = (heap_size + num_bytes_per_page - 1)
                             / num_bytes_per_page;
            heap_offset = num_bytes_per_page * init_page_count;
            heap_size = num_bytes_per_page * inc_page_count;
            if (heap_size > 0)
                heap_size -= 1 * BH_KB;
        }
        init_page_count += inc_page_count;
        max_page_count += inc_page_count;
        if (init_page_count > 65536) {
            set_error_buf(error_buf, error_buf_size,
                          "memory size must be at most 65536 pages (4GiB)");
            return NULL;
        }
        if (max_page_count > 65536)
            max_page_count = 65536;
    }

    LOG_VERBOSE("Memory instantiate:");
    LOG_VERBOSE("  page bytes: %u, init pages: %u, max pages: %u",
                num_bytes_per_page, init_page_count, max_page_count);
    LOG_VERBOSE("  heap offset: %u, heap size: %d\n", heap_offset, heap_size);

    total_size = (uint64)num_bytes_per_page * init_page_count;
#if WASM_ENABLE_SHARED_MEMORY != 0
    if (is_shared_memory) {
        /* Allocate max page for shared memory */
        total_size = (uint64)num_bytes_per_page * max_page_count;
    }
#endif

#ifndef OS_ENABLE_HW_BOUND_CHECK
    /* Allocate memory */
    if (!(p = runtime_malloc(total_size, error_buf, error_buf_size))) {
        return NULL;
    }
#else
    total_size = (total_size + page_size - 1) & ~(page_size - 1);

    /* Totally 8G is mapped, the opcode load/store address range is 0 to 8G:
     *   ea = i + memarg.offset
     * both i and memarg.offset are u32 in range 0 to 4G
     * so the range of ea is 0 to 8G
     */
    if (total_size >= UINT32_MAX
        || !(p = mapped_mem = os_mmap(NULL, map_size,
                                      MMAP_PROT_NONE, MMAP_MAP_NONE))) {
        set_error_buf(error_buf, error_buf_size, "mmap memory failed");
        return NULL;
    }

    if (os_mprotect(p, total_size, MMAP_PROT_READ | MMAP_PROT_WRITE) != 0) {
        set_error_buf(error_buf, error_buf_size, "mprotec memory failed");
        os_munmap(mapped_mem, map_size);
        return NULL;
    }
    memset(p, 0, (uint32)total_size);
#endif /* end of OS_ENABLE_HW_BOUND_CHECK */

    memory_inst->module_type = Wasm_Module_AoT;
    memory_inst->num_bytes_per_page = num_bytes_per_page;
    memory_inst->cur_page_count = init_page_count;
    memory_inst->max_page_count = max_page_count;

    /* Init memory info */
    memory_inst->memory_data.ptr = p;
    memory_inst->memory_data_end.ptr = p + (uint32)total_size;
    memory_inst->memory_data_size = (uint32)total_size;

    /* Initialize heap info */
    memory_inst->heap_data.ptr = p + heap_offset;
    memory_inst->heap_data_end.ptr = p + heap_offset + heap_size;
    if (heap_size > 0) {
        if (!(heap_handle = mem_allocator_create(memory_inst->heap_data.ptr,
                                                 heap_size))) {
            set_error_buf(error_buf, error_buf_size,
                          "init app heap failed");
            goto fail1;
        }
        memory_inst->heap_handle.ptr = heap_handle;
    }

    if (total_size > 0) {
       if (sizeof(uintptr_t) == sizeof(uint64)) {
           memory_inst->mem_bound_check_1byte.u64 = total_size - 1;
           memory_inst->mem_bound_check_2bytes.u64 = total_size - 2;
           memory_inst->mem_bound_check_4bytes.u64 = total_size - 4;
           memory_inst->mem_bound_check_8bytes.u64 = total_size - 8;
       }
       else {
           memory_inst->mem_bound_check_1byte.u32[0] = (uint32)total_size - 1;
           memory_inst->mem_bound_check_2bytes.u32[0] = (uint32)total_size - 2;
           memory_inst->mem_bound_check_4bytes.u32[0] = (uint32)total_size - 4;
           memory_inst->mem_bound_check_8bytes.u32[0] = (uint32)total_size - 8;
       }
    }

#if WASM_ENABLE_SHARED_MEMORY != 0
    if (is_shared_memory) {
        memory_inst->is_shared = true;
        if (!shared_memory_set_memory_inst((WASMModuleCommon *)module,
                                           (WASMMemoryInstanceCommon *)memory_inst)) {
            set_error_buf(error_buf, error_buf_size,
                          "allocate memory failed");
            goto fail2;
        }
    }
#endif

    return memory_inst;

#if WASM_ENABLE_SHARED_MEMORY != 0
fail2:
    if (heap_size > 0) {
        mem_allocator_destroy(memory_inst->heap_handle.ptr);
        memory_inst->heap_handle.ptr = NULL;
    }
#endif
fail1:
#ifndef OS_ENABLE_HW_BOUND_CHECK
    wasm_runtime_free(memory_inst->memory_data.ptr);
#else
    os_munmap(mapped_mem, map_size);
#endif
    memory_inst->memory_data.ptr = NULL;
    return NULL;
}

static AOTMemoryInstance*
aot_get_default_memory(AOTModuleInstance *module_inst)
{
    if (module_inst->memories.ptr)
        return ((AOTMemoryInstance **)module_inst->memories.ptr)[0];
    else
        return NULL;
}

static bool
memories_instantiate(AOTModuleInstance *module_inst, AOTModule *module,
                     uint32 heap_size, char *error_buf, uint32 error_buf_size)
{
    uint32 global_index, global_data_offset, base_offset, length;
    uint32 i, memory_count = module->memory_count;
    AOTMemoryInstance *memories, *memory_inst;
    AOTMemInitData *data_seg;
    uint64 total_size;

    module_inst->memory_count = memory_count;
    total_size = sizeof(AOTPointer) * (uint64)memory_count;
    if (!(module_inst->memories.ptr =
            runtime_malloc(total_size, error_buf, error_buf_size))) {
        return false;
    }

    memories = module_inst->global_table_data.memory_instances;
    for (i = 0; i < memory_count; i++, memories++) {
        memory_inst =
            memory_instantiate(module_inst, module,
                               memories, &module->memories[i],
                               heap_size, error_buf, error_buf_size);
        if (!memory_inst) {
            return false;
        }

        ((AOTMemoryInstance **)module_inst->memories.ptr)[i] = memory_inst;
    }

    /* Get default memory instance */
    memory_inst = aot_get_default_memory(module_inst);

    for (i = 0; i < module->mem_init_data_count; i++) {
        data_seg = module->mem_init_data_list[i];
#if WASM_ENABLE_BULK_MEMORY != 0
        if (data_seg->is_passive)
            continue;
#endif

        bh_assert(data_seg->offset.init_expr_type ==
                        INIT_EXPR_TYPE_I32_CONST
                  || data_seg->offset.init_expr_type ==
                        INIT_EXPR_TYPE_GET_GLOBAL);

        /* Resolve memory data base offset */
        if (data_seg->offset.init_expr_type == INIT_EXPR_TYPE_GET_GLOBAL) {
            global_index = data_seg->offset.u.global_index;
            bh_assert(global_index <
                        module->import_global_count + module->global_count);
            /* TODO: && globals[data_seg->offset.u.global_index].type ==
                        VALUE_TYPE_I32*/
            if (global_index < module->import_global_count)
                global_data_offset =
                    module->import_globals[global_index].data_offset;
            else
                global_data_offset =
                    module->globals[global_index - module->import_global_count]
                            .data_offset;

            base_offset = *(uint32*)
                ((uint8*)module_inst->global_data.ptr + global_data_offset);
        } else {
            base_offset = (uint32)data_seg->offset.u.i32;
        }

        /* Copy memory data */
        bh_assert(memory_inst->memory_data.ptr);

        /* Check memory data */
        /* check offset since length might negative */
        if (base_offset > memory_inst->memory_data_size) {
            LOG_DEBUG("base_offset(%d) > memory_data_size(%d)", base_offset,
                      memory_inst->memory_data_size);
            set_error_buf(error_buf, error_buf_size,
                          "data segment does not fit");
            return false;
        }

        /* check offset + length(could be zero) */
        length = data_seg->byte_count;
        if (base_offset + length > memory_inst->memory_data_size) {
            LOG_DEBUG("base_offset(%d) + length(%d) > memory_data_size(%d)",
                      base_offset, length, memory_inst->memory_data_size);
            set_error_buf(error_buf, error_buf_size,
                          "data segment does not fit");
            return false;
        }

        bh_memcpy_s((uint8*)memory_inst->memory_data.ptr + base_offset,
                    memory_inst->memory_data_size - base_offset,
                    data_seg->bytes, length);
    }

    return true;
}

static bool
init_func_ptrs(AOTModuleInstance *module_inst, AOTModule *module,
               char *error_buf, uint32 error_buf_size)
{
    uint32 i;
    void **func_ptrs;
    uint64 total_size =
        ((uint64)module->import_func_count + module->func_count) * sizeof(void*);

    /* Allocate memory */
    if (!(module_inst->func_ptrs.ptr = runtime_malloc
                (total_size, error_buf, error_buf_size))) {
        return false;
    }

    /* Set import function pointers */
    func_ptrs = (void**)module_inst->func_ptrs.ptr;
    for (i = 0; i < module->import_func_count; i++, func_ptrs++)
        *func_ptrs = (void*)module->import_funcs[i].func_ptr_linked;

    /* Set defined function pointers */
    memcpy(func_ptrs, module->func_ptrs, module->func_count * sizeof(void*));
    return true;
}

static bool
init_func_type_indexes(AOTModuleInstance *module_inst, AOTModule *module,
                       char *error_buf, uint32 error_buf_size)
{
    uint32 i;
    uint32 *func_type_index;
    uint64 total_size =
        ((uint64)module->import_func_count + module->func_count) * sizeof(uint32);

    /* Allocate memory */
    if (!(module_inst->func_type_indexes.ptr =
                runtime_malloc(total_size, error_buf, error_buf_size))) {
        return false;
    }

    /* Set import function type indexes */
    func_type_index = (uint32*)module_inst->func_type_indexes.ptr;
    for (i = 0; i < module->import_func_count; i++, func_type_index++)
        *func_type_index = module->import_funcs[i].func_type_index;

    memcpy(func_type_index, module->func_type_indexes,
           module->func_count * sizeof(uint32));

    return true;
}

static bool
create_export_funcs(AOTModuleInstance *module_inst, AOTModule *module,
                    char *error_buf, uint32 error_buf_size)
{
    AOTExport *exports = module->exports;
    AOTFunctionInstance *export_func;
    uint64 size;
    uint32 i, func_index, ftype_index;

    for (i = 0; i < module->export_count; i++) {
        if (exports[i].kind == EXPORT_KIND_FUNC)
            module_inst->export_func_count++;
    }

    if (module_inst->export_func_count > 0) {
        /* Allocate memory */
        size = sizeof(AOTFunctionInstance)
               * (uint64)module_inst->export_func_count;
        if (!(module_inst->export_funcs.ptr = export_func =
                    runtime_malloc(size, error_buf, error_buf_size))) {
            return false;
        }

        for (i = 0; i < module->export_count; i++) {
            if (exports[i].kind == EXPORT_KIND_FUNC) {
                export_func->func_name = exports[i].name;
                export_func->func_index = exports[i].index;
                if (export_func->func_index < module->import_func_count) {
                    export_func->is_import_func = true;
                    export_func->u.func_import =
                        &module->import_funcs[export_func->func_index];
                }
                else {
                    export_func->is_import_func = false;
                    func_index = export_func->func_index
                                 - module->import_func_count;
                    ftype_index = module->func_type_indexes[func_index];
                    export_func->u.func.func_type =
                                module->func_types[ftype_index];
                    export_func->u.func.func_ptr =
                                module->func_ptrs[func_index];
                }
                export_func++;
            }
        }
    }

    return true;
}

static bool
create_exports(AOTModuleInstance *module_inst, AOTModule *module,
               char *error_buf, uint32 error_buf_size)
{
    return create_export_funcs(module_inst, module,
                               error_buf, error_buf_size);
}

static bool
execute_post_inst_function(AOTModuleInstance *module_inst)
{
    AOTFunctionInstance *post_inst_func =
        aot_lookup_function(module_inst, "__post_instantiate", "()");

    if (!post_inst_func)
        /* Not found */
        return true;

    return aot_create_exec_env_and_call_function(module_inst, post_inst_func, 0, NULL);
}

static bool
execute_start_function(AOTModuleInstance *module_inst)
{
    AOTModule *module = (AOTModule*)module_inst->aot_module.ptr;
    WASMExecEnv *exec_env;
    typedef void (*F)(WASMExecEnv*);
    union { F f; void *v; } u;

    if (!module->start_function)
        return true;

    if (!(exec_env = wasm_exec_env_create((WASMModuleInstanceCommon*)module_inst,
                                          module_inst->default_wasm_stack_size))) {
        aot_set_exception(module_inst, "allocate memory failed");
        return false;
    }

    u.v = module->start_function;
    u.f(exec_env);

    wasm_exec_env_destroy(exec_env);
    return !aot_get_exception(module_inst);
}

#if WASM_ENABLE_BULK_MEMORY != 0
static bool
execute_memory_init_function(AOTModuleInstance *module_inst)
{
    AOTFunctionInstance *memory_init_func =
        aot_lookup_function(module_inst, "__wasm_call_ctors", "()");

    if (!memory_init_func)
        /* Not found */
        return true;

    return aot_create_exec_env_and_call_function(module_inst, memory_init_func,
                                                 0, NULL);
}
#endif

AOTModuleInstance*
aot_instantiate(AOTModule *module, bool is_sub_inst,
                uint32 stack_size, uint32 heap_size,
                char *error_buf, uint32 error_buf_size)
{
    AOTModuleInstance *module_inst;
    uint32 module_inst_struct_size =
        offsetof(AOTModuleInstance, global_table_data.bytes);
    uint64 module_inst_mem_inst_size =
        (uint64)module->memory_count * sizeof(AOTMemoryInstance);
    uint32 table_size = module->table_count > 0 ?
                        module->tables[0].table_init_size : 0;
    uint64 table_data_size = (uint64)table_size * sizeof(uint32);
    uint64 total_size = (uint64)module_inst_struct_size
                        + module_inst_mem_inst_size
                        + module->global_data_size
                        + table_data_size;
    uint8 *p;

    /* Check heap size */
    heap_size = align_uint(heap_size, 8);
    if (heap_size > APP_HEAP_SIZE_MAX)
        heap_size = APP_HEAP_SIZE_MAX;

    /* Allocate module instance, global data, table data and heap data */
    if (!(module_inst = runtime_malloc(total_size,
                                       error_buf, error_buf_size))) {
        return NULL;
    }

    module_inst->module_type = Wasm_Module_AoT;
    module_inst->aot_module.ptr = module;

    /* Initialize global info */
    p = (uint8*)module_inst + module_inst_struct_size +
                              module_inst_mem_inst_size;
    module_inst->global_data.ptr = p;
    module_inst->global_data_size = module->global_data_size;
    if (!global_instantiate(module_inst, module, error_buf, error_buf_size))
        goto fail;

    /* Initialize table info */
    p += module->global_data_size;
    module_inst->table_data.ptr = p;
    module_inst->table_size = table_size;
    /* Set all elements to -1 to mark them as uninitialized elements */
    memset(module_inst->table_data.ptr, -1, (uint32)table_data_size);
    if (!table_instantiate(module_inst, module, error_buf, error_buf_size))
        goto fail;

    /* Initialize memory space */
    if (!memories_instantiate(module_inst, module, heap_size,
                              error_buf, error_buf_size))
        goto fail;

    /* Initialize function pointers */
    if (!init_func_ptrs(module_inst, module, error_buf, error_buf_size))
        goto fail;

    /* Initialize function type indexes */
    if (!init_func_type_indexes(module_inst, module, error_buf, error_buf_size))
        goto fail;

    if (!create_exports(module_inst, module, error_buf, error_buf_size))
        goto fail;

#if WASM_ENABLE_LIBC_WASI != 0
    if (!is_sub_inst) {
        if (heap_size > 0
            && !wasm_runtime_init_wasi((WASMModuleInstanceCommon*)module_inst,
                                       module->wasi_args.dir_list,
                                       module->wasi_args.dir_count,
                                       module->wasi_args.map_dir_list,
                                       module->wasi_args.map_dir_count,
                                       module->wasi_args.env,
                                       module->wasi_args.env_count,
                                       module->wasi_args.argv,
                                       module->wasi_args.argc,
                                       error_buf, error_buf_size))
            goto fail;
    }
#endif

    /* Initialize the thread related data */
    if (stack_size == 0)
        stack_size = DEFAULT_WASM_STACK_SIZE;
#if WASM_ENABLE_SPEC_TEST != 0
    if (stack_size < 48 *1024)
        stack_size = 48 * 1024;
#endif
    module_inst->default_wasm_stack_size = stack_size;

    /* Execute __post_instantiate function and start function*/
    if (!execute_post_inst_function(module_inst)
        || !execute_start_function(module_inst)) {
        set_error_buf(error_buf, error_buf_size,
                      module_inst->cur_exception);
        goto fail;
    }

#if WASM_ENABLE_BULK_MEMORY != 0
#if WASM_ENABLE_LIBC_WASI != 0
    if (!module->is_wasi_module) {
#endif
        /* Only execute the memory init function for main instance because
            the data segments will be dropped once initialized.
        */
        if (!is_sub_inst) {
            if (!execute_memory_init_function(module_inst)) {
                set_error_buf(error_buf, error_buf_size,
                              module_inst->cur_exception);
                goto fail;
            }
        }
#if WASM_ENABLE_LIBC_WASI != 0
    }
#endif
#endif

#if WASM_ENABLE_MEMORY_TRACING != 0
    wasm_runtime_dump_module_inst_mem_consumption
                    ((WASMModuleInstanceCommon *)module_inst);
#endif

    return module_inst;

fail:
    aot_deinstantiate(module_inst, is_sub_inst);
    return NULL;
}

void
aot_deinstantiate(AOTModuleInstance *module_inst, bool is_sub_inst)
{
#if WASM_ENABLE_LIBC_WASI != 0
    /* Destroy wasi resource before freeing app heap, since some fields of
       wasi contex are allocated from app heap, and if app heap is freed,
       these fields will be set to NULL, we cannot free their internal data
       which may allocated from global heap. */
    /* Only destroy wasi ctx in the main module instance */
    if (!is_sub_inst)
        wasm_runtime_destroy_wasi((WASMModuleInstanceCommon*)module_inst);
#endif

    if (module_inst->memories.ptr)
        memories_deinstantiate(module_inst);

    if (module_inst->export_funcs.ptr)
        wasm_runtime_free(module_inst->export_funcs.ptr);

    if (module_inst->func_ptrs.ptr)
        wasm_runtime_free(module_inst->func_ptrs.ptr);

    if (module_inst->func_type_indexes.ptr)
        wasm_runtime_free(module_inst->func_type_indexes.ptr);

    wasm_runtime_free(module_inst);
}

AOTFunctionInstance*
aot_lookup_function(const AOTModuleInstance *module_inst,
                    const char *name, const char *signature)
{
    uint32 i;
    AOTFunctionInstance *export_funcs = (AOTFunctionInstance *)
                                        module_inst->export_funcs.ptr;

    for (i = 0; i < module_inst->export_func_count; i++)
        if (!strcmp(export_funcs[i].func_name, name))
            return &export_funcs[i];
    (void)signature;
    return NULL;
}

#define PUT_I64_TO_ADDR(addr, value) do {       \
    union { int64 val; uint32 parts[2]; } u;    \
    u.val = (value);                            \
    (addr)[0] = u.parts[0];                     \
    (addr)[1] = u.parts[1];                     \
  } while (0)

#define PUT_F64_TO_ADDR(addr, value) do {       \
    union { float64 val; uint32 parts[2]; } u;  \
    u.val = (value);                            \
    (addr)[0] = u.parts[0];                     \
    (addr)[1] = u.parts[1];                     \
  } while (0)


#ifdef OS_ENABLE_HW_BOUND_CHECK

#define STACK_OVERFLOW_CHECK_GUARD_PAGE_COUNT 3

static os_thread_local_attribute WASMExecEnv *aot_exec_env = NULL;

static inline uint8 *
get_stack_min_addr(WASMExecEnv *exec_env, uint32 page_size)
{
    uintptr_t stack_bound = (uintptr_t)exec_env->native_stack_boundary;
    return (uint8*)(stack_bound & ~(uintptr_t)(page_size -1 ));
}

static void
aot_signal_handler(void *sig_addr)
{
    AOTModuleInstance *module_inst;
    AOTMemoryInstance *memory_inst;
    WASMJmpBuf *jmpbuf_node;
    uint8 *mapped_mem_start_addr = NULL;
    uint8 *mapped_mem_end_addr = NULL;
    uint8 *stack_min_addr;
    uint32 page_size;
    uint32 guard_page_count = STACK_OVERFLOW_CHECK_GUARD_PAGE_COUNT;

    /* Check whether current thread is running aot function */
    if (aot_exec_env
        && aot_exec_env->handle == os_self_thread()
        && (jmpbuf_node = aot_exec_env->jmpbuf_stack_top)) {
        /* Get mapped mem info of current instance */
        module_inst = (AOTModuleInstance *)aot_exec_env->module_inst;
        /* Get the default memory instance */
        memory_inst = aot_get_default_memory(module_inst);
        if (memory_inst) {
            mapped_mem_start_addr = (uint8*)memory_inst->memory_data.ptr;
            mapped_mem_end_addr = (uint8*)memory_inst->memory_data.ptr
                                  + 8 * (uint64)BH_GB;
        }

        /* Get stack info of current thread */
        page_size = os_getpagesize();
        stack_min_addr = get_stack_min_addr(aot_exec_env, page_size);

        if (memory_inst
            && (mapped_mem_start_addr <= (uint8*)sig_addr
                && (uint8*)sig_addr < mapped_mem_end_addr)) {
            /* The address which causes segmentation fault is inside
               aot instance's guard regions */
            aot_set_exception_with_id(module_inst, EXCE_OUT_OF_BOUNDS_MEMORY_ACCESS);
            os_longjmp(jmpbuf_node->jmpbuf, 1);
        }
        else if (stack_min_addr - page_size <= (uint8*)sig_addr
                 && (uint8*)sig_addr < stack_min_addr
                                       + page_size * guard_page_count) {
            /* The address which causes segmentation fault is inside
               native thread's guard page */
            aot_set_exception_with_id(module_inst, EXCE_NATIVE_STACK_OVERFLOW);
            os_longjmp(jmpbuf_node->jmpbuf, 1);
        }
    }
}

bool
aot_signal_init()
{
    return os_signal_init(aot_signal_handler) == 0 ? true : false;
}

void
aot_signal_destroy()
{
    os_signal_destroy();
}

#if defined(__GNUC__)
__attribute__((no_sanitize_address)) static uint32
#else
static uint32
#endif
touch_pages(uint8 *stack_min_addr, uint32 page_size)
{
    uint8 sum = 0;
    while (1) {
        volatile uint8 *touch_addr =
            (volatile uint8*)os_alloca(page_size / 2);
        if (touch_addr < stack_min_addr + page_size) {
            sum += *(stack_min_addr + page_size - 1);
            break;
        }
        sum += *touch_addr;
    }
    return sum;
}

static bool
invoke_native_with_hw_bound_check(WASMExecEnv *exec_env, void *func_ptr,
                                 const WASMType *func_type, const char *signature,
                                 void *attachment,
                                 uint32 *argv, uint32 argc, uint32 *argv_ret)
{
    AOTModuleInstance *module_inst = (AOTModuleInstance*)exec_env->module_inst;
    WASMExecEnv **p_aot_exec_env = &aot_exec_env;
    WASMJmpBuf *jmpbuf_node, *jmpbuf_node_pop;
    uint32 page_size = os_getpagesize();
    uint32 guard_page_count = STACK_OVERFLOW_CHECK_GUARD_PAGE_COUNT;
    uint8 *stack_min_addr = get_stack_min_addr(exec_env, page_size);
    bool ret;

    /* Check native stack overflow firstly to ensure we have enough
       native stack to run the following codes before actually calling
       the aot function in invokeNative function. */
    if ((uint8*)&module_inst < exec_env->native_stack_boundary
                               + page_size * (guard_page_count + 1)) {
        aot_set_exception_with_id(module_inst, EXCE_NATIVE_STACK_OVERFLOW);
        return false;
    }

    if (aot_exec_env
        && (aot_exec_env != exec_env)) {
        aot_set_exception(module_inst, "invalid exec env");
        return false;
    }

    if (!exec_env->jmpbuf_stack_top) {
        /* Touch each stack page to ensure that it has been mapped: the OS may
           lazily grow the stack mapping as a guard page is hit. */
        touch_pages(stack_min_addr, page_size);
        /* First time to call aot function, protect one page */
        if (os_mprotect(stack_min_addr, page_size * guard_page_count,
                        MMAP_PROT_NONE) != 0) {
            aot_set_exception(module_inst, "set protected page failed");
            return false;
        }
    }

    if (!(jmpbuf_node = wasm_runtime_malloc(sizeof(WASMJmpBuf)))) {
        aot_set_exception_with_id(module_inst, EXCE_OUT_OF_MEMORY);
        return false;
    }

    wasm_exec_env_push_jmpbuf(exec_env, jmpbuf_node);

    aot_exec_env = exec_env;
    if (os_setjmp(jmpbuf_node->jmpbuf) == 0) {
        ret = wasm_runtime_invoke_native(exec_env, func_ptr, func_type,
                                         signature, attachment,
                                         argv, argc, argv_ret);
    }
    else {
        /* Exception has been set in signal handler before calling longjmp */
        ret = false;
    }

    jmpbuf_node_pop = wasm_exec_env_pop_jmpbuf(exec_env);
    bh_assert(jmpbuf_node == jmpbuf_node_pop);
    wasm_runtime_free(jmpbuf_node);
    if (!exec_env->jmpbuf_stack_top) {
        /* Unprotect the guard page when the nested call depth is zero */
        os_mprotect(stack_min_addr, page_size * guard_page_count,
                    MMAP_PROT_READ | MMAP_PROT_WRITE);
        *p_aot_exec_env = NULL;
    }
    os_sigreturn();
    os_signal_unmask();
    (void)jmpbuf_node_pop;
    return ret;
}

#define invoke_native_internal invoke_native_with_hw_bound_check
#else /* else of OS_ENABLE_HW_BOUND_CHECK */
#define invoke_native_internal wasm_runtime_invoke_native
#endif /* end of OS_ENABLE_HW_BOUND_CHECK */

bool
aot_call_function(WASMExecEnv *exec_env,
                  AOTFunctionInstance *function,
                  unsigned argc, uint32 argv[])
{
    AOTModuleInstance *module_inst = (AOTModuleInstance*)exec_env->module_inst;
    AOTFuncType *func_type = function->u.func.func_type;
    uint32 result_count = func_type->result_count;
    uint32 ext_ret_count = result_count > 1 ? result_count - 1 : 0;
    bool ret;

    if (ext_ret_count > 0) {
        uint32 cell_num = 0, i;
        uint8 *ext_ret_types = func_type->types + func_type->param_count + 1;
        uint32 argv1_buf[32], *argv1 = argv1_buf, *ext_rets = NULL;
        uint32 *argv_ret = argv;
        uint32 ext_ret_cell = wasm_get_cell_num(ext_ret_types, ext_ret_count);
        uint64 size;

        /* Allocate memory all arguments */
        size = sizeof(uint32) * (uint64)argc            /* original arguments */
               + sizeof(void*) * (uint64)ext_ret_count  /* extra result values' addr */
               + sizeof(uint32) * (uint64)ext_ret_cell; /* extra result values */
        if (size > sizeof(argv1_buf)
            && !(argv1 = runtime_malloc(size, module_inst->cur_exception,
                                        sizeof(module_inst->cur_exception)))) {
            aot_set_exception_with_id(module_inst, EXCE_OUT_OF_MEMORY);
            return false;
        }

        /* Copy original arguments */
        bh_memcpy_s(argv1, (uint32)size, argv, sizeof(uint32) * argc);

        /* Get the extra result value's address */
        ext_rets = argv1 + argc + sizeof(void*)/sizeof(uint32) * ext_ret_count;

        /* Append each extra result value's address to original arguments */
        for (i = 0; i < ext_ret_count; i++) {
            *(uintptr_t*)(argv1 + argc + sizeof(void*) / sizeof(uint32) * i) =
                (uintptr_t)(ext_rets + cell_num);
            cell_num += wasm_value_type_cell_num(ext_ret_types[i]);
        }

        ret = invoke_native_internal(exec_env, function->u.func.func_ptr,
                                     func_type, NULL, NULL, argv1, argc, argv);
        if (!ret || aot_get_exception(module_inst)) {
            if (argv1 != argv1_buf)
                wasm_runtime_free(argv1);
            return false;
        }

        /* Get extra result values */
        switch (func_type->types[func_type->param_count]) {
            case VALUE_TYPE_I32:
            case VALUE_TYPE_F32:
                argv_ret++;
                break;
            case VALUE_TYPE_I64:
            case VALUE_TYPE_F64:
                argv_ret += 2;
                break;
            default:
                bh_assert(0);
                break;
        }
        ext_rets = argv1 + argc + sizeof(void*)/sizeof(uint32) * ext_ret_count;
        bh_memcpy_s(argv_ret, sizeof(uint32) * cell_num,
                    ext_rets, sizeof(uint32) * cell_num);
        if (argv1 != argv1_buf)
            wasm_runtime_free(argv1);

        return true;
    }
    else {
        ret = invoke_native_internal(exec_env, function->u.func.func_ptr,
                                     func_type, NULL, NULL, argv, argc, argv);
        return ret && !aot_get_exception(module_inst) ? true : false;
    }
}

bool
aot_create_exec_env_and_call_function(AOTModuleInstance *module_inst,
                                      AOTFunctionInstance *func,
                                      unsigned argc, uint32 argv[])
{
    WASMExecEnv *exec_env;
    bool ret;

    if (!(exec_env = wasm_exec_env_create((WASMModuleInstanceCommon*)module_inst,
                                          module_inst->default_wasm_stack_size))) {
        aot_set_exception(module_inst, "allocate memory failed");
        return false;
    }

    /* set thread handle and stack boundary */
    wasm_exec_env_set_thread_info(exec_env);

    ret = aot_call_function(exec_env, func, argc, argv);
    wasm_exec_env_destroy(exec_env);
    return ret;
}

void
aot_set_exception(AOTModuleInstance *module_inst,
                  const char *exception)
{
    if (exception)
        snprintf(module_inst->cur_exception,
                 sizeof(module_inst->cur_exception),
                 "Exception: %s", exception);
    else
        module_inst->cur_exception[0] = '\0';
}

void
aot_set_exception_with_id(AOTModuleInstance *module_inst,
                          uint32 id)
{
    switch (id) {
        case EXCE_UNREACHABLE:
            aot_set_exception(module_inst, "unreachable");
            break;
        case EXCE_OUT_OF_MEMORY:
            aot_set_exception(module_inst, "allocate memory failed");
            break;
        case EXCE_OUT_OF_BOUNDS_MEMORY_ACCESS:
            aot_set_exception(module_inst, "out of bounds memory access");
            break;
        case EXCE_INTEGER_OVERFLOW:
            aot_set_exception(module_inst, "integer overflow");
            break;
        case EXCE_INTEGER_DIVIDE_BY_ZERO:
            aot_set_exception(module_inst, "integer divide by zero");
            break;
        case EXCE_INVALID_CONVERSION_TO_INTEGER:
            aot_set_exception(module_inst, "invalid conversion to integer");
            break;
        case EXCE_INVALID_FUNCTION_TYPE_INDEX:
            aot_set_exception(module_inst, "indirect call type mismatch");
            break;
        case EXCE_INVALID_FUNCTION_INDEX:
            aot_set_exception(module_inst, "invalid function index");
            break;
        case EXCE_UNDEFINED_ELEMENT:
            aot_set_exception(module_inst, "undefined element");
            break;
        case EXCE_UNINITIALIZED_ELEMENT:
            aot_set_exception(module_inst, "uninitialized element");
            break;
        case EXCE_CALL_UNLINKED_IMPORT_FUNC:
            aot_set_exception(module_inst, "fail to call unlinked import function");
            break;
        case EXCE_NATIVE_STACK_OVERFLOW:
            aot_set_exception(module_inst, "native stack overflow");
            break;
        case EXCE_UNALIGNED_ATOMIC:
            aot_set_exception(module_inst, "unaligned atomic");
            break;
        default:
            break;
    }
}

const char*
aot_get_exception(AOTModuleInstance *module_inst)
{
    if (module_inst->cur_exception[0] == '\0')
        return NULL;
    else
        return module_inst->cur_exception;
}

void
aot_clear_exception(AOTModuleInstance *module_inst)
{
    module_inst->cur_exception[0] = '\0';
}

static bool
execute_malloc_function(AOTModuleInstance *module_inst,
                        AOTFunctionInstance *malloc_func,
                        uint32 size, uint32 *p_result)
{
    uint32 argv[2];
    bool ret;

    argv[0] = size;
#ifdef OS_ENABLE_HW_BOUND_CHECK
    if (aot_exec_env != NULL) {
        bh_assert(aot_exec_env->module_inst
                  == (WASMModuleInstanceCommon *)module_inst);
        ret = aot_call_function(aot_exec_env, malloc_func, 1, argv);
    }
    else
#endif
    {
        ret = aot_create_exec_env_and_call_function
                                (module_inst, malloc_func, 1, argv);
    }

    if (ret)
        *p_result = argv[0];
    return ret;
}

static bool
execute_free_function(AOTModuleInstance *module_inst,
                      AOTFunctionInstance *free_func,
                      uint32 offset)
{
    uint32 argv[2];

    argv[0] = offset;
#ifdef OS_ENABLE_HW_BOUND_CHECK
    if (aot_exec_env != NULL) {
        bh_assert(aot_exec_env->module_inst
                  == (WASMModuleInstanceCommon *)module_inst);
        return aot_call_function(aot_exec_env, free_func, 1, argv);
    }
    else
#endif
    {
        return aot_create_exec_env_and_call_function
                            (module_inst, free_func, 1, argv);
    }
}

uint32
aot_module_malloc(AOTModuleInstance *module_inst, uint32 size,
                  void **p_native_addr)
{
    AOTMemoryInstance *memory_inst = aot_get_default_memory(module_inst);
    AOTModule *module = (AOTModule *)module_inst->aot_module.ptr;
    uint8 *addr = NULL;
    uint32 offset = 0;

    if (!memory_inst) {
        aot_set_exception(module_inst, "uninitialized memory");
        return 0;
    }

    if (memory_inst->heap_handle.ptr) {
        addr = mem_allocator_malloc(memory_inst->heap_handle.ptr, size);
    }
    else if (module->malloc_func_index != (uint32)-1
             && module->free_func_index != (uint32)-1) {
        AOTFunctionInstance *malloc_func =
            aot_lookup_function(module_inst, "malloc", "(i)i");

        bh_assert(malloc_func);
        if (!execute_malloc_function(module_inst, malloc_func,
                                     size, &offset)) {
            return 0;
        }
        addr = offset
               ? (uint8*)memory_inst->memory_data.ptr + offset
               : NULL;
    }

    if (!addr) {
        aot_set_exception(module_inst, "out of memory");
        return 0;
    }
    if (p_native_addr)
        *p_native_addr = addr;
    return (uint32)(addr - (uint8*)memory_inst->memory_data.ptr);
}

void
aot_module_free(AOTModuleInstance *module_inst, uint32 ptr)
{
    AOTMemoryInstance *memory_inst = aot_get_default_memory(module_inst);
    AOTModule *module = (AOTModule *)module_inst->aot_module.ptr;

    if (!memory_inst) {
        return;
    }

    if (ptr) {
        uint8 *addr = (uint8 *)memory_inst->memory_data.ptr + ptr;
        if (memory_inst->heap_handle.ptr
            &&(uint8 *)memory_inst->heap_data.ptr < addr
            && addr < (uint8 *)memory_inst->heap_data_end.ptr) {
            mem_allocator_free(memory_inst->heap_handle.ptr, addr);
        }
        else if (module->malloc_func_index != (uint32)-1
                 && module->free_func_index != (uint32)-1
                 && (uint8 *)memory_inst->memory_data.ptr <= addr
                 && addr < (uint8 *)memory_inst->memory_data_end.ptr) {
            AOTFunctionInstance *free_func =
                aot_lookup_function(module_inst, "free", "(i)i");

            bh_assert(free_func);
            execute_free_function(module_inst, free_func, ptr);
        }
    }
}

uint32
aot_module_dup_data(AOTModuleInstance *module_inst,
                    const char *src, uint32 size)
{
    char *buffer;
    uint32 buffer_offset = aot_module_malloc(module_inst, size,
                                             (void**)&buffer);

    if (buffer_offset != 0) {
        buffer = aot_addr_app_to_native(module_inst, buffer_offset);
        bh_memcpy_s(buffer, size, src, size);
    }
    return buffer_offset;
}

bool
aot_validate_app_addr(AOTModuleInstance *module_inst,
                      uint32 app_offset, uint32 size)
{
    AOTMemoryInstance *memory_inst = aot_get_default_memory(module_inst);

    /* integer overflow check */
    if(app_offset + size < app_offset) {
        goto fail;
    }
    if (app_offset + size <= memory_inst->memory_data_size) {
        return true;
    }
fail:
    aot_set_exception(module_inst, "out of bounds memory access");
    return false;
}

bool
aot_validate_native_addr(AOTModuleInstance *module_inst,
                         void *native_ptr, uint32 size)
{
    uint8 *addr = (uint8 *)native_ptr;
    AOTMemoryInstance *memory_inst = aot_get_default_memory(module_inst);

    /* integer overflow check */
    if (addr + size < addr) {
        goto fail;
    }

    if ((uint8 *)memory_inst->memory_data.ptr <= addr
        && addr + size <= (uint8 *)memory_inst->memory_data_end.ptr)
        return true;
fail:
    aot_set_exception(module_inst, "out of bounds memory access");
    return false;
}

void *
aot_addr_app_to_native(AOTModuleInstance *module_inst, uint32 app_offset)
{
    AOTMemoryInstance *memory_inst = aot_get_default_memory(module_inst);
    uint8 *addr = (uint8 *)memory_inst->memory_data.ptr + app_offset;

    if ((uint8 *)memory_inst->memory_data.ptr <= addr
        && addr < (uint8 *)memory_inst->memory_data_end.ptr)
        return addr;
    return NULL;
}

uint32
aot_addr_native_to_app(AOTModuleInstance *module_inst, void *native_ptr)
{
    uint8 *addr = (uint8 *)native_ptr;
    AOTMemoryInstance *memory_inst = aot_get_default_memory(module_inst);

    if ((uint8 *)memory_inst->memory_data.ptr <= addr
        && addr < (uint8 *)memory_inst->memory_data_end.ptr)
        return (uint32)(addr - (uint8 *)memory_inst->memory_data.ptr);
    return 0;
}

bool
aot_get_app_addr_range(AOTModuleInstance *module_inst,
                       uint32 app_offset,
                       uint32 *p_app_start_offset,
                       uint32 *p_app_end_offset)
{
    AOTMemoryInstance *memory_inst = aot_get_default_memory(module_inst);
    uint32 memory_data_size = memory_inst->memory_data_size;

    if (app_offset < memory_data_size) {
        if (p_app_start_offset)
            *p_app_start_offset = 0;
        if (p_app_end_offset)
            *p_app_end_offset = memory_data_size;
        return true;
    }
    return false;
}

bool
aot_get_native_addr_range(AOTModuleInstance *module_inst,
                          uint8 *native_ptr,
                          uint8 **p_native_start_addr,
                          uint8 **p_native_end_addr)
{
    uint8 *addr = (uint8 *)native_ptr;
    AOTMemoryInstance *memory_inst = aot_get_default_memory(module_inst);

    if ((uint8 *)memory_inst->memory_data.ptr <= addr
        && addr < (uint8 *)memory_inst->memory_data_end.ptr) {
        if (p_native_start_addr)
            *p_native_start_addr = (uint8 *)memory_inst->memory_data.ptr;
        if (p_native_end_addr)
            *p_native_end_addr = (uint8 *)memory_inst->memory_data_end.ptr;
        return true;
    }
    return false;
}

#ifndef OS_ENABLE_HW_BOUND_CHECK
bool
aot_enlarge_memory(AOTModuleInstance *module_inst, uint32 inc_page_count)
{
    AOTMemoryInstance *memory_inst = aot_get_default_memory(module_inst);
    uint32 num_bytes_per_page = memory_inst->num_bytes_per_page;
    uint32 cur_page_count = memory_inst->cur_page_count;
    uint32 max_page_count = memory_inst->max_page_count;
    uint32 total_page_count = cur_page_count + inc_page_count;
    uint32 total_size_old = memory_inst->memory_data_size;
    uint64 total_size = (uint64)num_bytes_per_page * total_page_count;
    uint32 heap_size = (uint32)((uint8 *)memory_inst->heap_data_end.ptr
                                - (uint8 *)memory_inst->heap_data.ptr);
    uint8 *memory_data_old = (uint8 *)memory_inst->memory_data.ptr;
    uint8 *heap_data_old = (uint8 *)memory_inst->heap_data.ptr;
    uint8 *memory_data, *heap_data;
    void *heap_handle_old = memory_inst->heap_handle.ptr;

    if (inc_page_count <= 0)
        /* No need to enlarge memory */
        return true;

    if (total_page_count < cur_page_count /* integer overflow */
        || total_page_count > max_page_count) {
        return false;
    }

    if (total_size >= UINT32_MAX) {
        return false;
    }

#if WASM_ENABLE_SHARED_MEMORY != 0
    if (memory_inst->is_shared) {
        /* For shared memory, we have reserved the maximum spaces during
            instantiate, only change the cur_page_count here */
        memory_inst->cur_page_count = total_page_count;
        return true;
    }
#endif

    if (heap_size > 0) {
        /* Destroy heap's lock firstly, if its memory is re-allocated,
           we cannot access its lock again. */
        mem_allocator_destroy_lock(memory_inst->heap_handle.ptr);
    }
    if (!(memory_data = wasm_runtime_realloc(memory_data_old,
                                             (uint32)total_size))) {
        if (!(memory_data = wasm_runtime_malloc((uint32)total_size))) {
            if (heap_size > 0) {
                /* Restore heap's lock if memory re-alloc failed */
                mem_allocator_reinit_lock(memory_inst->heap_handle.ptr);
            }
            return false;
        }
        bh_memcpy_s(memory_data, (uint32)total_size,
                    memory_data_old, total_size_old);
        wasm_runtime_free(memory_data_old);
    }

    memset(memory_data + total_size_old,
           0, (uint32)total_size - total_size_old);

    memory_inst->cur_page_count = total_page_count;
    memory_inst->memory_data_size = (uint32)total_size;
    memory_inst->memory_data.ptr = memory_data;
    memory_inst->memory_data_end.ptr = memory_data + total_size;

    if (heap_size > 0) {
        memory_inst->heap_handle.ptr = (uint8 *)heap_handle_old
                                       + (memory_data - memory_data_old);
        if (mem_allocator_migrate(memory_inst->heap_handle.ptr,
                                  heap_handle_old) != 0) {
            return false;
        }
    }

    heap_data = heap_data_old + (memory_data - memory_data_old);
    memory_inst->heap_data.ptr = heap_data;
    memory_inst->heap_data_end.ptr = heap_data + heap_size;

    if (sizeof(uintptr_t) == sizeof(uint64)) {
        memory_inst->mem_bound_check_1byte.u64 = total_size - 1;
        memory_inst->mem_bound_check_2bytes.u64 = total_size - 2;
        memory_inst->mem_bound_check_4bytes.u64 = total_size - 4;
        memory_inst->mem_bound_check_8bytes.u64 = total_size - 8;
    }
    else {
        memory_inst->mem_bound_check_1byte.u32[0] = (uint32)total_size - 1;
        memory_inst->mem_bound_check_2bytes.u32[0] = (uint32)total_size - 2;
        memory_inst->mem_bound_check_4bytes.u32[0] = (uint32)total_size - 4;
        memory_inst->mem_bound_check_8bytes.u32[0] = (uint32)total_size - 8;
    }
    return true;
}
#else /* else of OS_ENABLE_HW_BOUND_CHECK */
bool
aot_enlarge_memory(AOTModuleInstance *module_inst, uint32 inc_page_count)
{
    AOTMemoryInstance *memory_inst = aot_get_default_memory(module_inst);
    uint32 num_bytes_per_page = memory_inst->num_bytes_per_page;
    uint32 cur_page_count = memory_inst->cur_page_count;
    uint32 max_page_count = memory_inst->max_page_count;
    uint32 total_page_count = cur_page_count + inc_page_count;
    uint64 total_size = (uint64)num_bytes_per_page * total_page_count;

    if (inc_page_count <= 0)
        /* No need to enlarge memory */
        return true;

    if (total_page_count < cur_page_count /* integer overflow */
        || total_page_count > max_page_count) {
        return false;
    }

    if (os_mprotect(memory_inst->memory_data_end.ptr,
                    num_bytes_per_page * inc_page_count,
                    MMAP_PROT_READ | MMAP_PROT_WRITE) != 0) {
        return false;
    }

    memset(memory_inst->memory_data_end.ptr, 0,
           num_bytes_per_page * inc_page_count);

    memory_inst->cur_page_count = total_page_count;
    memory_inst->memory_data_size = (uint32)total_size;
    memory_inst->memory_data_end.ptr = (uint8 *)memory_inst->memory_data.ptr
                                       + (uint32)total_size;

    if (sizeof(uintptr_t) == sizeof(uint64)) {
        memory_inst->mem_bound_check_1byte.u64 = total_size - 1;
        memory_inst->mem_bound_check_2bytes.u64 = total_size - 2;
        memory_inst->mem_bound_check_4bytes.u64 = total_size - 4;
        memory_inst->mem_bound_check_8bytes.u64 = total_size - 8;
    }
    else {
        memory_inst->mem_bound_check_1byte.u32[0] = (uint32)total_size - 1;
        memory_inst->mem_bound_check_2bytes.u32[0] = (uint32)total_size - 2;
        memory_inst->mem_bound_check_4bytes.u32[0] = (uint32)total_size - 4;
        memory_inst->mem_bound_check_8bytes.u32[0] = (uint32)total_size - 8;
    }
    return true;
}
#endif /* end of OS_ENABLE_HW_BOUND_CHECK */

bool
aot_is_wasm_type_equal(AOTModuleInstance *module_inst,
                       uint32 type1_idx, uint32 type2_idx)
{
    WASMType *type1, *type2;
    AOTModule *module = (AOTModule*)module_inst->aot_module.ptr;

    if (type1_idx >= module->func_type_count
        || type2_idx >= module->func_type_count) {
        aot_set_exception(module_inst, "type index out of bounds");
        return false;
    }

    if (type1_idx == type2_idx)
        return true;

    type1 = module->func_types[type1_idx];
    type2 = module->func_types[type2_idx];

    return wasm_type_equal(type1, type2);
}

bool
aot_invoke_native(WASMExecEnv *exec_env, uint32 func_idx,
                  uint32 argc, uint32 *argv)
{
    AOTModuleInstance *module_inst = (AOTModuleInstance*)
                            wasm_runtime_get_module_inst(exec_env);
    AOTModule *aot_module = (AOTModule*)module_inst->aot_module.ptr;
    uint32 *func_type_indexes = (uint32*)module_inst->func_type_indexes.ptr;
    uint32 func_type_idx = func_type_indexes[func_idx];
    AOTFuncType *func_type = aot_module->func_types[func_type_idx];
    void **func_ptrs = (void**)module_inst->func_ptrs.ptr;
    void *func_ptr = func_ptrs[func_idx];
    AOTImportFunc *import_func;
    const char *signature;
    void *attachment;
    char buf[128];

#ifdef OS_ENABLE_HW_BOUND_CHECK
    uint32 page_size = os_getpagesize();
    uint32 guard_page_count = STACK_OVERFLOW_CHECK_GUARD_PAGE_COUNT;
    /* Check native stack overflow firstly to ensure we have enough
       native stack to run the following codes before actually calling
       the aot function in invokeNative function. */
    if ((uint8*)&module_inst < exec_env->native_stack_boundary
                               + page_size * (guard_page_count + 1)) {
        aot_set_exception_with_id(module_inst, EXCE_NATIVE_STACK_OVERFLOW);
        return false;
    }
#endif

    bh_assert(func_idx < aot_module->import_func_count);

    import_func = aot_module->import_funcs + func_idx;
    if (!func_ptr) {
        snprintf(buf, sizeof(buf),
                 "fail to call unlinked import function (%s, %s)",
                 import_func->module_name, import_func->func_name);
        aot_set_exception(module_inst, buf);
        return false;
    }

    signature = import_func->signature;
    attachment = import_func->attachment;
    if (!import_func->call_conv_raw) {
        return wasm_runtime_invoke_native(exec_env, func_ptr,
                                          func_type, signature, attachment,
                                          argv, argc, argv);
    }
    else {
        return wasm_runtime_invoke_native_raw(exec_env, func_ptr,
                                              func_type, signature, attachment,
                                              argv, argc, argv);
    }
}

bool
aot_call_indirect(WASMExecEnv *exec_env,
                  bool check_func_type, uint32 func_type_idx,
                  uint32 table_elem_idx,
                  uint32 argc, uint32 *argv)
{
    AOTModuleInstance *module_inst = (AOTModuleInstance*)
                                     wasm_runtime_get_module_inst(exec_env);
    AOTModule *aot_module = (AOTModule*)module_inst->aot_module.ptr;
    uint32 *func_type_indexes = (uint32*)module_inst->func_type_indexes.ptr;
    uint32 *table_data = (uint32*)module_inst->table_data.ptr;
    AOTFuncType *func_type;
    void **func_ptrs = (void**)module_inst->func_ptrs.ptr, *func_ptr;
    uint32 table_size = module_inst->table_size;
    uint32 func_idx, func_type_idx1;
    uint32 ext_ret_count;
    AOTImportFunc *import_func;
    const char *signature = NULL;
    void *attachment = NULL;
    char buf[128];
    bool ret;

    /* this function is called from native code, so exec_env->handle and
       exec_env->native_stack_boundary must have been set, we don't set
       it again */

    if ((uint8*)&module_inst < exec_env->native_stack_boundary) {
        aot_set_exception_with_id(module_inst, EXCE_NATIVE_STACK_OVERFLOW);
        return false;
    }

    if (table_elem_idx >= table_size) {
        aot_set_exception_with_id(module_inst, EXCE_UNDEFINED_ELEMENT);
        return false;
    }

    func_idx = table_data[table_elem_idx];
    if (func_idx == (uint32)-1) {
        aot_set_exception_with_id(module_inst, EXCE_UNINITIALIZED_ELEMENT);
        return false;
    }

    func_type_idx1 = func_type_indexes[func_idx];
    if (check_func_type
        && !aot_is_wasm_type_equal(module_inst, func_type_idx,
                                   func_type_idx1)) {
        aot_set_exception_with_id(module_inst,
                                  EXCE_INVALID_FUNCTION_TYPE_INDEX);
        return false;
    }
    func_type = aot_module->func_types[func_type_idx1];

    if (!(func_ptr = func_ptrs[func_idx])) {
        bh_assert(func_idx < aot_module->import_func_count);
        import_func = aot_module->import_funcs + func_idx;
        snprintf(buf, sizeof(buf),
                 "fail to call unlinked import function (%s, %s)",
                 import_func->module_name, import_func->func_name);
        aot_set_exception(module_inst, buf);
        return false;
    }

    if (func_idx < aot_module->import_func_count) {
        /* Call native function */
        import_func = aot_module->import_funcs + func_idx;
        signature = import_func->signature;
        if (import_func->call_conv_raw) {
            attachment = import_func->attachment;
            return wasm_runtime_invoke_native_raw(exec_env, func_ptr,
                                                  func_type, signature,
                                                  attachment,
                                                  argv, argc, argv);
        }
    }

    ext_ret_count = func_type->result_count > 1
                    ? func_type->result_count - 1 : 0;
    if (ext_ret_count > 0) {
        uint32 argv1_buf[32], *argv1 = argv1_buf;
        uint32 *ext_rets = NULL, *argv_ret = argv;
        uint32 cell_num = 0, i;
        uint8 *ext_ret_types = func_type->types + func_type->param_count + 1;
        uint32 ext_ret_cell = wasm_get_cell_num(ext_ret_types, ext_ret_count);
        uint64 size;

        /* Allocate memory all arguments */
        size = sizeof(uint32) * (uint64)argc            /* original arguments */
               + sizeof(void*) * (uint64)ext_ret_count  /* extra result values' addr */
               + sizeof(uint32) * (uint64)ext_ret_cell; /* extra result values */
        if (size > sizeof(argv1_buf)
            && !(argv1 = runtime_malloc(size, module_inst->cur_exception,
                                        sizeof(module_inst->cur_exception)))) {
            aot_set_exception_with_id(module_inst, EXCE_OUT_OF_MEMORY);
            return false;
        }

        /* Copy original arguments */
        bh_memcpy_s(argv1, (uint32)size, argv, sizeof(uint32) * argc);

        /* Get the extra result value's address */
        ext_rets = argv1 + argc + sizeof(void*)/sizeof(uint32) * ext_ret_count;

        /* Append each extra result value's address to original arguments */
        for (i = 0; i < ext_ret_count; i++) {
            *(uintptr_t*)(argv1 + argc + sizeof(void*) / sizeof(uint32) * i) =
                (uintptr_t)(ext_rets + cell_num);
            cell_num += wasm_value_type_cell_num(ext_ret_types[i]);
        }

        ret = invoke_native_internal(exec_env, func_ptr,
                                     func_type, signature, attachment,
                                     argv1, argc, argv);
        if (!ret || aot_get_exception(module_inst)) {
            if (argv1 != argv1_buf)
                wasm_runtime_free(argv1);
            return false;
        }

        /* Get extra result values */
        switch (func_type->types[func_type->param_count]) {
            case VALUE_TYPE_I32:
            case VALUE_TYPE_F32:
                argv_ret++;
                break;
            case VALUE_TYPE_I64:
            case VALUE_TYPE_F64:
                argv_ret += 2;
                break;
            default:
                bh_assert(0);
                break;
        }
        ext_rets = argv1 + argc + sizeof(void*)/sizeof(uint32) * ext_ret_count;
        bh_memcpy_s(argv_ret, sizeof(uint32) * cell_num,
                    ext_rets, sizeof(uint32) * cell_num);

        if (argv1 != argv1_buf)
            wasm_runtime_free(argv1);

        return true;
    }
    else {
        return invoke_native_internal(exec_env, func_ptr,
                                      func_type, signature, attachment,
                                      argv, argc, argv);
    }
}

#if WASM_ENABLE_BULK_MEMORY != 0
bool
aot_memory_init(AOTModuleInstance *module_inst, uint32 seg_index,
                uint32 offset, uint32 len, uint32 dst)
{
    AOTMemoryInstance *memory_inst = aot_get_default_memory(module_inst);
    AOTModule *aot_module;
    uint8 *data = NULL;
    uint8 *maddr;
    uint64 seg_len = 0;

    aot_module = (AOTModule *)module_inst->aot_module.ptr;
    if (aot_module->is_jit_mode) {
#if WASM_ENABLE_JIT != 0
        seg_len = aot_module->wasm_module->data_segments[seg_index]->data_length;
        data = aot_module->wasm_module->data_segments[seg_index]->data;
#endif
    }
    else {
        seg_len = aot_module->mem_init_data_list[seg_index]->byte_count;
        data = aot_module->mem_init_data_list[seg_index]->bytes;
    }

    if (!aot_validate_app_addr(module_inst, dst, len))
        return false;

    if ((uint64)offset + (uint64)len > seg_len) {
        aot_set_exception(module_inst, "out of bounds memory access");
        return false;
    }

    maddr = aot_addr_app_to_native(module_inst, dst);

    bh_memcpy_s(maddr, memory_inst->memory_data_size - dst,
                data + offset, len);
    return true;
}

bool
aot_data_drop(AOTModuleInstance *module_inst, uint32 seg_index)
{
    AOTModule *aot_module = (AOTModule *)(module_inst->aot_module.ptr);

    if (aot_module->is_jit_mode) {
#if WASM_ENABLE_JIT != 0
        aot_module->wasm_module->data_segments[seg_index]->data_length = 0;
        /* Currently we can't free the dropped data segment
            as they are stored in wasm bytecode */
#endif
    }
    else {
        aot_module->mem_init_data_list[seg_index]->byte_count = 0;
        /* Currently we can't free the dropped data segment
            as the mem_init_data_count is a continuous array */
    }
    return true;
}
#endif /* WASM_ENABLE_BULK_MEMORY */

#if WASM_ENABLE_THREAD_MGR != 0
bool
aot_set_aux_stack(WASMExecEnv *exec_env,
                  uint32 start_offset, uint32 size)
{
    AOTModuleInstance *module_inst =
        (AOTModuleInstance*)exec_env->module_inst;
    AOTModule *module = (AOTModule *)module_inst->aot_module.ptr;

    uint32 stack_top_idx = module->aux_stack_top_global_index;
    uint32 data_end = module->aux_data_end;
    uint32 stack_bottom = module->aux_stack_bottom;
    bool is_stack_before_data = stack_bottom < data_end ? true : false;

    /* Check the aux stack space, currently we don't allocate space in heap */
    if ((is_stack_before_data && (size > start_offset))
        || ((!is_stack_before_data) && (start_offset - data_end < size)))
        return false;

    if (stack_top_idx != (uint32)-1) {
        /* The aux stack top is a wasm global,
            set the initial value for the global */
        uint32 global_offset =
                module->globals[stack_top_idx].data_offset;
        uint8 *global_addr = (uint8 *)module_inst->global_data.ptr + global_offset;
        *(int32*)global_addr = start_offset;

        /* The aux stack boundary is a constant value,
            set the value to exec_env */
        exec_env->aux_stack_boundary = start_offset - size;
        return true;
    }

    return false;
}

bool
aot_get_aux_stack(WASMExecEnv *exec_env,
                  uint32 *start_offset, uint32 *size)
{
    AOTModuleInstance *module_inst =
        (AOTModuleInstance*)exec_env->module_inst;
    AOTModule *module = (AOTModule *)module_inst->aot_module.ptr;

    /* The aux stack information is resolved in loader
        and store in module */
    uint32 stack_bottom = module->aux_stack_bottom;
    uint32 total_aux_stack_size = module->aux_stack_size;

    if (stack_bottom != 0 && total_aux_stack_size != 0) {
        if (start_offset)
            *start_offset = stack_bottom;
        if (size)
            *size = total_aux_stack_size;
        return true;
    }
    return false;
}

#endif

#if (WASM_ENABLE_MEMORY_PROFILING != 0) || (WASM_ENABLE_MEMORY_TRACING != 0)
static uint32 const_string_size;

void const_string_node_size_cb(void *key, void *value)
{
    const_string_size += bh_hash_map_get_elem_struct_size();
    const_string_size += strlen((const char *)value) + 1;
}

void
aot_get_module_mem_consumption(const AOTModule *module,
                               WASMModuleMemConsumption *mem_conspn)
{
    uint32 i, size;

    memset(mem_conspn, 0, sizeof(*mem_conspn));

    mem_conspn->module_struct_size = sizeof(AOTModule);

    mem_conspn->types_size = sizeof(AOTFuncType *) * module->func_type_count;
    for (i = 0; i < module->func_type_count; i++) {
        AOTFuncType *type = module->func_types[i];
        size = offsetof(AOTFuncType, types) +
               sizeof(uint8) * (type->param_count + type->result_count);
        mem_conspn->types_size += size;
    }

    mem_conspn->imports_size =
        sizeof(AOTImportMemory) * module->import_memory_count
        + sizeof(AOTImportTable) * module->import_table_count
        + sizeof(AOTImportGlobal) * module->import_global_count
        + sizeof(AOTImportFunc) * module->import_func_count;

    /* func_ptrs and func_type_indexes */
    mem_conspn->functions_size =
        (sizeof(void *) + sizeof(uint32)) * module->func_count;

    mem_conspn->tables_size = sizeof(AOTTable) * module->table_count;

    mem_conspn->memories_size = sizeof(AOTMemory) * module->memory_count;
    mem_conspn->globals_size = sizeof(AOTGlobal) * module->global_count;
    mem_conspn->exports_size = sizeof(AOTExport) * module->export_count;

    mem_conspn->table_segs_size =
        sizeof(AOTTableInitData *) * module->table_init_data_count;
    for (i = 0; i < module->table_init_data_count; i++) {
        AOTTableInitData *init_data = module->table_init_data_list[i];
        size = offsetof(AOTTableInitData, func_indexes)
               + sizeof(uint32) * init_data->func_index_count;
        mem_conspn->table_segs_size += size;
    }

    mem_conspn->data_segs_size = sizeof(AOTMemInitData *)
                                 * module->mem_init_data_count;
    for (i = 0; i < module->mem_init_data_count; i++) {
        mem_conspn->data_segs_size += sizeof(AOTMemInitData);
    }

    mem_conspn->const_strs_size =
        bh_hash_map_get_struct_size(module->const_str_set);

    const_string_size = 0;
    if (module->const_str_set) {
        bh_hash_map_traverse(module->const_str_set,
                             const_string_node_size_cb);
    }
    mem_conspn->const_strs_size += const_string_size;

    /* code size + literal size + object data section size */
    mem_conspn->aot_code_size = module->code_size + module->literal_size
        + sizeof(AOTObjectDataSection) * module->data_section_count;
    for (i = 0; i < module->data_section_count; i++) {
        AOTObjectDataSection *obj_data = module->data_sections + i;
        mem_conspn->aot_code_size += sizeof(uint8) * obj_data->size;
    }

    mem_conspn->total_size += mem_conspn->module_struct_size;
    mem_conspn->total_size += mem_conspn->types_size;
    mem_conspn->total_size += mem_conspn->imports_size;
    mem_conspn->total_size += mem_conspn->functions_size;
    mem_conspn->total_size += mem_conspn->tables_size;
    mem_conspn->total_size += mem_conspn->memories_size;
    mem_conspn->total_size += mem_conspn->globals_size;
    mem_conspn->total_size += mem_conspn->exports_size;
    mem_conspn->total_size += mem_conspn->table_segs_size;
    mem_conspn->total_size += mem_conspn->data_segs_size;
    mem_conspn->total_size += mem_conspn->const_strs_size;
    mem_conspn->total_size += mem_conspn->aot_code_size;
}

void
aot_get_module_inst_mem_consumption(const AOTModuleInstance *module_inst,
                                    WASMModuleInstMemConsumption *mem_conspn)
{
    uint32 i;

    memset(mem_conspn, 0, sizeof(*mem_conspn));

    mem_conspn->module_inst_struct_size = sizeof(AOTModuleInstance);

    mem_conspn->memories_size =
        sizeof(AOTPointer) * module_inst->memory_count
        + sizeof(AOTMemoryInstance) * module_inst->memory_count;
    for (i = 0; i < module_inst->memory_count; i++) {
        AOTMemoryInstance *mem_inst =
            ((AOTMemoryInstance **)module_inst->memories.ptr)[i];
        mem_conspn->memories_size +=
            mem_inst->num_bytes_per_page * mem_inst->cur_page_count;
        mem_conspn->app_heap_size =
            mem_inst->heap_data_end.ptr - mem_inst->heap_data.ptr;
    }

    mem_conspn->tables_size = sizeof(uint32) * module_inst->table_size;

    /* func_ptrs and func_type_indexes */
    mem_conspn->functions_size =  (sizeof(void *) + sizeof(uint32)) *
        (((AOTModule *)module_inst->aot_module.ptr)->import_func_count
         + ((AOTModule *)module_inst->aot_module.ptr)->func_count);

    mem_conspn->globals_size = module_inst->global_data_size;

    mem_conspn->exports_size =
        sizeof(AOTFunctionInstance) * (uint64)module_inst->export_func_count;

    mem_conspn->total_size += mem_conspn->module_inst_struct_size;
    mem_conspn->total_size += mem_conspn->memories_size;
    mem_conspn->total_size += mem_conspn->functions_size;
    mem_conspn->total_size += mem_conspn->tables_size;
    mem_conspn->total_size += mem_conspn->globals_size;
    mem_conspn->total_size += mem_conspn->exports_size;
}
#endif /* end of (WASM_ENABLE_MEMORY_PROFILING != 0)
                 || (WASM_ENABLE_MEMORY_TRACING != 0) */
