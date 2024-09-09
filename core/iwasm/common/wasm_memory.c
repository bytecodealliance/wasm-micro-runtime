/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_runtime_common.h"
#include "../interpreter/wasm_runtime.h"
#include "../aot/aot_runtime.h"
#include "mem_alloc.h"
#include "wasm_memory.h"

#if WASM_ENABLE_SHARED_MEMORY != 0
#include "../common/wasm_shared_memory.h"
#endif

#if WASM_ENABLE_THREAD_MGR != 0
#include "../libraries/thread-mgr/thread_manager.h"
#endif

typedef enum Memory_Mode {
    MEMORY_MODE_UNKNOWN = 0,
    MEMORY_MODE_POOL,
    MEMORY_MODE_ALLOCATOR,
    MEMORY_MODE_SYSTEM_ALLOCATOR
} Memory_Mode;

static Memory_Mode memory_mode = MEMORY_MODE_UNKNOWN;

static mem_allocator_t pool_allocator = NULL;

#if WASM_ENABLE_SHARED_HEAP != 0
static WASMSharedHeap *shared_heap_list = NULL;
static korp_mutex shared_heap_list_lock;
#endif

static enlarge_memory_error_callback_t enlarge_memory_error_cb;
static void *enlarge_memory_error_user_data;

#if WASM_MEM_ALLOC_WITH_USER_DATA != 0
static void *allocator_user_data = NULL;
#endif

static void *(*malloc_func)(
#if WASM_MEM_ALLOC_WITH_USAGE != 0
    mem_alloc_usage_t usage,
#endif
#if WASM_MEM_ALLOC_WITH_USER_DATA != 0
    void *user_data,
#endif
    unsigned int size) = NULL;

static void *(*realloc_func)(
#if WASM_MEM_ALLOC_WITH_USAGE != 0
    mem_alloc_usage_t usage, bool full_size_mmaped,
#endif
#if WASM_MEM_ALLOC_WITH_USER_DATA != 0
    void *user_data,
#endif
    void *ptr, unsigned int size) = NULL;

static void (*free_func)(
#if WASM_MEM_ALLOC_WITH_USAGE != 0
    mem_alloc_usage_t usage,
#endif
#if WASM_MEM_ALLOC_WITH_USER_DATA != 0
    void *user_data,
#endif
    void *ptr) = NULL;

static unsigned int global_pool_size;

static uint64
align_as_and_cast(uint64 size, uint64 alignment)
{
    uint64 aligned_size = (size + alignment - 1) & ~(alignment - 1);

    return aligned_size;
}

static bool
wasm_memory_init_with_pool(void *mem, unsigned int bytes)
{
    mem_allocator_t allocator = mem_allocator_create(mem, bytes);

    if (allocator) {
        memory_mode = MEMORY_MODE_POOL;
        pool_allocator = allocator;
        global_pool_size = bytes;
        return true;
    }
    LOG_ERROR("Init memory with pool (%p, %u) failed.\n", mem, bytes);
    return false;
}

#if WASM_MEM_ALLOC_WITH_USER_DATA != 0
static bool
wasm_memory_init_with_allocator(void *_user_data, void *_malloc_func,
                                void *_realloc_func, void *_free_func)
{
    if (_malloc_func && _free_func && _malloc_func != _free_func) {
        memory_mode = MEMORY_MODE_ALLOCATOR;
        allocator_user_data = _user_data;
        malloc_func = _malloc_func;
        realloc_func = _realloc_func;
        free_func = _free_func;
        return true;
    }
    LOG_ERROR("Init memory with allocator (%p, %p, %p, %p) failed.\n",
              _user_data, _malloc_func, _realloc_func, _free_func);
    return false;
}
#else
static bool
wasm_memory_init_with_allocator(void *malloc_func_ptr, void *realloc_func_ptr,
                                void *free_func_ptr)
{
    if (malloc_func_ptr && free_func_ptr && malloc_func_ptr != free_func_ptr) {
        memory_mode = MEMORY_MODE_ALLOCATOR;
        malloc_func = malloc_func_ptr;
        realloc_func = realloc_func_ptr;
        free_func = free_func_ptr;
        return true;
    }
    LOG_ERROR("Init memory with allocator (%p, %p, %p) failed.\n",
              malloc_func_ptr, realloc_func_ptr, free_func_ptr);
    return false;
}
#endif

static inline bool
is_bounds_checks_enabled(WASMModuleInstanceCommon *module_inst)
{
#if WASM_CONFIGURABLE_BOUNDS_CHECKS != 0
    if (!module_inst) {
        return true;
    }

    return wasm_runtime_is_bounds_checks_enabled(module_inst);
#else
    return true;
#endif
}

#if WASM_ENABLE_SHARED_HEAP != 0
static void *
wasm_mmap_linear_memory(uint64_t map_size, uint64 commit_size);
static void
wasm_munmap_linear_memory(void *mapped_mem, uint64 commit_size,
                          uint64 map_size);

static void
set_error_buf(char *error_buf, uint32 error_buf_size, const char *string)
{
    if (error_buf != NULL) {
        snprintf(error_buf, error_buf_size,
                 "Operation of shared heap failed: %s", string);
    }
}

static void *
runtime_malloc(uint64 size, char *error_buf, uint32 error_buf_size)
{
    void *mem;

    if (size >= UINT32_MAX || !(mem = wasm_runtime_malloc((uint32)size))) {
        set_error_buf(error_buf, error_buf_size, "allocate memory failed");
        return NULL;
    }

    memset(mem, 0, (uint32)size);
    return mem;
}

WASMSharedHeap *
wasm_runtime_create_shared_heap(SharedHeapInitArgs *init_args, char *error_buf,
                                uint32 error_buf_size)
{
    uint64 heap_struct_size = sizeof(WASMSharedHeap);
    uint32 size = init_args->size;
    WASMSharedHeap *heap;

    if (!(heap = runtime_malloc(heap_struct_size, error_buf, error_buf_size))) {
        goto fail1;
    }
    if (!(heap->heap_handle =
              runtime_malloc(mem_allocator_get_heap_struct_size(), error_buf,
                             error_buf_size))) {
        goto fail2;
    }

    size = align_uint(size, os_getpagesize());
    if (size > UINT32_MAX - APP_HEAP_SIZE_MAX + 1 || size < APP_HEAP_SIZE_MIN) {
        set_error_buf(error_buf, error_buf_size, "invalid size of shared heap");
        goto fail1;
    }

    if (!(heap->base_addr = wasm_mmap_linear_memory(size, size))) {
        goto fail3;
    }
    if (!mem_allocator_create_with_struct_and_pool(
            heap->heap_handle, heap_struct_size, heap->base_addr, size)) {
        set_error_buf(error_buf, error_buf_size, "init share heap failed");
        goto fail4;
    }

    os_mutex_lock(&shared_heap_list_lock);
    if (shared_heap_list == NULL) {
        shared_heap_list = heap;
    }
    else {
        heap->next = shared_heap_list;
        shared_heap_list = heap;
    }
    os_mutex_unlock(&shared_heap_list_lock);
    return heap;

fail4:
    wasm_munmap_linear_memory(heap->base_addr, size, size);

fail3:
    wasm_runtime_free(heap->heap_handle);
fail2:
    wasm_runtime_free(heap);
fail1:
    return NULL;
}

bool
wasm_runtime_attach_shared_heap(WASMModuleInstanceCommon *module_inst,
                                void *shared_heap)
{
#if WASM_ENABLE_THREAD_MGR != 0
    wasm_cluster_attach_shared_heap(module_inst, shared_heap);
    return true;
#else
    return wasm_runtime_attach_shared_heap_internal(module_inst, shared_heap);
#endif
}

bool
wasm_runtime_attach_shared_heap_internal(WASMModuleInstanceCommon *module_inst,
                                         void *shared_heap)
{
    uint64 linear_mem_size = 0;
    WASMMemoryInstance *memory = NULL;
    WASMSharedHeap *heap = (WASMSharedHeap *)shared_heap;

    if (module_inst->module_type == Wasm_Module_Bytecode) {
        memory = wasm_get_default_memory((WASMModuleInstance *)module_inst);
    }
    else if (module_inst->module_type == Wasm_Module_AoT) {
        // TODO
    }

    // check if linear memory and shared heap are overlapped
    linear_mem_size = memory->memory_data_size;

    if (
#if WASM_ENABLE_MEMORY64 != 0
        (memory->is_memory64 && linear_mem_size > UINT64_MAX - heap->size) ||
#endif
        (!memory->is_memory64 && linear_mem_size > UINT32_MAX - heap->size)) {
        LOG_WARNING("Linear memory address is overlapped with shared heap");
        return false;
    }

    if (module_inst->module_type == Wasm_Module_Bytecode) {
        if (((WASMModuleInstance *)module_inst)->e->shared_heap) {
            LOG_WARNING("A shared heap is already attached");
            return true;
        }
        ((WASMModuleInstance *)module_inst)->e->shared_heap = heap;
    }
    else if (module_inst->module_type == Wasm_Module_AoT) {
        // TODO
    }

    return true;
}

bool
wasm_runtime_detach_shared_heap(WASMModuleInstanceCommon *module_inst)
{
#if WASM_ENABLE_THREAD_MGR != 0
    wasm_cluster_detach_shared_heap(module_inst);
    return true;
#else
    return wasm_runtime_detach_shared_heap_internal(module_inst);
#endif
}

bool
wasm_runtime_detach_shared_heap_internal(WASMModuleInstanceCommon *module_inst)
{
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        ((WASMModuleInstance *)module_inst)->e->shared_heap = NULL;
    }
    else if (module_inst->module_type == Wasm_Module_AoT) {
        // TODO
    }

    return true;
}

bool
is_app_addr_in_shared_heap(WASMModuleInstanceCommon *module_inst_comm, bool is_memory64,
                           uint64 app_offset, uint32 bytes)
{
    WASMSharedHeap *heap = NULL;
    if (module_inst_comm->module_type == Wasm_Module_Bytecode) {
        heap = ((WASMModuleInstance *)module_inst_comm)->e->shared_heap;
    }
    else if (module_inst_comm->module_type == Wasm_Module_AoT) {
        // TODO
    }

    if (!is_memory64) {
        if (app_offset >= UINT32_MAX - heap->size + 1
            && app_offset <= UINT32_MAX - bytes + 1) {
            return true;
        }
    }
    else {
        if (app_offset >= UINT64_MAX - heap->size + 1
            && app_offset <= UINT64_MAX - bytes + 1) {
            return true;
        }
    }
    return false;
}

bool
is_native_addr_in_shared_heap(WASMModuleInstanceCommon *module_inst_comm,
                              uint8 *addr, uint32 bytes)
{
    WASMModuleInstance *module_inst = (WASMModuleInstance *)module_inst_comm;
    bh_assert(module_inst_comm->module_type == Wasm_Module_Bytecode
              || module_inst_comm->module_type == Wasm_Module_AoT);
    WASMSharedHeap *heap = module_inst->e->shared_heap;

    if (heap && heap->base_addr && addr >= heap->base_addr
        && addr + bytes <= heap->base_addr + heap->size
        && addr + bytes > addr) {
        return true;
    }
    return false;
}

static uint64
shared_heap_addr_native_to_app(WASMModuleInstanceCommon *module_inst,
                               WASMMemoryInstance *memory, void *addr)
{
    WASMSharedHeap *heap = NULL;

    if (module_inst->module_type == Wasm_Module_Bytecode) {
        heap = ((WASMModuleInstance *)module_inst)->e->shared_heap;
    }
    else if (module_inst->module_type == Wasm_Module_AoT) {
        // TODO
    }

    if (!heap) {
        LOG_WARNING("Wasm module doesn't attach to a shared heap");
        return 0;
    }
    if (addr == NULL)
        return 0;

    if (memory && memory->is_memory64) {
        return UINT64_MAX - heap->size + 1 + ((uint8 *)addr - heap->base_addr);
    }
    else if (memory && !memory->is_memory64) {
        return UINT32_MAX - heap->size + 1 + ((uint8 *)addr - heap->base_addr);
    }
    return 0;
}

static void *shared_heap_addr_app_to_native(WASMModuleInstanceCommon *module_inst, WASMMemoryInstance *memory, uint64 ptr)
{
    void *addr = NULL;
    WASMSharedHeap *heap = NULL;

    if (module_inst->module_type == Wasm_Module_Bytecode) {
        heap = ((WASMModuleInstance *)module_inst)->e->shared_heap;
    }
    else if (module_inst->module_type == Wasm_Module_AoT) {
        // TODO
    }

    if (!heap) {
        LOG_WARNING("Wasm module doesn't attach to a shared heap");
        return NULL;
    }

    if (!memory) {
        LOG_WARNING("Wasm memory is not initialized");
        return NULL;
    }

    if (is_app_addr_in_shared_heap(module_inst, memory->is_memory64, ptr, 1)) {
        if (memory->is_memory64) {
            addr = heap->base_addr + (ptr - (UINT64_MAX - heap->size + 1));
        }
        else {
            addr = heap->base_addr + (ptr - (UINT32_MAX - heap->size + 1));
        }
    }

    return addr;
}

static uint64
shared_heap_get_addr_start(WASMSharedHeap *heap, WASMMemoryInstance *memory)
{
    uint64 shared_heap_start = 0;
    uint64 heap_size = heap->size;

    if (memory && !memory->is_memory64) {
        shared_heap_start = UINT32_MAX - heap_size + 1;
    }
    else if (memory && memory->is_memory64) {
        shared_heap_start = UINT64_MAX - heap_size + 1;
    }

    return shared_heap_start;
}

uint64
wasm_runtime_shared_heap_malloc(WASMModuleInstanceCommon *module_inst,
                                uint64_t size, void **p_native_addr)
{
    WASMSharedHeap *heap = NULL;
    WASMMemoryInstance *memory = NULL;

    if (module_inst->module_type == Wasm_Module_Bytecode) {
        heap = ((WASMModuleInstance *)module_inst)->e->shared_heap;
        memory = wasm_get_default_memory((WASMModuleInstance *)module_inst);
    }
    else if (module_inst->module_type == Wasm_Module_AoT) {
        // TODO
    }

    if (heap) {
        *p_native_addr = mem_allocator_malloc(heap->heap_handle, size);

        return shared_heap_addr_native_to_app(module_inst, memory, *p_native_addr);
    }
    else {
        LOG_WARNING("Wasm module doesn't attach to a shared heap");
    }
    return 0;
}

void
wasm_runtime_shared_heap_free(wasm_module_inst_t module_inst, uint64 ptr)
{
    WASMSharedHeap *heap = NULL;
    WASMMemoryInstance *memory = NULL;
    void *addr = NULL;

    if (module_inst->module_type == Wasm_Module_Bytecode) {
        heap = ((WASMModuleInstance *)module_inst)->e->shared_heap;
        memory = wasm_get_default_memory((WASMModuleInstance *)module_inst);
    }
    else if (module_inst->module_type == Wasm_Module_AoT) {
        // TODO
    }

    if (!heap) {
        LOG_WARNING("Wasm module doesn't attach to a shared heap");
        return;
    }

    addr = shared_heap_addr_app_to_native(module_inst, memory, ptr);

    if (heap) {
        mem_allocator_free(heap->base_addr, addr);
    }
}
#endif

bool
wasm_runtime_memory_init(mem_alloc_type_t mem_alloc_type,
                         const MemAllocOption *alloc_option)
{
    bool ret = false;
#if WASM_ENABLE_SHARED_HEAP != 0
    if (os_mutex_init(&shared_heap_list_lock)) {
        return false;
    }
#endif
    if (mem_alloc_type == Alloc_With_Pool) {
        ret = wasm_memory_init_with_pool(alloc_option->pool.heap_buf,
                                         alloc_option->pool.heap_size);
    }
    else if (mem_alloc_type == Alloc_With_Allocator) {
        ret = wasm_memory_init_with_allocator(
#if WASM_MEM_ALLOC_WITH_USER_DATA != 0
            alloc_option->allocator.user_data,
#endif
            alloc_option->allocator.malloc_func,
            alloc_option->allocator.realloc_func,
            alloc_option->allocator.free_func);
    }
    else if (mem_alloc_type == Alloc_With_System_Allocator) {
        memory_mode = MEMORY_MODE_SYSTEM_ALLOCATOR;
        ret = true;
    }
#if WASM_ENABLE_SHARED_HEAP != 0
    if (!ret) {
        os_mutex_destroy(&shared_heap_list_lock);
    }
#endif

    return ret;
}

#if WASM_ENABLE_SHARED_HEAP != 0
static void
wasm_runtime_shared_heap_destroy()
{
    WASMSharedHeap *heap = shared_heap_list;
    WASMSharedHeap *cur;
    int ret = 0;

    while (heap) {
        cur = heap;
        heap = heap->next;
        ret = ret + mem_allocator_destroy(cur->heap_handle);
        wasm_runtime_free(cur->heap_handle);
        wasm_munmap_linear_memory(cur->base_addr, cur->size, cur->size);
        wasm_runtime_free(cur);
    }

    if (ret != 0) {
        LOG_ERROR("Memory leak detected in shared heap");
    }
}
#endif

void
wasm_runtime_memory_destroy(void)
{

#if WASM_ENABLE_SHARED_HEAP != 0
    wasm_runtime_shared_heap_destroy();
#endif

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
wasm_runtime_memory_pool_size(void)
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
            "wasm_runtime_malloc failed: memory hasn't been initialized.\n");
        return NULL;
    }
    else if (memory_mode == MEMORY_MODE_POOL) {
        return mem_allocator_malloc(pool_allocator, size);
    }
    else if (memory_mode == MEMORY_MODE_ALLOCATOR) {
        return malloc_func(
#if WASM_MEM_ALLOC_WITH_USAGE != 0
            Alloc_For_Runtime,
#endif
#if WASM_MEM_ALLOC_WITH_USER_DATA != 0
            allocator_user_data,
#endif
            size);
    }
    else {
        return os_malloc(size);
    }
}

static inline void *
wasm_runtime_realloc_internal(void *ptr, unsigned int size)
{
    if (memory_mode == MEMORY_MODE_UNKNOWN) {
        LOG_WARNING(
            "wasm_runtime_realloc failed: memory hasn't been initialized.\n");
        return NULL;
    }
    else if (memory_mode == MEMORY_MODE_POOL) {
        return mem_allocator_realloc(pool_allocator, ptr, size);
    }
    else if (memory_mode == MEMORY_MODE_ALLOCATOR) {
        if (realloc_func)
            return realloc_func(
#if WASM_MEM_ALLOC_WITH_USAGE != 0
                Alloc_For_Runtime, false,
#endif
#if WASM_MEM_ALLOC_WITH_USER_DATA != 0
                allocator_user_data,
#endif
                ptr, size);
        else
            return NULL;
    }
    else {
        return os_realloc(ptr, size);
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
    else if (memory_mode == MEMORY_MODE_ALLOCATOR) {
        free_func(
#if WASM_MEM_ALLOC_WITH_USAGE != 0
            Alloc_For_Runtime,
#endif
#if WASM_MEM_ALLOC_WITH_USER_DATA != 0
            allocator_user_data,
#endif
            ptr);
    }
    else {
        os_free(ptr);
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

#if WASM_ENABLE_FUZZ_TEST != 0
    if (size >= WASM_MEM_ALLOC_MAX_SIZE) {
        LOG_WARNING("warning: wasm_runtime_malloc with too large size\n");
        return NULL;
    }
#endif

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

bool
wasm_runtime_validate_app_addr(WASMModuleInstanceCommon *module_inst_comm,
                               uint64 app_offset, uint64 size)
{
    WASMModuleInstance *module_inst = (WASMModuleInstance *)module_inst_comm;
    WASMMemoryInstance *memory_inst;
    uint64 max_linear_memory_size = MAX_LINEAR_MEMORY_SIZE;

    bh_assert(module_inst_comm->module_type == Wasm_Module_Bytecode
              || module_inst_comm->module_type == Wasm_Module_AoT);

    if (!is_bounds_checks_enabled(module_inst_comm)) {
        return true;
    }

    memory_inst = wasm_get_default_memory(module_inst);
    if (!memory_inst) {
        goto fail;
    }

#if WASM_ENABLE_MEMORY64 != 0
    if (memory_inst->is_memory64)
        max_linear_memory_size = MAX_LINEAR_MEM64_MEMORY_SIZE;
#endif
    /* boundary overflow check */
    if (size > max_linear_memory_size
        || app_offset > max_linear_memory_size - size) {
        goto shared_heap_bound_check;
    }

    SHARED_MEMORY_LOCK(memory_inst);

    if (app_offset + size <= memory_inst->memory_data_size) {
        SHARED_MEMORY_UNLOCK(memory_inst);
        return true;
    }

    SHARED_MEMORY_UNLOCK(memory_inst);

shared_heap_bound_check:
#if WASM_ENABLE_SHARED_HEAP != 0
    if (is_app_addr_in_shared_heap(module_inst_comm, memory_inst->is_memory64,
                                   app_offset, size)) {
        return true;
    }
#endif
fail:
    wasm_set_exception(module_inst, "out of bounds memory access");
    return false;
}

bool
wasm_runtime_validate_app_str_addr(WASMModuleInstanceCommon *module_inst_comm,
                                   uint64 app_str_offset)
{
    WASMModuleInstance *module_inst = (WASMModuleInstance *)module_inst_comm;
    uint64 app_end_offset, max_linear_memory_size = MAX_LINEAR_MEMORY_SIZE;
    char *str, *str_end;

    bh_assert(module_inst_comm->module_type == Wasm_Module_Bytecode
              || module_inst_comm->module_type == Wasm_Module_AoT);

    if (!is_bounds_checks_enabled(module_inst_comm)) {
        return true;
    }

    if (!wasm_runtime_get_app_addr_range(module_inst_comm, app_str_offset, NULL,
                                         &app_end_offset))
        goto fail;

#if WASM_ENABLE_MEMORY64 != 0
    if (module_inst->memories[0]->is_memory64)
        max_linear_memory_size = MAX_LINEAR_MEM64_MEMORY_SIZE;
#endif
    /* boundary overflow check, max start offset can only be size - 1, while end
     * offset can be size */
    if (app_str_offset >= max_linear_memory_size
        || app_end_offset > max_linear_memory_size)
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
                                  void *native_ptr, uint64 size)
{
    WASMModuleInstance *module_inst = (WASMModuleInstance *)module_inst_comm;
    WASMMemoryInstance *memory_inst;
    uint8 *addr = (uint8 *)native_ptr;
    uint64 max_linear_memory_size = MAX_LINEAR_MEMORY_SIZE;

    bh_assert(module_inst_comm->module_type == Wasm_Module_Bytecode
              || module_inst_comm->module_type == Wasm_Module_AoT);

    if (!is_bounds_checks_enabled(module_inst_comm)) {
        return true;
    }

    memory_inst = wasm_get_default_memory(module_inst);
    if (!memory_inst) {
        goto fail;
    }

#if WASM_ENABLE_MEMORY64 != 0
    if (memory_inst->is_memory64)
        max_linear_memory_size = MAX_LINEAR_MEM64_MEMORY_SIZE;
#endif
    /* boundary overflow check */
    if (size > max_linear_memory_size || (uintptr_t)addr > UINTPTR_MAX - size) {
        goto fail;
    }

    SHARED_MEMORY_LOCK(memory_inst);

    if (memory_inst->memory_data <= addr
        && addr + size <= memory_inst->memory_data_end) {
        SHARED_MEMORY_UNLOCK(memory_inst);
        return true;
    }

#if WASM_ENABLE_SHARED_HEAP != 0
    else if (is_native_addr_in_shared_heap(module_inst_comm, native_ptr,
                                           size)) {
        SHARED_MEMORY_UNLOCK(memory_inst);
        return true;
    }
#endif
    SHARED_MEMORY_UNLOCK(memory_inst);

fail:
    wasm_set_exception(module_inst, "out of bounds memory access");
    return false;
}

void *
wasm_runtime_addr_app_to_native(WASMModuleInstanceCommon *module_inst_comm,
                                uint64 app_offset)
{
    WASMModuleInstance *module_inst = (WASMModuleInstance *)module_inst_comm;
    WASMMemoryInstance *memory_inst;
    uint8 *addr;
    bool bounds_checks;

    bh_assert(module_inst_comm->module_type == Wasm_Module_Bytecode
              || module_inst_comm->module_type == Wasm_Module_AoT);

    bounds_checks = is_bounds_checks_enabled(module_inst_comm);

    memory_inst = wasm_get_default_memory(module_inst);
    if (!memory_inst) {
        return NULL;
    }

    SHARED_MEMORY_LOCK(memory_inst);

    addr = memory_inst->memory_data + (uintptr_t)app_offset;

    if (bounds_checks) {
        if (memory_inst->memory_data <= addr
            && addr < memory_inst->memory_data_end) {
            SHARED_MEMORY_UNLOCK(memory_inst);
            return addr;
        }
#if WASM_ENABLE_SHARED_HEAP != 0
        else if (is_app_addr_in_shared_heap(module_inst_comm,
                                            memory_inst->is_memory64,
                                            app_offset, 1)) {
            uint64 heap_start = shared_heap_get_addr_start(
                module_inst->e->shared_heap, memory_inst);
            uint64 heap_offset = (uint64)app_offset - heap_start;

            addr = module_inst->e->shared_heap->base_addr + heap_offset;
            SHARED_MEMORY_UNLOCK(memory_inst);
            return addr;
        }
#endif
        SHARED_MEMORY_UNLOCK(memory_inst);
        return NULL;
    }

    /* If bounds checks is disabled, return the address directly */
    SHARED_MEMORY_UNLOCK(memory_inst);
    return addr;
}

uint64
wasm_runtime_addr_native_to_app(WASMModuleInstanceCommon *module_inst_comm,
                                void *native_ptr)
{
    WASMModuleInstance *module_inst = (WASMModuleInstance *)module_inst_comm;
    WASMMemoryInstance *memory_inst;
    uint8 *addr = (uint8 *)native_ptr;
    bool bounds_checks;
    uint64 ret;

    bh_assert(module_inst_comm->module_type == Wasm_Module_Bytecode
              || module_inst_comm->module_type == Wasm_Module_AoT);

    bounds_checks = is_bounds_checks_enabled(module_inst_comm);

#if WASM_ENABLE_SHARED_HEAP != 0
    /* If shared heap is enabled, bounds check is always needed */
    bounds_checks = true;
#endif

    memory_inst = wasm_get_default_memory(module_inst);
    if (!memory_inst) {
        return 0;
    }

    SHARED_MEMORY_LOCK(memory_inst);

    if (bounds_checks) {
        if (memory_inst->memory_data <= addr
            && addr < memory_inst->memory_data_end) {
            ret = (uint64)(addr - memory_inst->memory_data);
            SHARED_MEMORY_UNLOCK(memory_inst);
            return ret;
        }
        else {
#if WASM_ENABLE_SHARED_HEAP != 0
            uint64 shared_heap_start = shared_heap_get_addr_start(
                module_inst->e->shared_heap, memory_inst);
            ret =
                (uint64)(addr - (uint8 *)module_inst->e->shared_heap->base_addr)
                + shared_heap_start;
            SHARED_MEMORY_UNLOCK(memory_inst);
            return ret;
#endif
        }
    }
    /* If bounds checks is disabled, return the offset directly */
    else if (addr != NULL) {
        ret = (uint64)(addr - memory_inst->memory_data);
        SHARED_MEMORY_UNLOCK(memory_inst);
        return ret;
    }

    SHARED_MEMORY_UNLOCK(memory_inst);
    return 0;
}

bool
wasm_runtime_get_app_addr_range(WASMModuleInstanceCommon *module_inst_comm,
                                uint64 app_offset, uint64 *p_app_start_offset,
                                uint64 *p_app_end_offset)
{
    WASMModuleInstance *module_inst = (WASMModuleInstance *)module_inst_comm;
    WASMMemoryInstance *memory_inst;
    uint64 memory_data_size;

    bh_assert(module_inst_comm->module_type == Wasm_Module_Bytecode
              || module_inst_comm->module_type == Wasm_Module_AoT);

    memory_inst = wasm_get_default_memory(module_inst);
    if (!memory_inst) {
        return false;
    }

    SHARED_MEMORY_LOCK(memory_inst);

    memory_data_size = memory_inst->memory_data_size;

    if (app_offset < memory_data_size) {
        if (p_app_start_offset)
            *p_app_start_offset = 0;
        if (p_app_end_offset)
            *p_app_end_offset = memory_data_size;
        SHARED_MEMORY_UNLOCK(memory_inst);
        return true;
    }

    SHARED_MEMORY_UNLOCK(memory_inst);
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

    memory_inst = wasm_get_default_memory(module_inst);
    if (!memory_inst) {
        return false;
    }

    SHARED_MEMORY_LOCK(memory_inst);

    if (memory_inst->memory_data <= addr
        && addr < memory_inst->memory_data_end) {
        if (p_native_start_addr)
            *p_native_start_addr = memory_inst->memory_data;
        if (p_native_end_addr)
            *p_native_end_addr = memory_inst->memory_data_end;
        SHARED_MEMORY_UNLOCK(memory_inst);
        return true;
    }

    SHARED_MEMORY_UNLOCK(memory_inst);
    return false;
}

bool
wasm_check_app_addr_and_convert(WASMModuleInstance *module_inst, bool is_str,
                                uint64 app_buf_addr, uint64 app_buf_size,
                                void **p_native_addr)
{
    WASMMemoryInstance *memory_inst = wasm_get_default_memory(module_inst);
    uint8 *native_addr;
    bool bounds_checks;

    bh_assert(app_buf_addr <= UINTPTR_MAX && app_buf_size <= UINTPTR_MAX);

    if (!memory_inst) {
        wasm_set_exception(module_inst, "out of bounds memory access");
        return false;
    }

    native_addr = memory_inst->memory_data + (uintptr_t)app_buf_addr;

    bounds_checks = is_bounds_checks_enabled((wasm_module_inst_t)module_inst);

    if (!bounds_checks) {
        if (app_buf_addr == 0) {
            native_addr = NULL;
        }
        goto success;
    }

    /* No need to check the app_offset and buf_size if memory access
       boundary check with hardware trap is enabled */
#ifndef OS_ENABLE_HW_BOUND_CHECK
    SHARED_MEMORY_LOCK(memory_inst);

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

    SHARED_MEMORY_UNLOCK(memory_inst);
#endif

success:
    *p_native_addr = (void *)native_addr;
    return true;

#ifndef OS_ENABLE_HW_BOUND_CHECK
fail:
    SHARED_MEMORY_UNLOCK(memory_inst);
    wasm_set_exception(module_inst, "out of bounds memory access");
    return false;
#endif
}

WASMMemoryInstance *
wasm_get_default_memory(WASMModuleInstance *module_inst)
{
    if (module_inst->memories)
        return module_inst->memories[0];
    else
        return NULL;
}

WASMMemoryInstance *
wasm_get_memory_with_idx(WASMModuleInstance *module_inst, uint32 index)
{
    bh_assert(index < module_inst->memory_count);
    if (module_inst->memories)
        return module_inst->memories[index];
    else
        return NULL;
}

void
wasm_runtime_set_mem_bound_check_bytes(WASMMemoryInstance *memory,
                                       uint64 memory_data_size)
{
#if WASM_ENABLE_FAST_JIT != 0 || WASM_ENABLE_JIT != 0 || WASM_ENABLE_AOT != 0
#if UINTPTR_MAX == UINT64_MAX
    memory->mem_bound_check_1byte.u64 = memory_data_size - 1;
    memory->mem_bound_check_2bytes.u64 = memory_data_size - 2;
    memory->mem_bound_check_4bytes.u64 = memory_data_size - 4;
    memory->mem_bound_check_8bytes.u64 = memory_data_size - 8;
    memory->mem_bound_check_16bytes.u64 = memory_data_size - 16;
#else
    memory->mem_bound_check_1byte.u32[0] = (uint32)memory_data_size - 1;
    memory->mem_bound_check_2bytes.u32[0] = (uint32)memory_data_size - 2;
    memory->mem_bound_check_4bytes.u32[0] = (uint32)memory_data_size - 4;
    memory->mem_bound_check_8bytes.u32[0] = (uint32)memory_data_size - 8;
    memory->mem_bound_check_16bytes.u32[0] = (uint32)memory_data_size - 16;
#endif
#endif
}

static void
wasm_munmap_linear_memory(void *mapped_mem, uint64 commit_size, uint64 map_size)
{
#ifdef BH_PLATFORM_WINDOWS
    os_mem_decommit(mapped_mem, commit_size);
#else
    (void)commit_size;
#endif
    os_munmap(mapped_mem, map_size);
}

static void *
wasm_mremap_linear_memory(void *mapped_mem, uint64 old_size, uint64 new_size,
                          uint64 commit_size)
{
    void *new_mem;

    bh_assert(new_size > 0);
    bh_assert(new_size > old_size);

    if (mapped_mem) {
        new_mem = os_mremap(mapped_mem, old_size, new_size);
    }
    else {
        new_mem = os_mmap(NULL, new_size, MMAP_PROT_NONE, MMAP_MAP_NONE,
                          os_get_invalid_handle());
    }
    if (!new_mem) {
        return NULL;
    }

#ifdef BH_PLATFORM_WINDOWS
    if (commit_size > 0
        && !os_mem_commit(new_mem, commit_size,
                          MMAP_PROT_READ | MMAP_PROT_WRITE)) {
        os_munmap(new_mem, new_size);
        return NULL;
    }
#endif

    if (os_mprotect(new_mem, commit_size, MMAP_PROT_READ | MMAP_PROT_WRITE)
        != 0) {
        wasm_munmap_linear_memory(new_mem, new_size, new_size);
        return NULL;
    }

    return new_mem;
}

static void *
wasm_mmap_linear_memory(uint64_t map_size, uint64 commit_size)
{
    return wasm_mremap_linear_memory(NULL, 0, map_size, commit_size);
}

bool
wasm_enlarge_memory_internal(WASMModuleInstance *module, uint32 inc_page_count,
                             uint32 memidx)
{
#if WASM_ENABLE_MULTI_MEMORY != 0
    WASMMemoryInstance *memory = wasm_get_memory_with_idx(module, memidx);
#else
    WASMMemoryInstance *memory = wasm_get_default_memory(module);
#endif

#if WASM_ENABLE_SHARED_HEAP != 0
    WASMSharedHeap *heap;
#endif

    uint8 *memory_data_old, *memory_data_new, *heap_data_old;
    uint32 num_bytes_per_page, heap_size;
    uint32 cur_page_count, max_page_count, total_page_count;
    uint64 total_size_old = 0, total_size_new;
    bool ret = true, full_size_mmaped;
    enlarge_memory_error_reason_t failure_reason = INTERNAL_ERROR;

    if (!memory) {
        ret = false;
        goto return_func;
    }

#ifdef OS_ENABLE_HW_BOUND_CHECK
    full_size_mmaped = true;
#elif WASM_ENABLE_SHARED_MEMORY != 0
    full_size_mmaped = shared_memory_is_shared(memory);
#else
    full_size_mmaped = false;
#endif

    memory_data_old = memory->memory_data;
    total_size_old = memory->memory_data_size;

    heap_data_old = memory->heap_data;
    heap_size = (uint32)(memory->heap_data_end - memory->heap_data);

    num_bytes_per_page = memory->num_bytes_per_page;
    cur_page_count = memory->cur_page_count;
    max_page_count = memory->max_page_count;
    total_page_count = inc_page_count + cur_page_count;
    total_size_new = num_bytes_per_page * (uint64)total_page_count;

#if WASM_ENABLE_SHARED_HEAP != 0
    heap = module->e->shared_heap;
    if (memory->is_memory64 && total_size_new > UINT64_MAX - heap->size + 1) {
        LOG_WARNING("Linear memory address is overlapped with shared heap");
        ret = false;
        goto return_func;
    }
    else if (!memory->is_memory64 && total_size_new > UINT32_MAX - heap->size + 1) {
        LOG_WARNING("Linear memory address is overlapped with shared heap");
        ret = false;
        goto return_func;
    }
#endif
    if (inc_page_count <= 0)
        /* No need to enlarge memory */
        return true;

    if (total_page_count < cur_page_count) { /* integer overflow */
        ret = false;
        goto return_func;
    }

    if (total_page_count > max_page_count) {
        failure_reason = MAX_SIZE_REACHED;
        ret = false;
        goto return_func;
    }

    bh_assert(total_size_new
              <= GET_MAX_LINEAR_MEMORY_SIZE(memory->is_memory64));

#if WASM_MEM_ALLOC_WITH_USAGE != 0
    if (!(memory_data_new =
              realloc_func(Alloc_For_LinearMemory, full_size_mmaped,
#if WASM_MEM_ALLOC_WITH_USER_DATA != 0
                           NULL,
#endif
                           memory_data_old, total_size_new))) {
        ret = false;
        goto return_func;
    }
    if (heap_size > 0) {
        if (mem_allocator_migrate(memory->heap_handle,
                                  (char *)heap_data_old
                                      + (memory_data_new - memory_data_old),
                                  heap_size)
            != 0) {
            ret = false;
        }
    }
    memory->heap_data = memory_data_new + (heap_data_old - memory_data_old);
    memory->heap_data_end = memory->heap_data + heap_size;
    memory->memory_data = memory_data_new;
#else
    if (full_size_mmaped) {
#ifdef BH_PLATFORM_WINDOWS
        if (!os_mem_commit(memory->memory_data_end,
                           (mem_offset_t)(total_size_new - total_size_old),
                           MMAP_PROT_READ | MMAP_PROT_WRITE)) {
            ret = false;
            goto return_func;
        }
#endif

        if (os_mprotect(memory->memory_data_end,
                        (mem_offset_t)(total_size_new - total_size_old),
                        MMAP_PROT_READ | MMAP_PROT_WRITE)
            != 0) {
#ifdef BH_PLATFORM_WINDOWS
            os_mem_decommit(memory->memory_data_end,
                            (mem_offset_t)(total_size_new - total_size_old));
#endif
            ret = false;
            goto return_func;
        }
    }
    else {
        if (heap_size > 0) {
            if (mem_allocator_is_heap_corrupted(memory->heap_handle)) {
                wasm_runtime_show_app_heap_corrupted_prompt();
                ret = false;
                goto return_func;
            }
        }

        if (!(memory_data_new =
                  wasm_mremap_linear_memory(memory_data_old, total_size_old,
                                            total_size_new, total_size_new))) {
            ret = false;
            goto return_func;
        }

        if (heap_size > 0) {
            if (mem_allocator_migrate(memory->heap_handle,
                                      (char *)heap_data_old
                                          + (memory_data_new - memory_data_old),
                                      heap_size)
                != 0) {
                /* Don't return here as memory->memory_data is obsolete and
                must be updated to be correctly used later. */
                ret = false;
            }
        }

        memory->heap_data = memory_data_new + (heap_data_old - memory_data_old);
        memory->heap_data_end = memory->heap_data + heap_size;
        memory->memory_data = memory_data_new;
#if defined(os_writegsbase)
        /* write base addr of linear memory to GS segment register */
        os_writegsbase(memory_data_new);
#endif
    }
#endif /* end of WASM_MEM_ALLOC_WITH_USAGE */

    /*
     * AOT compiler assumes at least 8 byte alignment.
     * see aot_check_memory_overflow.
     */
    bh_assert(((uintptr_t)memory->memory_data & 0x7) == 0);

    memory->num_bytes_per_page = num_bytes_per_page;
    memory->cur_page_count = total_page_count;
    memory->max_page_count = max_page_count;
    SET_LINEAR_MEMORY_SIZE(memory, total_size_new);
    memory->memory_data_end = memory->memory_data + total_size_new;

    wasm_runtime_set_mem_bound_check_bytes(memory, total_size_new);

return_func:
    if (!ret && enlarge_memory_error_cb) {
        WASMExecEnv *exec_env = NULL;

#if WASM_ENABLE_INTERP != 0
        if (module->module_type == Wasm_Module_Bytecode)
            exec_env = ((WASMModuleInstance *)module)->cur_exec_env;
#endif
#if WASM_ENABLE_AOT != 0
        if (module->module_type == Wasm_Module_AoT)
            exec_env = ((AOTModuleInstance *)module)->cur_exec_env;
#endif

        enlarge_memory_error_cb(inc_page_count, total_size_old, 0,
                                failure_reason,
                                (WASMModuleInstanceCommon *)module, exec_env,
                                enlarge_memory_error_user_data);
    }

    return ret;
}

bool
wasm_runtime_enlarge_memory(WASMModuleInstanceCommon *module_inst,
                            uint64 inc_page_count)
{
    if (inc_page_count > UINT32_MAX) {
        return false;
    }

#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT) {
        return aot_enlarge_memory((AOTModuleInstance *)module_inst,
                                  (uint32)inc_page_count);
    }
#endif
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        return wasm_enlarge_memory((WASMModuleInstance *)module_inst,
                                   (uint32)inc_page_count);
    }
#endif

    return false;
}

void
wasm_runtime_set_enlarge_mem_error_callback(
    const enlarge_memory_error_callback_t callback, void *user_data)
{
    enlarge_memory_error_cb = callback;
    enlarge_memory_error_user_data = user_data;
}

bool
wasm_enlarge_memory(WASMModuleInstance *module, uint32 inc_page_count)
{
    bool ret = false;

#if WASM_ENABLE_SHARED_MEMORY != 0
    if (module->memory_count > 0)
        shared_memory_lock(module->memories[0]);
#endif
    ret = wasm_enlarge_memory_internal(module, inc_page_count, 0);
#if WASM_ENABLE_SHARED_MEMORY != 0
    if (module->memory_count > 0)
        shared_memory_unlock(module->memories[0]);
#endif

    return ret;
}

bool
wasm_enlarge_memory_with_idx(WASMModuleInstance *module, uint32 inc_page_count,
                             uint32 memidx)
{
    bool ret = false;

#if WASM_ENABLE_SHARED_MEMORY != 0
    if (memidx < module->memory_count)
        shared_memory_lock(module->memories[memidx]);
#endif
    ret = wasm_enlarge_memory_internal(module, inc_page_count, memidx);
#if WASM_ENABLE_SHARED_MEMORY != 0
    if (memidx < module->memory_count)
        shared_memory_unlock(module->memories[memidx]);
#endif

    return ret;
}

void
wasm_deallocate_linear_memory(WASMMemoryInstance *memory_inst)
{
    uint64 map_size;

    bh_assert(memory_inst);
    bh_assert(memory_inst->memory_data);

#ifndef OS_ENABLE_HW_BOUND_CHECK
#if WASM_ENABLE_SHARED_MEMORY != 0
    if (shared_memory_is_shared(memory_inst)) {
        map_size = (uint64)memory_inst->num_bytes_per_page
                   * memory_inst->max_page_count;
    }
    else
#endif
    {
        map_size = (uint64)memory_inst->num_bytes_per_page
                   * memory_inst->cur_page_count;
    }
#else
    map_size = 8 * (uint64)BH_GB;
#endif

#if WASM_MEM_ALLOC_WITH_USAGE != 0
    (void)map_size;
    free_func(Alloc_For_LinearMemory,
#if WASM_MEM_ALLOC_WITH_USER_DATA != 0
              NULL,
#endif
              memory_inst->memory_data);
#else
    wasm_munmap_linear_memory(memory_inst->memory_data,
                              memory_inst->memory_data_size, map_size);
#endif

    memory_inst->memory_data = NULL;
}

int
wasm_allocate_linear_memory(uint8 **data, bool is_shared_memory,
                            bool is_memory64, uint64 num_bytes_per_page,
                            uint64 init_page_count, uint64 max_page_count,
                            uint64 *memory_data_size)
{
    uint64 map_size, page_size;

    bh_assert(data);
    bh_assert(memory_data_size);

#ifndef OS_ENABLE_HW_BOUND_CHECK
#if WASM_ENABLE_SHARED_MEMORY != 0
    if (is_shared_memory) {
        /* Allocate maximum memory size when memory is shared */
        map_size = max_page_count * num_bytes_per_page;
    }
    else
#endif
    {
        map_size = init_page_count * num_bytes_per_page;
    }
#else  /* else of OS_ENABLE_HW_BOUND_CHECK */
    /* Totally 8G is mapped, the opcode load/store address range is 0 to 8G:
     *   ea = i + memarg.offset
     * both i and memarg.offset are u32 in range 0 to 4G
     * so the range of ea is 0 to 8G
     */
    map_size = 8 * (uint64)BH_GB;
#endif /* end of OS_ENABLE_HW_BOUND_CHECK */

    page_size = os_getpagesize();
    *memory_data_size = init_page_count * num_bytes_per_page;

    bh_assert(*memory_data_size <= GET_MAX_LINEAR_MEMORY_SIZE(is_memory64));
    *memory_data_size = align_as_and_cast(*memory_data_size, page_size);

    if (map_size > 0) {
#if WASM_MEM_ALLOC_WITH_USAGE != 0
        (void)wasm_mmap_linear_memory;
        if (!(*data = malloc_func(Alloc_For_LinearMemory,
#if WASM_MEM_ALLOC_WITH_USER_DATA != 0
                                  NULL,
#endif
                                  *memory_data_size))) {
            return BHT_ERROR;
        }
#else
        if (!(*data = wasm_mmap_linear_memory(map_size, *memory_data_size))) {
            return BHT_ERROR;
        }
#endif
    }

    /*
     * AOT compiler assumes at least 8 byte alignment.
     * see aot_check_memory_overflow.
     */
    bh_assert(((uintptr_t)*data & 0x7) == 0);

    return BHT_OK;
}
