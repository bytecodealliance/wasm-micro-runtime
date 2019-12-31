/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot_runtime.h"
#include "wasm_memory.h"
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
        wasm_assert(import_global->data_offset ==
                    p - (uint8*)module_inst->global_data.ptr);
        memcpy(p, &import_global->global_data_linked, import_global->size);
        p += import_global->size;
    }

    /* Initialize defined global data */
    for (i = 0; i < module->global_count; i++, global++) {
        wasm_assert(global->data_offset ==
                    p - (uint8*)module_inst->global_data.ptr);
        init_expr = &global->init_expr;
        switch (init_expr->init_expr_type) {
            case INIT_EXPR_TYPE_GET_GLOBAL:
                wasm_assert(init_expr->u.global_index < module->import_global_count);
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

    wasm_assert(module_inst->global_data_size == p - (uint8*)module_inst->global_data.ptr);
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
            wasm_assert(table_seg->offset.init_expr_type ==
                            INIT_EXPR_TYPE_I32_CONST
                        || table_seg->offset.init_expr_type ==
                            INIT_EXPR_TYPE_GET_GLOBAL);

            /* Resolve table data base offset */
            if (table_seg->offset.init_expr_type == INIT_EXPR_TYPE_GET_GLOBAL) {
                global_index = table_seg->offset.u.global_index;
                wasm_assert(global_index <
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
                   char *error_buf, uint32 error_buf_size)
{
    uint32 i, global_index, global_data_offset, base_offset, length;
    AOTMemInitData *data_seg;
    uint64 total_size = (uint64)NumBytesPerPage * module->mem_init_page_count;

    /* Allocate memory */
    if (total_size >= UINT32_MAX
        || !(module_inst->memory_data.ptr = wasm_malloc((uint32)total_size))) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module instantiate failed: allocate memory failed.");
        return false;
    }

    memset(module_inst->memory_data.ptr, 0, (uint32)total_size);

    /* Init memory info */
    module_inst->memory_data_end.ptr = (uint8*)module_inst->memory_data.ptr
                                       + total_size;
    module_inst->memory_data_size = (uint32)total_size;
    module_inst->mem_cur_page_count = module->mem_init_page_count;
    module_inst->mem_max_page_count = module->mem_max_page_count;

    if (module->mem_init_page_count > 0) {
        for (i = 0; i < module->mem_init_data_count; i++) {
            data_seg = module->mem_init_data_list[i];
            wasm_assert(data_seg->offset.init_expr_type ==
                            INIT_EXPR_TYPE_I32_CONST
                        || data_seg->offset.init_expr_type ==
                            INIT_EXPR_TYPE_GET_GLOBAL);

            /* Resolve memory data base offset */
            if (data_seg->offset.init_expr_type == INIT_EXPR_TYPE_GET_GLOBAL) {
                global_index = data_seg->offset.u.global_index;
                wasm_assert(global_index <
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
                wasm_free(module_inst->memory_data.ptr);
                module_inst->memory_data.ptr = NULL;
                set_error_buf(error_buf, error_buf_size,
                             "AOT module instantiate failed: data segment out of range.");
                return false;
            }

            /* Copy memory data */
            memcpy((uint8*)module_inst->memory_data.ptr + base_offset,
                   data_seg->bytes, length);
        }
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
    if (total_size >= UINT32_MAX
        || !(module_inst->func_ptrs.ptr = wasm_malloc((uint32)total_size))) {
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
        || !(module_inst->func_type_indexes.ptr = wasm_malloc((uint32)total_size))) {
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
init_main_tlr(AOTModuleInstance *module_inst, AOTModule *module,
              char *error_buf, uint32 error_buf_size)
{
    WASMThread *main_tlr;

    /* Allocate memory */
    if (!(main_tlr = wasm_malloc(sizeof(WASMThread)))) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module instantiate failed: allocate memory failed.");
        return false;
    }

    /* Set thread info */
    memset(main_tlr, 0, sizeof(WASMThread));
    module_inst->main_tlr.ptr = main_tlr;
    main_tlr->module_inst = (WASMModuleInstance*)module_inst;

    /* Bind thread data with current native thread:
       set thread local root to current thread. */
    wasm_runtime_set_tlr(main_tlr);
    main_tlr->handle = ws_self_thread();

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
    return aot_call_wasm(module_inst, post_inst_func, 0, NULL);
}

static bool
execute_start_function(AOTModuleInstance *module_inst)
{
    typedef void (*F)(AOTModuleInstance*);
    union { F f; void *v; } u;
    AOTModule *module = (AOTModule*)module_inst->aot_module.ptr;

    if (!module->start_function)
        return true;

    u.v = module->start_function;
    u.f(module_inst);
    return !aot_get_exception(module_inst);
}

AOTModuleInstance*
aot_instantiate(AOTModule *module, uint32 heap_size,
                char *error_buf, uint32 error_buf_size)
{
    AOTModuleInstance *module_inst;
    uint32 module_inst_struct_size =
        offsetof(AOTModuleInstance, global_table_heap_data.bytes);
    uint64 table_data_size = (uint64)module->table_size * sizeof(uint32);
    uint64 total_size = (uint64)module_inst_struct_size
                        + module->global_data_size
                        + table_data_size + heap_size;
    void *heap_handle;
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
        || !(module_inst = wasm_malloc((uint32)total_size))) {
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

    /* Initialize heap info */
    p += (uint32)table_data_size;
    module_inst->heap_data.ptr = p;
    p += heap_size;
    module_inst->heap_data_end.ptr = p;
    module_inst->heap_data_size = heap_size;
#if WASM_ENABLE_MEMORY_GROW != 0
    module_inst->heap_base_offset = DEFAULT_APP_HEAP_BASE_OFFSET;
#else
    module_inst->heap_base_offset = module_inst->memory_data_size;
#endif
    if (!(heap_handle = mem_allocator_create(module_inst->heap_data.ptr,
                                             heap_size))) {
        set_error_buf(error_buf, error_buf_size,
                      "AOT module instantiate failed: init app heap failed.");
        goto fail;
    }
    module_inst->heap_handle.ptr = heap_handle;

    /* Initialize memory space */
    if (!memory_instantiate(module_inst, module, error_buf, error_buf_size))
        goto fail;

    /* Initialize function pointers */
    if (!init_func_ptrs(module_inst, module, error_buf, error_buf_size))
        goto fail;

    /* Initialize function type indexes */
    if (!init_func_type_indexes(module_inst, module, error_buf, error_buf_size))
        goto fail;

    /* Initialize main thread */
    if (!init_main_tlr(module_inst, module, error_buf, error_buf_size))
        goto fail;

#if WASM_ENABLE_WASI != 0
    if (!wasm_runtime_init_wasi((WASMModuleInstance*)module_inst,
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
#if WASM_ENABLE_WASI != 0
    /* Destroy wasi resource before freeing app heap, since some fields of
       wasi contex are allocated from app heap, and if app heap is freed,
       these fields will be set to NULL, we cannot free their internal data
       which may allocated from global heap. */
    wasm_runtime_destroy_wasi((WASMModuleInstance*)module_inst);
#endif

    if (module_inst->memory_data.ptr)
        wasm_free(module_inst->memory_data.ptr);

    if (module_inst->heap_handle.ptr)
        mem_allocator_destroy(module_inst->heap_handle.ptr);

    if (module_inst->func_ptrs.ptr)
        wasm_free(module_inst->func_ptrs.ptr);

    if (module_inst->func_type_indexes.ptr)
        wasm_free(module_inst->func_type_indexes.ptr);

    if (module_inst->main_tlr.ptr)
        wasm_free(module_inst->main_tlr.ptr);

    wasm_free(module_inst);
}

static bool
check_type(uint8 type, const char *p)
{
    const char *str = "i32";

    if (strlen(p) < 3)
        return false;

    switch (type) {
        case VALUE_TYPE_I32:
            str = "i32";
            break;
        case VALUE_TYPE_I64:
            str = "i64";
            break;
        case VALUE_TYPE_F32:
            str = "f32";
            break;
        case VALUE_TYPE_F64:
            str = "f64";
            break;
    }
    if (strncmp(p, str, 3))
        return false;

    return true;
}

static bool
check_function_type(const WASMType *type,
                    const char *signature)
{
    uint32 i;
    const char *p = signature;

    if (!p || *p++ != '(')
        return false;

    for (i = 0; i < type->param_count; i++) {
        if (!check_type(type->types[i], p))
            return false;
        p += 3;
    }

    if (*p++ != ')')
        return false;

    if (type->result_count) {
        if (!check_type(type->types[type->param_count], p))
            return false;
        p += 3;
    }

    if (*p != '\0')
        return false;

    return true;
}

AOTFunctionInstance*
aot_lookup_function(const AOTModuleInstance *module_inst,
                    const char *name, const char *signature)
{
    uint32 i;
    AOTModule *module = (AOTModule*)module_inst->aot_module.ptr;

    for (i = 0; i < module->export_func_count; i++) {
        if (!strcmp(module->export_funcs[i].func_name, name)
            && check_function_type(module->export_funcs[i].func_type,
                                   signature))
            return &module->export_funcs[i];
    }

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
aot_call_wasm(AOTModuleInstance *module_inst,
              AOTFunctionInstance *function,
              unsigned argc, uint32 argv[])
{
    AOTFuncType *func_type = function->func_type;
    if (!wasm_runtime_invoke_native(function->func_ptr, func_type,
                                   (WASMModuleInstance*)module_inst,
                                    argv, argc, argv))
        return false;
    return !aot_get_exception(module_inst) ? true : false;
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

#if WASM_ENABLE_EXT_MEMORY_SPACE != 0
bool
aot_set_ext_memory(AOTModuleInstance *module_inst,
                   uint8 *ext_mem_data, uint32 ext_mem_size,
                   char *error_buf, uint32 error_buf_size)
{
    if (module_inst->ext_mem_data.ptr) {
        set_error_buf(error_buf, error_buf_size,
                      "Set external memory failed: "
                      "an external memory has been set.");
        return false;
    }

    if (!ext_mem_data
        || ext_mem_size > 1 * BH_GB
        || ext_mem_data + ext_mem_size < ext_mem_data) {
        set_error_buf(error_buf, error_buf_size,
                      "Set external memory failed: "
                      "invalid input.");
        return false;
    }

    module_inst->ext_mem_data.ptr = ext_mem_data;
    module_inst->ext_mem_data_end.ptr = ext_mem_data + ext_mem_size;
    module_inst->ext_mem_data_size = ext_mem_size;
    module_inst->ext_mem_base_offset = DEFAULT_EXT_MEM_BASE_OFFSET;

    return true;
}
#endif

int32
aot_module_malloc(AOTModuleInstance *module_inst, uint32 size)
{
    uint8 *addr =
        mem_allocator_malloc(module_inst->heap_handle.ptr, size);

    if (!addr) {
        aot_set_exception(module_inst, "out of memory");
        return 0;
    }
    return (int32)(module_inst->heap_base_offset
                   + (addr - (uint8*)module_inst->heap_data.ptr));
}

void
aot_module_free(AOTModuleInstance *module_inst, int32 ptr)
{
    if (ptr) {
        uint8 *addr = (uint8*)module_inst->heap_data.ptr
                       + (ptr - module_inst->heap_base_offset);
        if ((uint8*)module_inst->heap_data.ptr < addr
            && addr < (uint8*)module_inst->heap_data_end.ptr)
            mem_allocator_free(module_inst->heap_handle.ptr, addr);
    }
}

int32
aot_module_dup_data(AOTModuleInstance *module_inst,
                    const char *src, uint32 size)
{
    int32 buffer_offset = aot_module_malloc(module_inst, size);

    if (buffer_offset != 0) {
        char *buffer;
        buffer = aot_addr_app_to_native(module_inst, buffer_offset);
        memcpy(buffer, src, size);
    }
    return buffer_offset;
}

bool
aot_validate_app_addr(AOTModuleInstance *module_inst,
                      int32 app_offset, uint32 size)
{
    uint8 *addr;

    /* integer overflow check */
    if(app_offset + (int32)size < app_offset) {
        goto fail;
    }

    if (0 <= app_offset
        && app_offset < (int32)module_inst->memory_data_size) {
        addr = (uint8*)module_inst->memory_data.ptr + app_offset;
        if (!((uint8*)module_inst->memory_data.ptr <= addr
              && addr + size <= (uint8*)module_inst->memory_data_end.ptr))
            goto fail;
        return true;
    }
    /* Currently heap_size is no more than 1G, and heap_base_offset is 1G,
       heap_base_offset + heap_data_size will not be larger than INT32_MAX */
    else if (module_inst->heap_base_offset < app_offset
             && app_offset < module_inst->heap_base_offset
                             + (int32)module_inst->heap_data_size) {
        addr = (uint8*)module_inst->heap_data.ptr
               + (app_offset - module_inst->heap_base_offset);
        if (!((uint8*)module_inst->heap_data.ptr <= addr
               && addr + size <= (uint8*)module_inst->heap_data_end.ptr))
            goto fail;
        return true;
    }
#if WASM_ENABLE_EXT_MEMORY_SPACE != 0
    else if (module_inst->ext_mem_data.ptr
             && module_inst->ext_mem_base_offset <= app_offset
             && app_offset < module_inst->ext_mem_base_offset
                             + module_inst->ext_mem_data_size) {
        addr = (uint8*)module_inst->ext_mem_data.ptr
               + (app_offset - module_inst->ext_mem_base_offset);
        if (!((uint8*)module_inst->ext_mem_data.ptr <= addr
              && addr + size <= (uint8*)module_inst->ext_mem_data_end.ptr))
            goto fail;

        return true;
    }
#endif

fail:
    aot_set_exception(module_inst, "out of bounds memory access");
    return false;
}

bool
aot_validate_native_addr(AOTModuleInstance *module_inst,
                         void *native_ptr, uint32 size)
{
    uint8 *addr = native_ptr;

    /* integer overflow check */
    if (addr + size < addr) {
        goto fail;
    }

    if (((uint8*)module_inst->memory_data.ptr <= addr
         && addr + size <= (uint8*)module_inst->memory_data_end.ptr)
        || ((uint8*)module_inst->heap_data.ptr <= addr
            && addr + size <= (uint8*)module_inst->heap_data_end.ptr)
#if WASM_ENABLE_EXT_MEMORY_SPACE != 0
        || (module_inst->ext_mem_data.ptr
            && (uint8*)module_inst->ext_mem_data.ptr <= addr
            && addr + size <= (uint8*)module_inst->ext_mem_data_end.ptr)
#endif
       )
        return true;

fail:
    aot_set_exception(module_inst, "out of bounds memory access");
    return false;
}

void *
aot_addr_app_to_native(AOTModuleInstance *module_inst, int32 app_offset)
{
    if (0 <= app_offset && app_offset < module_inst->heap_base_offset)
        return (uint8*)module_inst->memory_data.ptr + app_offset;

    if (module_inst->heap_base_offset < app_offset
        && app_offset < module_inst->heap_base_offset
                        + (int32)module_inst->heap_data_size)
        return (uint8*)module_inst->heap_data.ptr
               + (app_offset - module_inst->heap_base_offset);

#if WASM_ENABLE_EXT_MEMORY_SPACE != 0
    if (module_inst->ext_mem_data.ptr
        && module_inst->ext_mem_base_offset <= app_offset
        && app_offset < module_inst->ext_mem_base_offset
                        + module_inst->ext_mem_data_size)
        return (uint8*)module_inst->ext_mem_data.ptr
               + (app_offset - module_inst->ext_mem_base_offset);
#endif

    return NULL;
}

int32
aot_addr_native_to_app(AOTModuleInstance *module_inst, void *native_ptr)
{
    if ((uint8*)module_inst->memory_data.ptr <= (uint8*)native_ptr
        && (uint8*)native_ptr < (uint8*)module_inst->memory_data_end.ptr)
        return (int32)((uint8*)native_ptr - (uint8*)module_inst->memory_data.ptr);

    if ((uint8*)module_inst->heap_data.ptr <= (uint8*)native_ptr
        && (uint8*)native_ptr < (uint8*)module_inst->heap_data_end.ptr)
        return (int32)(module_inst->heap_base_offset
                       + ((uint8*)native_ptr - (uint8*)module_inst->heap_data.ptr));

#if WASM_ENABLE_EXT_MEMORY_SPACE != 0
    if (module_inst->ext_mem_data.ptr
        && (uint8*)module_inst->ext_mem_data.ptr <= (uint8*)native_ptr
        && (uint8*)native_ptr < (uint8*)module_inst->ext_mem_data_end.ptr)
        return module_inst->ext_mem_base_offset
               + ((uint8*)native_ptr - (uint8*)module_inst->ext_mem_data.ptr);
#endif

    return 0;
}

bool
aot_get_app_addr_range(AOTModuleInstance *module_inst,
                       int32 app_offset,
                       int32 *p_app_start_offset,
                       int32 *p_app_end_offset)
{
    int32 app_start_offset, app_end_offset;

    if (0 <= app_offset && app_offset < (int32)module_inst->memory_data_size) {
        app_start_offset = 0;
        app_end_offset = (int32)module_inst->memory_data_size;
    }
    else if (module_inst->heap_base_offset < app_offset
             && app_offset < module_inst->heap_base_offset
                             + (int32)module_inst->heap_data_size) {
        app_start_offset = module_inst->heap_base_offset;
        app_end_offset = module_inst->heap_base_offset
                         + (int32)module_inst->heap_data_size;
    }
#if WASM_ENABLE_EXT_MEMORY_SPACE != 0
    else if (module_inst->ext_mem_data.ptr
             && module_inst->ext_mem_base_offset <= app_offset
             && app_offset < module_inst->ext_mem_base_offset
                             + module_inst->ext_mem_data_size) {
        app_start_offset = module_inst->ext_mem_base_offset;
        app_end_offset = app_start_offset
                         + module_inst->ext_mem_data_size;
    }
#endif
    else
        return false;

    if (p_app_start_offset)
        *p_app_start_offset = app_start_offset;
    if (p_app_end_offset)
        *p_app_end_offset = app_end_offset;
    return true;
}

bool
aot_get_native_addr_range(AOTModuleInstance *module_inst,
                          uint8 *native_ptr,
                          uint8 **p_native_start_addr,
                          uint8 **p_native_end_addr)
{
    uint8 *native_start_addr, *native_end_addr;

    if ((uint8*)module_inst->memory_data.ptr <= (uint8*)native_ptr
        && (uint8*)native_ptr < (uint8*)module_inst->memory_data_end.ptr) {
        native_start_addr = (uint8*)module_inst->memory_data.ptr;
        native_end_addr = (uint8*)module_inst->memory_data_end.ptr;
    }
    else if ((uint8*)module_inst->heap_data.ptr <= (uint8*)native_ptr
             && (uint8*)native_ptr < (uint8*)module_inst->heap_data_end.ptr) {
        native_start_addr = (uint8*)module_inst->heap_data.ptr;
        native_end_addr = (uint8*)module_inst->heap_data_end.ptr;
    }
#if WASM_ENABLE_EXT_MEMORY_SPACE != 0
    else if (module_inst->ext_mem_data.ptr
             && (uint8*)module_inst->ext_mem_data.ptr <= (uint8*)native_ptr
             && (uint8*)native_ptr < (uint8*)module_inst->ext_mem_data_end.ptr) {
        native_start_addr = (uint8*)module_inst->ext_mem_data.ptr;
        native_end_addr = (uint8*)module_inst->ext_mem_data_end.ptr;
    }
#endif
    else
        return false;

    if (p_native_start_addr)
        *p_native_start_addr = native_start_addr;
    if (p_native_end_addr)
        *p_native_end_addr = native_end_addr;
    return true;
}

bool
aot_enlarge_memory(AOTModuleInstance *module_inst, uint32 inc_page_count)
{
    uint8 *mem_data_old = module_inst->memory_data.ptr, *mem_data_new;
    uint32 cur_page_count = module_inst->mem_cur_page_count;
    uint32 max_page_count = module_inst->mem_max_page_count;
    uint32 total_page_count = cur_page_count + inc_page_count;
    uint32 old_size = NumBytesPerPage * cur_page_count;
    uint64 total_size = (uint64)NumBytesPerPage * total_page_count;

    if (inc_page_count <= 0)
        /* No need to enlarge memory */
        return true;

    if (total_page_count < cur_page_count /* integer overflow */
        || total_page_count > max_page_count) {
        aot_set_exception(module_inst, "fail to enlarge memory.");
        return false;
    }

    if (total_size >= UINT32_MAX
        || !(mem_data_new = wasm_malloc((uint32)total_size))) {
        aot_set_exception(module_inst, "fail to enlarge memory.");
        return false;
    }

    memcpy(mem_data_new, mem_data_old, old_size);
    memset(mem_data_new + old_size, 0, (uint32)total_size - old_size);
    module_inst->mem_cur_page_count = total_page_count;
    module_inst->memory_data_size = (uint32)total_size;
    module_inst->memory_data.ptr = mem_data_new;
    module_inst->memory_data_end.ptr = mem_data_new + (uint32)total_size;

    wasm_free(mem_data_old);
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
