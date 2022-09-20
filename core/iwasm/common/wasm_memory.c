/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_runtime_common.h"
#include "../interpreter/wasm_runtime.h"
#include "bh_platform.h"
#include "mem_alloc.h"

typedef enum Memory_Mode {
    MEMORY_MODE_UNKNOWN = 0,
    MEMORY_MODE_POOL,
    MEMORY_MODE_ALLOCATOR
} Memory_Mode;

static Memory_Mode memory_mode = MEMORY_MODE_UNKNOWN;

static mem_allocator_t pool_allocator = NULL;

static void *(*malloc_func)(unsigned int size) = NULL;
static void *(*realloc_func)(void *ptr, unsigned int size) = NULL;
static void (*free_func)(void *ptr) = NULL;

static unsigned int global_pool_size;

static bool
wasm_memory_init_with_pool(void *mem, unsigned int bytes)
{
    mem_allocator_t _allocator = mem_allocator_create(mem, bytes);

    if (_allocator) {
        memory_mode = MEMORY_MODE_POOL;
        pool_allocator = _allocator;
        global_pool_size = bytes;
        return true;
    }
    LOG_ERROR("Init memory with pool (%p, %u) failed.\n", mem, bytes);
    return false;
}

static bool
wasm_memory_init_with_allocator(void *_malloc_func, void *_realloc_func,
                                void *_free_func)
{
    if (_malloc_func && _free_func && _malloc_func != _free_func) {
        memory_mode = MEMORY_MODE_ALLOCATOR;
        malloc_func = _malloc_func;
        realloc_func = _realloc_func;
        free_func = _free_func;
        return true;
    }
    LOG_ERROR("Init memory with allocator (%p, %p, %p) failed.\n", _malloc_func,
              _realloc_func, _free_func);
    return false;
}

bool
wasm_runtime_memory_init(mem_alloc_type_t mem_alloc_type,
                         const MemAllocOption *alloc_option)
{
    if (mem_alloc_type == Alloc_With_Pool)
        return wasm_memory_init_with_pool(alloc_option->pool.heap_buf,
                                          alloc_option->pool.heap_size);
    else if (mem_alloc_type == Alloc_With_Allocator)
        return wasm_memory_init_with_allocator(
            alloc_option->allocator.malloc_func,
            alloc_option->allocator.realloc_func,
            alloc_option->allocator.free_func);
    else if (mem_alloc_type == Alloc_With_System_Allocator)
        return wasm_memory_init_with_allocator(os_malloc, os_realloc, os_free);
    else
        return false;
}

void
wasm_runtime_memory_destroy()
{
    if (memory_mode == MEMORY_MODE_POOL) {
#if BH_ENABLE_GC_VERIFY == 0
        (void)mem_allocator_destroy(pool_allocator);
#else
        int ret = mem_allocator_destroy(pool_allocator);
        if (ret != 0) {
            /* Memory leak detected */
            exit(-1);
        }
#endif
    }
    memory_mode = MEMORY_MODE_UNKNOWN;
}

unsigned
wasm_runtime_memory_pool_size()
{
    if (memory_mode == MEMORY_MODE_POOL)
        return global_pool_size;
    else
        return UINT32_MAX;
}

static inline void *
wasm_runtime_malloc_internal(unsigned int size)
{
    if (memory_mode == MEMORY_MODE_UNKNOWN) {
        LOG_WARNING(
            "wasm_runtime_malloc failed: memory hasn't been initialize.\n");
        return NULL;
    }
    else if (memory_mode == MEMORY_MODE_POOL) {
        return mem_allocator_malloc(pool_allocator, size);
    }
    else {
        return malloc_func(size);
    }
}

static inline void *
wasm_runtime_realloc_internal(void *ptr, unsigned int size)
{
    if (memory_mode == MEMORY_MODE_UNKNOWN) {
        LOG_WARNING(
            "wasm_runtime_realloc failed: memory hasn't been initialize.\n");
        return NULL;
    }
    else if (memory_mode == MEMORY_MODE_POOL) {
        return mem_allocator_realloc(pool_allocator, ptr, size);
    }
    else {
        if (realloc_func)
            return realloc_func(ptr, size);
        else
            return NULL;
    }
}

static inline void
wasm_runtime_free_internal(void *ptr)
{
    if (!ptr) {
        LOG_WARNING("warning: wasm_runtime_free with NULL pointer\n");
#if BH_ENABLE_GC_VERIFY != 0
        exit(-1);
#endif
        return;
    }

    if (memory_mode == MEMORY_MODE_UNKNOWN) {
        LOG_WARNING("warning: wasm_runtime_free failed: "
                    "memory hasn't been initialize.\n");
    }
    else if (memory_mode == MEMORY_MODE_POOL) {
        mem_allocator_free(pool_allocator, ptr);
    }
    else {
        free_func(ptr);
    }
}

void *
wasm_runtime_malloc(unsigned int size)
{
    if (size == 0) {
        LOG_WARNING("warning: wasm_runtime_malloc with size zero\n");
        /* At lease alloc 1 byte to avoid malloc failed */
        size = 1;
#if BH_ENABLE_GC_VERIFY != 0
        exit(-1);
#endif
    }

    return wasm_runtime_malloc_internal(size);
}

void *
wasm_runtime_realloc(void *ptr, unsigned int size)
{
    return wasm_runtime_realloc_internal(ptr, size);
}

void
wasm_runtime_free(void *ptr)
{
    wasm_runtime_free_internal(ptr);
}

bool
wasm_runtime_get_mem_alloc_info(mem_alloc_info_t *mem_alloc_info)
{
    if (memory_mode == MEMORY_MODE_POOL) {
        return mem_allocator_get_alloc_info(pool_allocator, mem_alloc_info);
    }
    return false;
}

static WASMMemoryInstance *
get_default_memory(WASMModuleInstance *module_inst)
{
    if (module_inst->memories)
        return module_inst->memories[0];
    else
        return NULL;
}

bool
wasm_runtime_validate_app_addr(WASMModuleInstanceCommon *module_inst_comm,
                               uint32 app_offset, uint32 size)
{
    WASMModuleInstance *module_inst = (WASMModuleInstance *)module_inst_comm;
    WASMMemoryInstance *memory_inst;

    bh_assert(module_inst_comm->module_type == Wasm_Module_Bytecode
              || module_inst_comm->module_type == Wasm_Module_AoT);

    memory_inst = get_default_memory(module_inst);
    if (!memory_inst) {
        goto fail;
    }

    /* integer overflow check */
    if (app_offset > UINT32_MAX - size) {
        goto fail;
    }

    if (app_offset + size <= memory_inst->memory_data_size) {
        return true;
    }

fail:
    wasm_set_exception(module_inst, "out of bounds memory access");
    return false;
}

bool
wasm_runtime_validate_app_str_addr(WASMModuleInstanceCommon *module_inst_comm,
                                   uint32 app_str_offset)
{
    WASMModuleInstance *module_inst = (WASMModuleInstance *)module_inst_comm;
    uint32 app_end_offset;
    char *str, *str_end;

    bh_assert(module_inst_comm->module_type == Wasm_Module_Bytecode
              || module_inst_comm->module_type == Wasm_Module_AoT);

    if (!wasm_runtime_get_app_addr_range(module_inst_comm, app_str_offset, NULL,
                                         &app_end_offset))
        goto fail;

    str = wasm_runtime_addr_app_to_native(module_inst_comm, app_str_offset);
    str_end = str + (app_end_offset - app_str_offset);
    while (str < str_end && *str != '\0')
        str++;
    if (str == str_end)
        goto fail;

    return true;
fail:
    wasm_set_exception(module_inst, "out of bounds memory access");
    return false;
}

bool
wasm_runtime_validate_native_addr(WASMModuleInstanceCommon *module_inst_comm,
                                  void *native_ptr, uint32 size)
{
    WASMModuleInstance *module_inst = (WASMModuleInstance *)module_inst_comm;
    WASMMemoryInstance *memory_inst;
    uint8 *addr = (uint8 *)native_ptr;

    bh_assert(module_inst_comm->module_type == Wasm_Module_Bytecode
              || module_inst_comm->module_type == Wasm_Module_AoT);

    memory_inst = get_default_memory(module_inst);
    if (!memory_inst) {
        goto fail;
    }

    /* integer overflow check */
    if ((uintptr_t)addr > UINTPTR_MAX - size) {
        goto fail;
    }

    if (memory_inst->memory_data <= addr
        && addr + size <= memory_inst->memory_data_end) {
        return true;
    }

fail:
    wasm_set_exception(module_inst, "out of bounds memory access");
    return false;
}

void *
wasm_runtime_addr_app_to_native(WASMModuleInstanceCommon *module_inst_comm,
                                uint32 app_offset)
{
    WASMModuleInstance *module_inst = (WASMModuleInstance *)module_inst_comm;
    WASMMemoryInstance *memory_inst;
    uint8 *addr;

    bh_assert(module_inst_comm->module_type == Wasm_Module_Bytecode
              || module_inst_comm->module_type == Wasm_Module_AoT);

    memory_inst = get_default_memory(module_inst);
    if (!memory_inst) {
        return NULL;
    }

    addr = memory_inst->memory_data + app_offset;

    if (memory_inst->memory_data <= addr && addr < memory_inst->memory_data_end)
        return addr;

    return NULL;
}

uint32
wasm_runtime_addr_native_to_app(WASMModuleInstanceCommon *module_inst_comm,
                                void *native_ptr)
{
    WASMModuleInstance *module_inst = (WASMModuleInstance *)module_inst_comm;
    WASMMemoryInstance *memory_inst;
    uint8 *addr = (uint8 *)native_ptr;

    bh_assert(module_inst_comm->module_type == Wasm_Module_Bytecode
              || module_inst_comm->module_type == Wasm_Module_AoT);

    memory_inst = get_default_memory(module_inst);
    if (!memory_inst) {
        return 0;
    }

    if (memory_inst->memory_data <= addr && addr < memory_inst->memory_data_end)
        return (uint32)(addr - memory_inst->memory_data);

    return 0;
}

bool
wasm_runtime_get_app_addr_range(WASMModuleInstanceCommon *module_inst_comm,
                                uint32 app_offset, uint32 *p_app_start_offset,
                                uint32 *p_app_end_offset)
{
    WASMModuleInstance *module_inst = (WASMModuleInstance *)module_inst_comm;
    WASMMemoryInstance *memory_inst;
    uint32 memory_data_size;

    bh_assert(module_inst_comm->module_type == Wasm_Module_Bytecode
              || module_inst_comm->module_type == Wasm_Module_AoT);

    memory_inst = get_default_memory(module_inst);
    if (!memory_inst) {
        return false;
    }

    memory_data_size = memory_inst->memory_data_size;

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
wasm_runtime_get_native_addr_range(WASMModuleInstanceCommon *module_inst_comm,
                                   uint8 *native_ptr,
                                   uint8 **p_native_start_addr,
                                   uint8 **p_native_end_addr)
{
    WASMModuleInstance *module_inst = (WASMModuleInstance *)module_inst_comm;
    WASMMemoryInstance *memory_inst;
    uint8 *addr = (uint8 *)native_ptr;

    bh_assert(module_inst_comm->module_type == Wasm_Module_Bytecode
              || module_inst_comm->module_type == Wasm_Module_AoT);

    memory_inst = get_default_memory(module_inst);
    if (!memory_inst) {
        return false;
    }

    if (memory_inst->memory_data <= addr
        && addr < memory_inst->memory_data_end) {
        if (p_native_start_addr)
            *p_native_start_addr = memory_inst->memory_data;
        if (p_native_end_addr)
            *p_native_end_addr = memory_inst->memory_data_end;
        return true;
    }

    return false;
}

bool
wasm_check_app_addr_and_convert(WASMModuleInstance *module_inst, bool is_str,
                                uint32 app_buf_addr, uint32 app_buf_size,
                                void **p_native_addr)
{
    WASMMemoryInstance *memory_inst = get_default_memory(module_inst);
    uint8 *native_addr;

    if (!memory_inst) {
        goto fail;
    }

    native_addr = memory_inst->memory_data + app_buf_addr;

    /* No need to check the app_offset and buf_size if memory access
       boundary check with hardware trap is enabled */
#ifndef OS_ENABLE_HW_BOUND_CHECK
    if (app_buf_addr >= memory_inst->memory_data_size) {
        goto fail;
    }

    if (!is_str) {
        if (app_buf_size > memory_inst->memory_data_size - app_buf_addr) {
            goto fail;
        }
    }
    else {
        const char *str, *str_end;

        /* The whole string must be in the linear memory */
        str = (const char *)native_addr;
        str_end = (const char *)memory_inst->memory_data_end;
        while (str < str_end && *str != '\0')
            str++;
        if (str == str_end)
            goto fail;
    }
#endif

    *p_native_addr = (void *)native_addr;
    return true;
fail:
    wasm_set_exception(module_inst, "out of bounds memory access");
    return false;
}
