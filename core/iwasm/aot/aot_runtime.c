/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_runtime.h"
#include "bh_log.h"
#include "mem_alloc.h"

static void
set_error_buf(char *error_buf, uint32 error_buf_size, const char *string)
{
    if (error_buf != NULL)
        snprintf(error_buf, error_buf_size, "%s", string);
}

static bool
global_instantiate(AOTModuleInstance *module_inst, AOTModule *module,
                   char *error_buf, uint32 error_buf_size)
{
    uint32 i;
    InitializerExpression *init_expr;
    uint8 *p = (uint8*)module_inst->global_data.ptr;
    AOTImportGlobal *import_global = module->import_globals;;
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
                    set_error_buf(error_buf, error_buf_size,
                                  "Instantiate global failed: unknown global.");
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

    if (module->table_init_data_count > 0) {
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
            length = table_seg->func_index_count;
            if (base_offset < module_inst->table_size) {
                memcpy((uint32*)module_inst->table_data.ptr + base_offset,
                       table_seg->func_indexes, length * sizeof(uint32));
            }
        }
    }

    return true;
}

static bool
memory_instantiate(AOTModuleInstance *module_inst, AOTModule *module,
                   uint32 heap_size, char *error_buf, uint32 error_buf_size)
{
    uint32 i, global_index, global_data_offset, base_offset, length;
    AOTMemInitData *data_seg;
    void *heap_handle;
    uint64 memory_data_size = (uint64)module->num_bytes_per_page
                              * module->mem_init_page_count;
    uint64 total_size = heap_size + memory_data_size;
    uint8 *p;

    /* Allocate memory */
    if (total_size >= UINT32_MAX
        || !(p = wasm_runtime_malloc((uint32)total_size))) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module instantiate failed: allocate memory failed.");
        return false;
    }

    memset(p, 0, (uint32)total_size);

    /* Initialize heap info */
    module_inst->heap_data.ptr = p;
    p += heap_size;
    module_inst->heap_data_end.ptr = p;
    if (!(heap_handle = mem_allocator_create(module_inst->heap_data.ptr,
                                             heap_size))) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module instantiate failed: init app heap failed.");
        goto fail1;
    }
    module_inst->heap_handle.ptr = heap_handle;
    module_inst->heap_data_size = heap_size;
#if WASM_ENABLE_SPEC_TEST == 0
    module_inst->heap_base_offset = -(int32)heap_size;
#else
    module_inst->heap_base_offset = 0;
#endif

    /* Init memory info */
    module_inst->memory_data.ptr = p;
    p += (uint32)memory_data_size;
    module_inst->memory_data_end.ptr = p;
    module_inst->memory_data_size = (uint32)memory_data_size;
#if WASM_ENABLE_SPEC_TEST == 0
    module_inst->total_mem_size = (uint32)(heap_size + memory_data_size);
#else
    module_inst->total_mem_size = (uint32)memory_data_size;
#endif
    module_inst->mem_cur_page_count = module->mem_init_page_count;
    module_inst->mem_max_page_count = module->mem_max_page_count;

    if (module_inst->total_mem_size > 0) {
        module_inst->mem_bound_check_1byte = module_inst->total_mem_size - 1;
        module_inst->mem_bound_check_2bytes = module_inst->total_mem_size - 2;
        module_inst->mem_bound_check_4bytes = module_inst->total_mem_size - 4;
        module_inst->mem_bound_check_8bytes = module_inst->total_mem_size - 8;
    }

    if (module->mem_init_page_count > 0) {
        for (i = 0; i < module->mem_init_data_count; i++) {
            data_seg = module->mem_init_data_list[i];
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
            }
            else
                base_offset = (uint32)data_seg->offset.u.i32;

            length = data_seg->byte_count;

            /* Check memory data */
            if (length > 0
                && (base_offset >= module_inst->memory_data_size
                    || base_offset + length > module_inst->memory_data_size)) {
                set_error_buf(error_buf, error_buf_size,
                             "AOT module instantiate failed: data segment out of range.");
                goto fail2;
            }

            /* Copy memory data */
            memcpy((uint8*)module_inst->memory_data.ptr + base_offset,
                   data_seg->bytes, length);
        }
    }

    return true;

fail2:
    mem_allocator_destroy(module_inst->heap_handle.ptr);
    module_inst->heap_handle.ptr = NULL;
fail1:
    wasm_runtime_free(module_inst->heap_data.ptr);
    module_inst->heap_data.ptr = NULL;
    return false;
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
    if (total_size >= UINT32_MAX
        || !(module_inst->func_ptrs.ptr = wasm_runtime_malloc((uint32)total_size))) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module instantiate failed: allocate memory failed.");
        return false;
    }

    memset(module_inst->func_ptrs.ptr, 0, (uint32)total_size);

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
    if (total_size >= UINT32_MAX
        || !(module_inst->func_type_indexes.ptr =
                                wasm_runtime_malloc((uint32)total_size))) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module instantiate failed: allocate memory failed.");
        return false;
    }

    memset(module_inst->func_type_indexes.ptr, 0, (uint32)total_size);

    /* Set import function type indexes */
    func_type_index = (uint32*)module_inst->func_type_indexes.ptr;
    for (i = 0; i < module->import_func_count; i++, func_type_index++)
        *func_type_index = module->import_funcs[i].func_type_index;

    memcpy(func_type_index, module->func_type_indexes,
           module->func_count * sizeof(uint32));

    return true;
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
        aot_set_exception(module_inst, "allocate memory failed.");
        return false;
    }

    u.v = module->start_function;
    u.f(exec_env);

    wasm_exec_env_destroy(exec_env);
    return !aot_get_exception(module_inst);
}

AOTModuleInstance*
aot_instantiate(AOTModule *module,
                uint32 stack_size, uint32 heap_size,
                char *error_buf, uint32 error_buf_size)
{
    AOTModuleInstance *module_inst;
    uint32 module_inst_struct_size =
        offsetof(AOTModuleInstance, global_table_data.bytes);
    uint64 table_data_size = (uint64)module->table_size * sizeof(uint32);
    uint64 total_size = (uint64)module_inst_struct_size
                        + module->global_data_size
                        + table_data_size;
    uint8 *p;

    /* Check heap size */
    heap_size = align_uint(heap_size, 8);
    if (heap_size == 0)
        heap_size = APP_HEAP_SIZE_DEFAULT;
    if (heap_size < APP_HEAP_SIZE_MIN)
        heap_size = APP_HEAP_SIZE_MIN;
    if (heap_size > APP_HEAP_SIZE_MAX)
        heap_size = APP_HEAP_SIZE_MAX;

    /* Allocate module instance, global data, table data and heap data */
    if (total_size >= UINT32_MAX
        || !(module_inst = wasm_runtime_malloc((uint32)total_size))) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module instantiate failed: allocate memory failed.");
        return NULL;
    }

    memset(module_inst, 0, total_size);
    module_inst->module_type = Wasm_Module_AoT;
    module_inst->aot_module.ptr = module;

    /* Initialize global info */
    p = (uint8*)module_inst + module_inst_struct_size;
    module_inst->global_data.ptr = p;
    module_inst->global_data_size = module->global_data_size;
    if (!global_instantiate(module_inst, module, error_buf, error_buf_size))
        goto fail;

    /* Initialize table info */
    p += module->global_data_size;
    module_inst->table_data.ptr = p;
    module_inst->table_size = module->table_size;
    /* Set all elements to -1 to mark them as uninitialized elements */
    memset(module_inst->table_data.ptr, -1, (uint32)table_data_size);
    if (!table_instantiate(module_inst, module, error_buf, error_buf_size))
        goto fail;

    /* Initialize memory space */
    if (!memory_instantiate(module_inst, module, heap_size,
                            error_buf, error_buf_size))
        goto fail;

    /* Initialize function pointers */
    if (!init_func_ptrs(module_inst, module, error_buf, error_buf_size))
        goto fail;

    /* Initialize function type indexes */
    if (!init_func_type_indexes(module_inst, module, error_buf, error_buf_size))
        goto fail;

#if WASM_ENABLE_LIBC_WASI != 0
    if (!wasm_runtime_init_wasi((WASMModuleInstanceCommon*)module_inst,
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

    return module_inst;

fail:
    aot_deinstantiate(module_inst);
    return NULL;
}

void
aot_deinstantiate(AOTModuleInstance *module_inst)
{
#if WASM_ENABLE_LIBC_WASI != 0
    /* Destroy wasi resource before freeing app heap, since some fields of
       wasi contex are allocated from app heap, and if app heap is freed,
       these fields will be set to NULL, we cannot free their internal data
       which may allocated from global heap. */
    wasm_runtime_destroy_wasi((WASMModuleInstanceCommon*)module_inst);
#endif

    if (module_inst->heap_handle.ptr)
        mem_allocator_destroy(module_inst->heap_handle.ptr);

    if (module_inst->heap_data.ptr)
        wasm_runtime_free(module_inst->heap_data.ptr);

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
    AOTModule *module = (AOTModule*)module_inst->aot_module.ptr;

    for (i = 0; i < module->export_func_count; i++)
        if (!strcmp(module->export_funcs[i].func_name, name))
            return &module->export_funcs[i];
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

bool
aot_call_function(WASMExecEnv *exec_env,
                  AOTFunctionInstance *function,
                  unsigned argc, uint32 argv[])
{
    AOTModuleInstance *module_inst = (AOTModuleInstance*)exec_env->module_inst;
    AOTFuncType *func_type = function->func_type;
    bool ret = wasm_runtime_invoke_native(exec_env, function->func_ptr,
                                          func_type, NULL, NULL, argv, argc, argv);
    return ret && !aot_get_exception(module_inst) ? true : false;
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
        aot_set_exception(module_inst, "allocate memory failed.");
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

int32
aot_module_malloc(AOTModuleInstance *module_inst, uint32 size,
                  void **p_native_addr)
{
    uint8 *addr = mem_allocator_malloc(module_inst->heap_handle.ptr, size);
    if (!addr) {
        aot_set_exception(module_inst, "out of memory");
        return 0;
    }
    if (p_native_addr)
        *p_native_addr = addr;
    return (int32)(addr - (uint8*)module_inst->memory_data.ptr);
}

void
aot_module_free(AOTModuleInstance *module_inst, int32 ptr)
{
    if (ptr) {
        uint8 *addr = (uint8*)module_inst->memory_data.ptr + ptr;
        if ((uint8*)module_inst->heap_data.ptr < addr
            && addr < (uint8*)module_inst->memory_data.ptr)
            mem_allocator_free(module_inst->heap_handle.ptr, addr);
    }
}

int32
aot_module_dup_data(AOTModuleInstance *module_inst,
                    const char *src, uint32 size)
{
    char *buffer;
    int32 buffer_offset = aot_module_malloc(module_inst, size,
                                            (void**)&buffer);

    if (buffer_offset != 0) {
        buffer = aot_addr_app_to_native(module_inst, buffer_offset);
        memcpy(buffer, src, size);
    }
    return buffer_offset;
}

bool
aot_validate_app_addr(AOTModuleInstance *module_inst,
                      int32 app_offset, uint32 size)
{
    /* integer overflow check */
    if(app_offset + (int32)size < app_offset) {
        goto fail;
    }

    if (app_offset <= module_inst->heap_base_offset
        || app_offset + (int32)size > (int32)module_inst->memory_data_size) {
        goto fail;
    }
    return true;
fail:
    aot_set_exception(module_inst, "out of bounds memory access");
    return false;
}

bool
aot_validate_native_addr(AOTModuleInstance *module_inst,
                         void *native_ptr, uint32 size)
{
    uint8 *addr = (uint8*)native_ptr;
    int32 memory_data_size = (int32)module_inst->memory_data_size;

    /* integer overflow check */
    if (addr + size < addr) {
        goto fail;
    }

    if (addr <= (uint8*)module_inst->heap_data.ptr
        || addr + size > (uint8*)module_inst->memory_data.ptr
                         + memory_data_size) {
        goto fail;
    }
    return true;
fail:
    aot_set_exception(module_inst, "out of bounds memory access");
    return false;
}

void *
aot_addr_app_to_native(AOTModuleInstance *module_inst, int32 app_offset)
{
    int32 memory_data_size = (int32)module_inst->memory_data_size;
    uint8 *addr = (uint8 *)module_inst->memory_data.ptr + app_offset;

    if ((uint8*)module_inst->heap_data.ptr < addr
        && addr < (uint8*)module_inst->memory_data.ptr
                  + memory_data_size)
        return addr;
    return NULL;
}

int32
aot_addr_native_to_app(AOTModuleInstance *module_inst, void *native_ptr)
{
    uint8 *addr = (uint8*)native_ptr;
    int32 memory_data_size = (int32)module_inst->memory_data_size;

    if ((uint8*)module_inst->heap_data.ptr < addr
        && addr < (uint8*)module_inst->memory_data.ptr
                  + memory_data_size)
        return (int32)(addr - (uint8*)module_inst->memory_data.ptr);
    return 0;
}

bool
aot_get_app_addr_range(AOTModuleInstance *module_inst,
                       int32 app_offset,
                       int32 *p_app_start_offset,
                       int32 *p_app_end_offset)
{
    int32 memory_data_size = (int32)module_inst->memory_data_size;

    if (module_inst->heap_base_offset < app_offset
        && app_offset < memory_data_size) {
        if (p_app_start_offset)
            *p_app_start_offset = module_inst->heap_base_offset;
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
    uint8 *addr = (uint8*)native_ptr;
    int32 memory_data_size = (int32)module_inst->memory_data_size;

    if ((uint8*)module_inst->heap_data.ptr < addr
        && addr < (uint8*)module_inst->memory_data.ptr
                  + memory_data_size) {
        if (p_native_start_addr)
            *p_native_start_addr = (uint8*)module_inst->heap_data.ptr;
        if (p_native_end_addr)
            *p_native_end_addr = (uint8*)module_inst->memory_data.ptr
                                 + memory_data_size;
        return true;
    }
    return false;
}

bool
aot_enlarge_memory(AOTModuleInstance *module_inst, uint32 inc_page_count)
{
    uint8 *heap_data_old = module_inst->heap_data.ptr, *heap_data;
    uint32 num_bytes_per_page =
        ((AOTModule*)module_inst->aot_module.ptr)->num_bytes_per_page;
    uint32 cur_page_count = module_inst->mem_cur_page_count;
    uint32 max_page_count = module_inst->mem_max_page_count;
    uint32 total_page_count = cur_page_count + inc_page_count;
    uint64 memory_data_size = (uint64)num_bytes_per_page * total_page_count;
    uint32 heap_size = (uint32)((uint8*)module_inst->memory_data.ptr
                                - (uint8*)module_inst->heap_data.ptr);
    uint32 total_size_old = heap_size + module_inst->memory_data_size;
    uint64 total_size = heap_size + memory_data_size;
    void *heap_handle_old = module_inst->heap_handle.ptr;

    if (inc_page_count <= 0)
        /* No need to enlarge memory */
        return true;

    if (total_page_count < cur_page_count /* integer overflow */
        || total_page_count > max_page_count) {
        aot_set_exception(module_inst, "fail to enlarge memory.");
        return false;
    }

    if (total_size >= UINT32_MAX) {
        aot_set_exception(module_inst, "fail to enlarge memory.");
        return false;
    }

    /* Destroy heap's lock firstly, if its memory is re-allocated,
       we cannot access its lock again. */
    mem_allocator_destroy_lock(module_inst->heap_handle.ptr);
    if (!(heap_data = wasm_runtime_realloc(heap_handle_old, (uint32)total_size))) {
        if (!(heap_data = wasm_runtime_malloc((uint32)total_size))) {
            /* Restore heap's lock if memory re-alloc failed */
            mem_allocator_reinit_lock(module_inst->heap_handle.ptr);
            aot_set_exception(module_inst, "fail to enlarge memory.");
            return false;
        }
        bh_memcpy_s(heap_data, (uint32)total_size,
                    heap_data_old, total_size_old);
        wasm_runtime_free(heap_data_old);
    }

    memset(heap_data + total_size_old,
           0, (uint32)total_size - total_size_old);

    module_inst->heap_data.ptr = heap_data;
    module_inst->heap_handle.ptr = (uint8*)heap_handle_old
                                   + (heap_data - heap_data_old);
    module_inst->heap_data_end.ptr = heap_data + heap_size;

    if (mem_allocator_migrate(module_inst->heap_handle.ptr,
                              heap_handle_old) != 0) {
        aot_set_exception(module_inst, "fail to enlarge memory.");
        return false;
    }

    module_inst->mem_cur_page_count = total_page_count;
    module_inst->memory_data_size = (uint32)memory_data_size;
#if WASM_ENABLE_SPEC_TEST == 0
    module_inst->total_mem_size = (uint32)(heap_size + memory_data_size);
#else
    module_inst->total_mem_size = (uint32)memory_data_size;
#endif
    module_inst->memory_data.ptr = (uint8*)heap_data + heap_size;
    module_inst->memory_data_end.ptr = (uint8*)module_inst->memory_data.ptr
                                       + (uint32)memory_data_size;

    module_inst->mem_bound_check_1byte = module_inst->total_mem_size - 1;
    module_inst->mem_bound_check_2bytes = module_inst->total_mem_size - 2;
    module_inst->mem_bound_check_4bytes = module_inst->total_mem_size - 4;
    module_inst->mem_bound_check_8bytes = module_inst->total_mem_size - 8;
    return true;
}

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
    AOTImportFunc *import_func;
    const char *signature = NULL;
    void *attachment = NULL;
    char buf[128];

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

    /* this function is called from native code, so exec_env->handle and
       exec_env->native_stack_boundary must have been set, we don't set
       it again */

    if ((uint8*)&module_inst < exec_env->native_stack_boundary) {
        aot_set_exception_with_id(module_inst, EXCE_NATIVE_STACK_OVERFLOW);
        return false;
    }

    return wasm_runtime_invoke_native(exec_env, func_ptr,
                                      func_type, signature, attachment,
                                      argv, argc, argv);
}
