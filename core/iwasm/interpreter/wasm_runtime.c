/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_runtime.h"
#include "wasm_loader.h"
#include "wasm_interp.h"
#include "bh_common.h"
#include "bh_log.h"
#include "mem_alloc.h"
#include "../common/wasm_runtime_common.h"

static void
set_error_buf(char *error_buf, uint32 error_buf_size, const char *string)
{
    if (error_buf != NULL)
        snprintf(error_buf, error_buf_size, "%s", string);
}

WASMModule*
wasm_load(const uint8 *buf, uint32 size,
          char *error_buf, uint32 error_buf_size)
{
    return wasm_loader_load(buf, size, error_buf, error_buf_size);
}

WASMModule*
wasm_load_from_sections(WASMSection *section_list,
                        char *error_buf, uint32_t error_buf_size)
{
    return wasm_loader_load_from_sections(section_list,
                                          error_buf, error_buf_size);
}

void
wasm_unload(WASMModule *module)
{
    wasm_loader_unload(module);
}

/**
 * Destroy memory instances.
 */
static void
memories_deinstantiate(WASMMemoryInstance **memories, uint32 count)
{
    uint32 i;
    if (memories) {
        for (i = 0; i < count; i++)
            if (memories[i]) {
                if (memories[i]->heap_handle)
                    mem_allocator_destroy(memories[i]->heap_handle);
                wasm_runtime_free(memories[i]->heap_data);
                wasm_runtime_free(memories[i]);
            }
        wasm_runtime_free(memories);
  }
}

static WASMMemoryInstance*
memory_instantiate(uint32 num_bytes_per_page,
                   uint32 init_page_count, uint32 max_page_count,
                   uint32 global_data_size,
                   uint32 heap_size,
                   char *error_buf, uint32 error_buf_size)
{
    WASMMemoryInstance *memory;
    uint64 total_size = offsetof(WASMMemoryInstance, base_addr) +
                        num_bytes_per_page * (uint64)init_page_count +
                        global_data_size;

    /* Allocate memory space, addr data and global data */
    if (total_size >= UINT32_MAX
        || !(memory = wasm_runtime_malloc((uint32)total_size))) {
        set_error_buf(error_buf, error_buf_size,
                      "Instantiate memory failed: allocate memory failed.");
        return NULL;
    }

    memset(memory, 0, (uint32)total_size);
    memory->num_bytes_per_page = num_bytes_per_page;
    memory->cur_page_count = init_page_count;
    memory->max_page_count = max_page_count;

    memory->memory_data = memory->base_addr;

    memory->global_data = memory->memory_data +
                          num_bytes_per_page * memory->cur_page_count;
    memory->global_data_size = global_data_size;

    memory->end_addr = memory->global_data + global_data_size;

    /* Allocate heap space */
    if (!(memory->heap_data = wasm_runtime_malloc(heap_size))) {
        set_error_buf(error_buf, error_buf_size,
                      "Instantiate memory failed: allocate memory failed.");
        goto fail1;
    }
    memory->heap_data_end = memory->heap_data + heap_size;

    /* Initialize heap */
    if (!(memory->heap_handle = mem_allocator_create
                (memory->heap_data, heap_size))) {
        goto fail2;
    }

#if WASM_ENABLE_MEMORY_GROW != 0
    memory->heap_base_offset = DEFAULT_APP_HEAP_BASE_OFFSET;
#else
    memory->heap_base_offset = memory->end_addr - memory->memory_data;
#endif

    return memory;

fail2:
    wasm_runtime_free(memory->heap_data);

fail1:
    wasm_runtime_free(memory);
    return NULL;
}

/**
 * Instantiate memories in a module.
 */
static WASMMemoryInstance**
memories_instantiate(const WASMModule *module,
                     uint32 global_data_size, uint32 heap_size,
                     char *error_buf, uint32 error_buf_size)
{
    WASMImport *import;
    uint32 mem_index = 0, i, memory_count =
        module->import_memory_count + module->memory_count;
    uint64 total_size;
    WASMMemoryInstance **memories, *memory;

    if (memory_count == 0 && global_data_size > 0)
        memory_count = 1;

    total_size = sizeof(WASMMemoryInstance*) * (uint64)memory_count;

    if (total_size >= UINT32_MAX
        || !(memories = wasm_runtime_malloc((uint32)total_size))) {
        set_error_buf(error_buf, error_buf_size,
                      "Instantiate memory failed: "
                      "allocate memory failed.");
        return NULL;
    }

    memset(memories, 0, (uint32)total_size);

    /* instantiate memories from import section */
    import = module->import_memories;
    for (i = 0; i < module->import_memory_count; i++, import++) {
        if (!(memory = memories[mem_index++] =
                    memory_instantiate(import->u.memory.num_bytes_per_page,
                                       import->u.memory.init_page_count,
                                       import->u.memory. max_page_count,
                                       global_data_size,
                                       heap_size, error_buf, error_buf_size))) {
            set_error_buf(error_buf, error_buf_size,
                         "Instantiate memory failed: "
                         "allocate memory failed.");
            memories_deinstantiate(memories, memory_count);
            return NULL;
        }
    }

    /* instantiate memories from memory section */
    for (i = 0; i < module->memory_count; i++) {
        if (!(memory = memories[mem_index++] =
                    memory_instantiate(module->memories[i].num_bytes_per_page,
                                       module->memories[i].init_page_count,
                                       module->memories[i].max_page_count,
                                       global_data_size,
                                       heap_size, error_buf, error_buf_size))) {
            set_error_buf(error_buf, error_buf_size,
                          "Instantiate memory failed: "
                          "allocate memory failed.");
            memories_deinstantiate(memories, memory_count);
            return NULL;
        }
    }

    if (mem_index == 0) {
        /* no import memory and define memory, but has global variables */
        if (!(memory = memories[mem_index++] =
                    memory_instantiate(0, 0, 0, global_data_size,
                                       heap_size, error_buf, error_buf_size))) {
            set_error_buf(error_buf, error_buf_size,
                          "Instantiate memory failed: "
                          "allocate memory failed.\n");
            memories_deinstantiate(memories, memory_count);
            return NULL;
        }
    }

    bh_assert(mem_index == memory_count);
    return memories;
}

/**
 * Destroy table instances.
 */
static void
tables_deinstantiate(WASMTableInstance **tables, uint32 count)
{
    uint32 i;
    if (tables) {
        for (i = 0; i < count; i++)
            if (tables[i])
                wasm_runtime_free(tables[i]);
        wasm_runtime_free(tables);
    }
}

/**
 * Instantiate tables in a module.
 */
static WASMTableInstance**
tables_instantiate(const WASMModule *module,
                   char *error_buf, uint32 error_buf_size)
{
    WASMImport *import;
    uint32 table_index = 0, i, table_count =
        module->import_table_count + module->table_count;
    uint64 total_size = sizeof(WASMTableInstance*) * (uint64)table_count;
    WASMTableInstance **tables, *table;

    if (total_size >= UINT32_MAX
        || !(tables = wasm_runtime_malloc((uint32)total_size))) {
        set_error_buf(error_buf, error_buf_size,
                      "Instantiate table failed: "
                      "allocate memory failed.");
        return NULL;
    }

    memset(tables, 0, (uint32)total_size);

    /* instantiate tables from import section */
    import = module->import_tables;
    for (i = 0; i < module->import_table_count; i++, import++) {
        total_size = offsetof(WASMTableInstance, base_addr) +
                     sizeof(uint32) * (uint64)import->u.table.init_size;
        if (total_size >= UINT32_MAX
            || !(table = tables[table_index++] =
                        wasm_runtime_malloc((uint32)total_size))) {
            set_error_buf(error_buf, error_buf_size,
                          "Instantiate table failed: "
                          "allocate memory failed.");
            tables_deinstantiate(tables, table_count);
            return NULL;
        }

        /* Set all elements to -1 to mark them as uninitialized elements */
        memset(table, -1, (uint32)total_size);
        table->elem_type = import->u.table.elem_type;
        table->cur_size = import->u.table.init_size;
        table->max_size = import->u.table.max_size;
    }

    /* instantiate tables from table section */
    for (i = 0; i < module->table_count; i++) {
        total_size = offsetof(WASMTableInstance, base_addr) +
                     sizeof(uint32) * (uint64)module->tables[i].init_size;
        if (total_size >= UINT32_MAX
            || !(table = tables[table_index++] =
                        wasm_runtime_malloc((uint32)total_size))) {
            set_error_buf(error_buf, error_buf_size,
                          "Instantiate table failed: "
                          "allocate memory failed.");
            tables_deinstantiate(tables, table_count);
            return NULL;
        }

        /* Set all elements to -1 to mark them as uninitialized elements */
        memset(table, -1, (uint32)total_size);
        table->elem_type = module->tables[i].elem_type;
        table->cur_size = module->tables[i].init_size;
        table->max_size = module->tables[i].max_size;
    }

    bh_assert(table_index == table_count);
    return tables;
}

/**
 * Destroy function instances.
 */
static void
functions_deinstantiate(WASMFunctionInstance *functions, uint32 count)
{
    if (functions) {
        wasm_runtime_free(functions);
    }
}

/**
 * Instantiate functions in a module.
 */
static WASMFunctionInstance*
functions_instantiate(const WASMModule *module,
                      char *error_buf, uint32 error_buf_size)
{
    WASMImport *import;
    uint32 i, function_count =
        module->import_function_count + module->function_count;
    uint64 total_size = sizeof(WASMFunctionInstance) * (uint64)function_count;
    WASMFunctionInstance *functions, *function;

    if (total_size >= UINT32_MAX
        || !(functions = wasm_runtime_malloc((uint32)total_size))) {
        set_error_buf(error_buf, error_buf_size,
                      "Instantiate function failed: "
                      "allocate memory failed.");
        return NULL;
    }

    memset(functions, 0, (uint32)total_size);

    /* instantiate functions from import section */
    function = functions;
    import = module->import_functions;
    for (i = 0; i < module->import_function_count; i++, import++) {
        function->is_import_func = true;
        function->u.func_import = &import->u.function;

        function->param_cell_num =
            wasm_type_param_cell_num(import->u.function.func_type);
        function->ret_cell_num =
            wasm_type_return_cell_num(import->u.function.func_type);
        function->local_cell_num = 0;

        function->param_count =
            (uint16)function->u.func_import->func_type->param_count;
        function->local_count = 0;
        function->param_types = function->u.func_import->func_type->types;
        function->local_types = NULL;

        function++;
    }

    /* instantiate functions from function section */
    for (i = 0; i < module->function_count; i++) {
        function->is_import_func = false;
        function->u.func = module->functions[i];

        function->param_cell_num = function->u.func->param_cell_num;
        function->ret_cell_num = function->u.func->ret_cell_num;
        function->local_cell_num = function->u.func->local_cell_num;

        function->param_count = (uint16)function->u.func->func_type->param_count;
        function->local_count = (uint16)function->u.func->local_count;
        function->param_types = function->u.func->func_type->types;
        function->local_types = function->u.func->local_types;

        function->local_offsets = function->u.func->local_offsets;

#if WASM_ENABLE_FAST_INTERP != 0
        function->const_cell_num = function->u.func->const_cell_num;
#endif

        function++;
    }

    bh_assert((uint32)(function - functions) == function_count);
    return functions;
}

/**
 * Destroy global instances.
 */
static void
globals_deinstantiate(WASMGlobalInstance *globals)
{
    if (globals)
        wasm_runtime_free(globals);
}

/**
 * Instantiate globals in a module.
 */
static WASMGlobalInstance*
globals_instantiate(const WASMModule *module,
                    uint32 *p_global_data_size,
                    char *error_buf, uint32 error_buf_size)
{
    WASMImport *import;
    uint32 global_data_offset = 0;
    uint32 i, global_count =
        module->import_global_count + module->global_count;
    uint64 total_size = sizeof(WASMGlobalInstance) * (uint64)global_count;
    WASMGlobalInstance *globals, *global;

    if (total_size >= UINT32_MAX
        || !(globals = wasm_runtime_malloc((uint32)total_size))) {
        set_error_buf(error_buf, error_buf_size,
                      "Instantiate global failed: "
                      "allocate memory failed.");
        return NULL;
    }

    memset(globals, 0, (uint32)total_size);

    /* instantiate globals from import section */
    global = globals;
    import = module->import_globals;
    for (i = 0; i < module->import_global_count; i++, import++) {
        WASMGlobalImport *global_import = &import->u.global;
        global->type = global_import->type;
        global->is_mutable = global_import->is_mutable;
        global->initial_value = global_import->global_data_linked;
        global->data_offset = global_data_offset;
        global_data_offset += wasm_value_type_size(global->type);

        global++;
    }

    /* instantiate globals from global section */
    for (i = 0; i < module->global_count; i++) {
        global->type = module->globals[i].type;
        global->is_mutable = module->globals[i].is_mutable;

        global->data_offset = global_data_offset;
        global_data_offset += wasm_value_type_size(global->type);

        global++;
    }

    bh_assert((uint32)(global - globals) == global_count);
    *p_global_data_size = global_data_offset;
    return globals;
}

static void
globals_instantiate_fix(WASMGlobalInstance *globals,
                        const WASMModule *module,
                        WASMModuleInstance *module_inst)
{
    WASMGlobalInstance *global = globals;
    WASMImport *import = module->import_globals;
    uint32 i;

    /* Fix globals from import section */
    for (i = 0; i < module->import_global_count; i++, import++, global++) {
        if (!strcmp(import->u.names.module_name, "env")) {
            if (!strcmp(import->u.names.field_name, "memoryBase")
                || !strcmp(import->u.names.field_name, "__memory_base")) {
                global->initial_value.addr = 0;
            }
            else if (!strcmp(import->u.names.field_name, "tableBase")
                     || !strcmp(import->u.names.field_name, "__table_base")) {
                global->initial_value.addr = 0;
            }
            else if (!strcmp(import->u.names.field_name, "DYNAMICTOP_PTR")) {
                global->initial_value.i32 = (int32)
                    (module_inst->default_memory->num_bytes_per_page
                     * module_inst->default_memory->cur_page_count);
                module_inst->DYNAMICTOP_PTR_offset = global->data_offset;
            }
            else if (!strcmp(import->u.names.field_name, "STACKTOP")) {
                global->initial_value.i32 = 0;
            }
            else if (!strcmp(import->u.names.field_name, "STACK_MAX")) {
                /* Unused in emcc wasm bin actually. */
                global->initial_value.i32 = 0;
            }
        }
    }

    for (i = 0; i < module->global_count; i++) {
        InitializerExpression *init_expr = &module->globals[i].init_expr;

        if (init_expr->init_expr_type == INIT_EXPR_TYPE_GET_GLOBAL) {
            bh_assert(init_expr->u.global_index < module->import_global_count);
            global->initial_value = globals[init_expr->u.global_index].initial_value;
        }
        else {
            bh_memcpy_s(&global->initial_value, sizeof(WASMValue),
                        &init_expr->u, sizeof(init_expr->u));
        }
        global++;
    }
}

/**
 * Return export function count in module export section.
 */
static uint32
get_export_function_count(const WASMModule *module)
{
    WASMExport *export = module->exports;
    uint32 count = 0, i;

    for (i = 0; i < module->export_count; i++, export++)
        if (export->kind == EXPORT_KIND_FUNC)
            count++;

    return count;
}

/**
 * Destroy export function instances.
 */
static void
export_functions_deinstantiate(WASMExportFuncInstance *functions)
{
    if (functions)
        wasm_runtime_free(functions);
}

/**
 * Instantiate export functions in a module.
 */
static WASMExportFuncInstance*
export_functions_instantiate(const WASMModule *module,
                             WASMModuleInstance *module_inst,
                             uint32 export_func_count,
                             char *error_buf, uint32 error_buf_size)
{
    WASMExportFuncInstance *export_funcs, *export_func;
    WASMExport *export = module->exports;
    uint32 i;
    uint64 total_size = sizeof(WASMExportFuncInstance) * (uint64)export_func_count;

    if (total_size >= UINT32_MAX
        || !(export_func = export_funcs = wasm_runtime_malloc((uint32)total_size))) {
        set_error_buf(error_buf, error_buf_size,
                      "Instantiate export function failed: "
                      "allocate memory failed.");
        return NULL;
    }

    memset(export_funcs, 0, (uint32)total_size);

    for (i = 0; i < module->export_count; i++, export++)
        if (export->kind == EXPORT_KIND_FUNC) {
            export_func->name = export->name;
            export_func->function = &module_inst->functions[export->index];
            export_func++;
        }

    bh_assert((uint32)(export_func - export_funcs) == export_func_count);
    return export_funcs;
}

static bool
execute_post_inst_function(WASMModuleInstance *module_inst)
{
    WASMFunctionInstance *post_inst_func = NULL;
    WASMType *post_inst_func_type;
    uint32 i;

    for (i = 0; i < module_inst->export_func_count; i++)
        if (!strcmp(module_inst->export_functions[i].name, "__post_instantiate")) {
            post_inst_func = module_inst->export_functions[i].function;
            break;
        }

    if (!post_inst_func)
        /* Not found */
        return true;

    post_inst_func_type = post_inst_func->u.func->func_type;
    if (post_inst_func_type->param_count != 0
        || post_inst_func_type->result_count != 0)
        /* Not a valid function type, ignore it */
        return true;

    return wasm_create_exec_env_and_call_function(module_inst, post_inst_func,
                                                  0, NULL);
}

static bool
execute_start_function(WASMModuleInstance *module_inst)
{
    WASMFunctionInstance *func = module_inst->start_function;

    if (!func)
        return true;

    bh_assert(!func->is_import_func && func->param_cell_num == 0
              && func->ret_cell_num == 0);

    return wasm_create_exec_env_and_call_function(module_inst, func, 0, NULL);
}

/**
 * Instantiate module
 */
WASMModuleInstance*
wasm_instantiate(WASMModule *module,
                 uint32 stack_size, uint32 heap_size,
                 char *error_buf, uint32 error_buf_size)
{
    WASMModuleInstance *module_inst;
    WASMTableSeg *table_seg;
    WASMDataSeg *data_seg;
    WASMGlobalInstance *globals = NULL, *global;
    uint32 global_count, global_data_size = 0, i, j;
    uint32 base_offset, length, memory_size;
    uint8 *global_data, *global_data_end;
    uint8 *memory_data;
    uint32 *table_data;

    if (!module)
        return NULL;

    /* Check heap size */
    heap_size = align_uint(heap_size, 8);
    if (heap_size == 0)
        heap_size = APP_HEAP_SIZE_DEFAULT;
    if (heap_size < APP_HEAP_SIZE_MIN)
        heap_size = APP_HEAP_SIZE_MIN;
    if (heap_size > APP_HEAP_SIZE_MAX)
        heap_size = APP_HEAP_SIZE_MAX;

    /* Instantiate global firstly to get the mutable data size */
    global_count = module->import_global_count + module->global_count;
    if (global_count &&
        !(globals = globals_instantiate(module,
                                        &global_data_size,
                                        error_buf, error_buf_size)))
        return NULL;

    /* Allocate the memory */
    if (!(module_inst = wasm_runtime_malloc((uint32)sizeof(WASMModuleInstance)))) {
        set_error_buf(error_buf, error_buf_size,
                      "Instantiate module failed: allocate memory failed.");
        globals_deinstantiate(globals);
        return NULL;
    }

    memset(module_inst, 0, (uint32)sizeof(WASMModuleInstance));
    module_inst->global_count = global_count;
    module_inst->globals = globals;

    module_inst->memory_count =
        module->import_memory_count + module->memory_count;
    module_inst->table_count =
        module->import_table_count + module->table_count;
    module_inst->function_count =
        module->import_function_count + module->function_count;
    module_inst->export_func_count = get_export_function_count(module);

    /* Instantiate memories/tables/functions */
    if (((module_inst->memory_count > 0 || global_count > 0)
         && !(module_inst->memories =
             memories_instantiate(module, global_data_size,
                                  heap_size, error_buf, error_buf_size)))
        || (module_inst->table_count > 0
            && !(module_inst->tables = tables_instantiate(module,
                                                          error_buf,
                                                          error_buf_size)))
        || (module_inst->function_count > 0
            && !(module_inst->functions = functions_instantiate(module,
                                                                error_buf,
                                                                error_buf_size)))
        || (module_inst->export_func_count > 0
            && !(module_inst->export_functions = export_functions_instantiate(
                    module, module_inst, module_inst->export_func_count,
                    error_buf, error_buf_size)))) {
        wasm_deinstantiate(module_inst);
        return NULL;
    }

    if (module_inst->memory_count || global_count > 0) {
        WASMMemoryInstance *memory;

        memory = module_inst->default_memory = module_inst->memories[0];
        memory_data = module_inst->default_memory->memory_data;

        /* fix import memoryBase */
        globals_instantiate_fix(globals, module, module_inst);

        /* Initialize the global data */
        global_data = memory->global_data;
        global_data_end = global_data + global_data_size;
        global = globals;
        for (i = 0; i < global_count; i++, global++) {
            switch (global->type) {
                case VALUE_TYPE_I32:
                case VALUE_TYPE_F32:
                    *(int32*)global_data = global->initial_value.i32;
                    global_data += sizeof(int32);
                    break;
                case VALUE_TYPE_I64:
                case VALUE_TYPE_F64:
                    bh_memcpy_s(global_data, (uint32)(global_data_end - global_data),
                                &global->initial_value.i64, sizeof(int64));
                    global_data += sizeof(int64);
                    break;
                default:
                    bh_assert(0);
            }
        }
        bh_assert(global_data == global_data_end);

        /* Initialize the memory data with data segment section */
        if (module_inst->default_memory->cur_page_count > 0) {
            for (i = 0; i < module->data_seg_count; i++) {
                data_seg = module->data_segments[i];
                bh_assert(data_seg->memory_index == 0);
                bh_assert(data_seg->base_offset.init_expr_type ==
                            INIT_EXPR_TYPE_I32_CONST
                          || data_seg->base_offset.init_expr_type ==
                            INIT_EXPR_TYPE_GET_GLOBAL);

                if (data_seg->base_offset.init_expr_type == INIT_EXPR_TYPE_GET_GLOBAL) {
                    bh_assert(data_seg->base_offset.u.global_index < global_count
                              && globals[data_seg->base_offset.u.global_index].type ==
                                    VALUE_TYPE_I32);
                    data_seg->base_offset.u.i32 =
                        globals[data_seg->base_offset.u.global_index].initial_value.i32;
                }

                base_offset = (uint32)data_seg->base_offset.u.i32;
                length = data_seg->data_length;
                memory_size = module_inst->default_memory->num_bytes_per_page
                              * module_inst->default_memory->cur_page_count;

                if (length > 0
                    && (base_offset >= memory_size
                        || base_offset + length > memory_size)) {
                    set_error_buf(error_buf, error_buf_size,
                            "Instantiate module failed: data segment out of range.");
                    wasm_deinstantiate(module_inst);
                    return NULL;
                }

                bh_memcpy_s(memory_data + base_offset, memory_size - base_offset,
                            data_seg->data, length);
            }
        }
    }

    if (module_inst->table_count) {
        module_inst->default_table = module_inst->tables[0];

        /* Initialize the table data with table segment section */
        table_data = (uint32*)module_inst->default_table->base_addr;
        table_seg = module->table_segments;
        for (i = 0; i < module->table_seg_count; i++, table_seg++) {
            bh_assert(table_seg->table_index == 0);
            bh_assert(table_seg->base_offset.init_expr_type ==
                        INIT_EXPR_TYPE_I32_CONST
                      || table_seg->base_offset.init_expr_type ==
                        INIT_EXPR_TYPE_GET_GLOBAL);

            if (table_seg->base_offset.init_expr_type ==
                    INIT_EXPR_TYPE_GET_GLOBAL) {
                bh_assert(table_seg->base_offset.u.global_index < global_count
                          && globals[table_seg->base_offset.u.global_index].type ==
                                VALUE_TYPE_I32);
                table_seg->base_offset.u.i32 =
                    globals[table_seg->base_offset.u.global_index].initial_value.i32;
            }
            if ((uint32)table_seg->base_offset.u.i32 <
                    module_inst->default_table->cur_size) {
                length = table_seg->function_count;
                if ((uint32)table_seg->base_offset.u.i32 + length >
                        module_inst->default_table->cur_size)
                    length = module_inst->default_table->cur_size
                             - (uint32)table_seg->base_offset.u.i32;
                /* Check function index */
                for (j = 0; j < length; j++) {
                    if (table_seg->func_indexes[j] >= module_inst->function_count) {
                        set_error_buf(error_buf, error_buf_size,
                                      "function index is overflow");
                        wasm_deinstantiate(module_inst);
                        return NULL;
                    }
                }
                bh_memcpy_s(table_data + table_seg->base_offset.u.i32,
                            (uint32)((module_inst->default_table->cur_size
                                      - (uint32)table_seg->base_offset.u.i32)
                                     * sizeof(uint32)),
                            table_seg->func_indexes, (uint32)(length * sizeof(uint32)));
            }
        }
    }

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
                                error_buf, error_buf_size)) {
        wasm_deinstantiate(module_inst);
        return NULL;
    }
#endif

    if (module->start_function != (uint32)-1) {
        bh_assert(module->start_function >= module->import_function_count);
        module_inst->start_function =
            &module_inst->functions[module->start_function];
    }

    module_inst->module = module;

    /* module instance type */
    module_inst->module_type = Wasm_Module_Bytecode;

    /* Initialize the thread related data */
    if (stack_size == 0)
        stack_size = DEFAULT_WASM_STACK_SIZE;
    module_inst->default_wasm_stack_size = stack_size;

    /* Execute __post_instantiate function */
    if (!execute_post_inst_function(module_inst)
        || !execute_start_function(module_inst)) {
        set_error_buf(error_buf, error_buf_size,
                      module_inst->cur_exception);
        wasm_deinstantiate(module_inst);
        return NULL;
    }

    (void)global_data_end;
    return module_inst;
}

void
wasm_deinstantiate(WASMModuleInstance *module_inst)
{
    if (!module_inst)
        return;

#if WASM_ENABLE_LIBC_WASI != 0
    /* Destroy wasi resource before freeing app heap, since some fields of
       wasi contex are allocated from app heap, and if app heap is freed,
       these fields will be set to NULL, we cannot free their internal data
       which may allocated from global heap. */
    wasm_runtime_destroy_wasi((WASMModuleInstanceCommon*)module_inst);
#endif

    if (module_inst->memory_count > 0)
        memories_deinstantiate(module_inst->memories, module_inst->memory_count);
    else if (module_inst->memories != NULL && module_inst->global_count > 0)
        /* No imported memory and defined memory, the memory is created when
           global count > 0. */
        memories_deinstantiate(module_inst->memories, 1);

    tables_deinstantiate(module_inst->tables, module_inst->table_count);
    functions_deinstantiate(module_inst->functions, module_inst->function_count);
    globals_deinstantiate(module_inst->globals);
    export_functions_deinstantiate(module_inst->export_functions);

    wasm_runtime_free(module_inst);
}

WASMFunctionInstance*
wasm_lookup_function(const WASMModuleInstance *module_inst,
                     const char *name, const char *signature)
{
    uint32 i;
    for (i = 0; i < module_inst->export_func_count; i++)
        if (!strcmp(module_inst->export_functions[i].name, name))
            return module_inst->export_functions[i].function;
    (void)signature;
    return NULL;
}

bool
wasm_call_function(WASMExecEnv *exec_env,
                   WASMFunctionInstance *function,
                   unsigned argc, uint32 argv[])
{
    WASMModuleInstance *module_inst = (WASMModuleInstance*)exec_env->module_inst;
    wasm_interp_call_wasm(module_inst, exec_env, function, argc, argv);
    return !wasm_get_exception(module_inst) ? true : false;
}

bool
wasm_create_exec_env_and_call_function(WASMModuleInstance *module_inst,
                                       WASMFunctionInstance *func,
                                       unsigned argc, uint32 argv[])
{
    WASMExecEnv *exec_env;
    bool ret;

    if (!(exec_env = wasm_exec_env_create((WASMModuleInstanceCommon*)module_inst,
                                          module_inst->default_wasm_stack_size))) {
        wasm_set_exception(module_inst, "allocate memory failed.");
        return false;
    }

    ret = wasm_call_function(exec_env, func, argc, argv);
    wasm_exec_env_destroy(exec_env);
    return ret;
}

void
wasm_set_exception(WASMModuleInstance *module_inst,
                   const char *exception)
{
    if (exception)
        snprintf(module_inst->cur_exception,
                 sizeof(module_inst->cur_exception),
                 "Exception: %s", exception);
    else
        module_inst->cur_exception[0] = '\0';
}

const char*
wasm_get_exception(WASMModuleInstance *module_inst)
{
    if (module_inst->cur_exception[0] == '\0')
        return NULL;
    else
        return module_inst->cur_exception;
}

int32
wasm_module_malloc(WASMModuleInstance *module_inst, uint32 size,
                   void **p_native_addr)
{
    WASMMemoryInstance *memory = module_inst->default_memory;
    uint8 *addr = mem_allocator_malloc(memory->heap_handle, size);
    if (p_native_addr)
        *p_native_addr = addr;
    if (!addr) {
        wasm_set_exception(module_inst, "out of memory");
        return 0;
    }
    return memory->heap_base_offset + (int32)(addr - memory->heap_data);
}

void
wasm_module_free(WASMModuleInstance *module_inst, int32 ptr)
{
    if (ptr) {
        WASMMemoryInstance *memory = module_inst->default_memory;
        uint8 *addr = memory->heap_data + (ptr - memory->heap_base_offset);
        if (memory->heap_data < addr && addr < memory->heap_data_end)
            mem_allocator_free(memory->heap_handle, addr);
    }
}

int32
wasm_module_dup_data(WASMModuleInstance *module_inst,
                     const char *src, uint32 size)
{
    char *buffer;
    int32 buffer_offset = wasm_module_malloc(module_inst, size,
                                             (void**)&buffer);
    if (buffer_offset != 0) {
        buffer = wasm_addr_app_to_native(module_inst, buffer_offset);
        bh_memcpy_s(buffer, size, src, size);
    }
    return buffer_offset;
}

bool
wasm_validate_app_addr(WASMModuleInstance *module_inst,
                       int32 app_offset, uint32 size)
{
    WASMMemoryInstance *memory;
    uint8 *addr;

    /* integer overflow check */
    if(app_offset + (int32)size < app_offset) {
        goto fail;
    }

    memory = module_inst->default_memory;
    if (0 <= app_offset
        && app_offset < memory->heap_base_offset) {
        addr = memory->memory_data + app_offset;
        if (!(memory->base_addr <= addr && addr + size <= memory->end_addr))
            goto fail;
        return true;
    }
    else if (memory->heap_base_offset < app_offset
             && app_offset < memory->heap_base_offset
                             + (memory->heap_data_end - memory->heap_data)) {
        addr = memory->heap_data + (app_offset - memory->heap_base_offset);
        if (!(memory->heap_data <= addr && addr + size <= memory->heap_data_end))
            goto fail;
        return true;
    }

fail:
    wasm_set_exception(module_inst, "out of bounds memory access");
    return false;
}

bool
wasm_validate_native_addr(WASMModuleInstance *module_inst,
                          void *native_ptr, uint32 size)
{
    uint8 *addr = native_ptr;
    WASMMemoryInstance *memory = module_inst->default_memory;

    if (addr + size < addr) {
        goto fail;
    }

    if ((memory->base_addr <= addr && addr + size <= memory->end_addr)
        || (memory->heap_data <= addr && addr + size <= memory->heap_data_end)
       )
        return true;

fail:
    wasm_set_exception(module_inst, "out of bounds memory access");
    return false;
}

void *
wasm_addr_app_to_native(WASMModuleInstance *module_inst,
                        int32 app_offset)
{
    WASMMemoryInstance *memory = module_inst->default_memory;
    if (0 <= app_offset && app_offset < memory->heap_base_offset)
        return memory->memory_data + app_offset;
    else if (memory->heap_base_offset < app_offset
             && app_offset < memory->heap_base_offset
                             + (memory->heap_data_end - memory->heap_data))
        return memory->heap_data + (app_offset - memory->heap_base_offset);
    else
        return NULL;
}

int32
wasm_addr_native_to_app(WASMModuleInstance *module_inst,
                        void *native_ptr)
{
    WASMMemoryInstance *memory = module_inst->default_memory;
    if (memory->base_addr <= (uint8*)native_ptr
        && (uint8*)native_ptr < memory->end_addr)
        return (int32)((uint8*)native_ptr - memory->memory_data);
    else if (memory->heap_data <= (uint8*)native_ptr
             && (uint8*)native_ptr < memory->heap_data_end)
        return memory->heap_base_offset
               + (int32)((uint8*)native_ptr - memory->heap_data);
    else
        return 0;
}

bool
wasm_get_app_addr_range(WASMModuleInstance *module_inst,
                        int32 app_offset,
                        int32 *p_app_start_offset,
                        int32 *p_app_end_offset)
{
    int32 app_start_offset, app_end_offset;
    WASMMemoryInstance *memory = module_inst->default_memory;

    if (0 <= app_offset && app_offset < memory->heap_base_offset) {
        app_start_offset = 0;
        app_end_offset = (int32)(memory->num_bytes_per_page * memory->cur_page_count);
    }
    else if (memory->heap_base_offset < app_offset
             && app_offset < memory->heap_base_offset
                             + (memory->heap_data_end - memory->heap_data)) {
        app_start_offset = memory->heap_base_offset;
        app_end_offset = memory->heap_base_offset
                         + (int32)(memory->heap_data_end - memory->heap_data);
    }
    else
        return false;

    if (p_app_start_offset)
        *p_app_start_offset = app_start_offset;
    if (p_app_end_offset)
        *p_app_end_offset = app_end_offset;
    return true;
}

bool
wasm_get_native_addr_range(WASMModuleInstance *module_inst,
                           uint8 *native_ptr,
                           uint8 **p_native_start_addr,
                           uint8 **p_native_end_addr)
{
    uint8 *native_start_addr, *native_end_addr;
    WASMMemoryInstance *memory = module_inst->default_memory;

    if (memory->base_addr <= (uint8*)native_ptr
        && (uint8*)native_ptr < memory->end_addr) {
        native_start_addr = memory->memory_data;
        native_end_addr = memory->memory_data
                          + memory->num_bytes_per_page * memory->cur_page_count;
    }
    else if (memory->heap_data <= (uint8*)native_ptr
             && (uint8*)native_ptr < memory->heap_data_end) {
        native_start_addr = memory->heap_data;
        native_end_addr = memory->heap_data_end;
    }
    else
        return false;

    if (p_native_start_addr)
        *p_native_start_addr = native_start_addr;
    if (p_native_end_addr)
        *p_native_end_addr = native_end_addr;
    return true;
}

bool
wasm_enlarge_memory(WASMModuleInstance *module, uint32 inc_page_count)
{
#if WASM_ENABLE_MEMORY_GROW != 0
    WASMMemoryInstance *memory = module->default_memory, *new_memory;
    uint32 old_page_count = memory->cur_page_count;
    uint32 total_size_old = memory->end_addr - (uint8*)memory;
    uint32 total_page_count = inc_page_count + memory->cur_page_count;
    uint64 total_size = offsetof(WASMMemoryInstance, base_addr) +
                        memory->num_bytes_per_page * (uint64)total_page_count +
                        memory->global_data_size;
    uint8 *global_data_old;

    if (inc_page_count <= 0)
        /* No need to enlarge memory */
        return true;

    if (total_page_count < memory->cur_page_count /* integer overflow */
        || total_page_count > memory->max_page_count) {
        wasm_set_exception(module, "fail to enlarge memory.");
        return false;
    }

    if (total_size >= UINT32_MAX) {
        wasm_set_exception(module, "fail to enlarge memory.");
        return false;
    }

    if (!(new_memory = wasm_runtime_realloc(memory, (uint32)total_size))) {
        if (!(new_memory = wasm_runtime_malloc((uint32)total_size))) {
            wasm_set_exception(module, "fail to enlarge memory.");
            return false;
        }
        bh_memcpy_s((uint8*)new_memory, (uint32)total_size,
                    (uint8*)memory, total_size_old);
        wasm_runtime_free(memory);
    }

    memset((uint8*)new_memory + total_size_old,
           0, (uint32)total_size - total_size_old);

    new_memory->cur_page_count = total_page_count;
    new_memory->memory_data = new_memory->base_addr;
    new_memory->global_data = new_memory->memory_data +
                              new_memory->num_bytes_per_page * total_page_count;
    new_memory->end_addr = new_memory->global_data + new_memory->global_data_size;

    global_data_old = new_memory->memory_data +
                              new_memory->num_bytes_per_page * old_page_count;

    /* Copy global data */
    bh_memcpy_s(new_memory->global_data, new_memory->global_data_size,
                global_data_old, new_memory->global_data_size);
    memset(global_data_old, 0, new_memory->global_data_size);

    module->memories[0] = module->default_memory = new_memory;
    return true;
#else /* else of WASM_ENABLE_MEMORY_GROW */
    wasm_set_exception(module, "unsupported operation: enlarge memory.");
    return false;
#endif /* end of WASM_ENABLE_MEMORY_GROW */
}

