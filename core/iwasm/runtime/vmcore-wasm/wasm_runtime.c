/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "wasm_runtime.h"
#include "wasm_thread.h"
#include "wasm_loader.h"
#include "wasm_native.h"
#include "wasm_interp.h"
#include "wasm_log.h"
#include "wasm_platform_log.h"
#include "wasm_memory.h"
#include "mem_alloc.h"


static void
set_error_buf(char *error_buf, uint32 error_buf_size, const char *string)
{
    if (error_buf != NULL)
        snprintf(error_buf, error_buf_size, "%s", string);
}

bool
wasm_runtime_init()
{
    if (wasm_platform_init() != 0)
        return false;

    if (wasm_log_init() != 0)
        return false;

    if (ws_thread_sys_init() != 0)
        return false;

    wasm_runtime_set_tlr(NULL);

    wasm_native_init();
    return true;
}

void
wasm_runtime_destroy()
{
    wasm_runtime_set_tlr(NULL);
    ws_thread_sys_destroy();
}

static void
init_wasm_stack(WASMStack *wasm_stack, uint8 *stack, uint32 stack_size)
{
    wasm_stack->top = wasm_stack->bottom = stack;
    wasm_stack->top_boundary = stack + stack_size;
}

bool
wasm_runtime_call_wasm(WASMModuleInstance *module_inst,
                       WASMExecEnv *exec_env,
                       WASMFunctionInstance *function,
                       unsigned argc, uint32 argv[])
{
    if (!exec_env) {
        if (!module_inst->wasm_stack) {
            if (!(module_inst->wasm_stack =
                        wasm_malloc(module_inst->wasm_stack_size))) {
                wasm_runtime_set_exception(module_inst, "allocate memory failed.");
                return false;
            }
        }

        init_wasm_stack(&module_inst->main_tlr.wasm_stack,
                        module_inst->wasm_stack, module_inst->wasm_stack_size);
    }
    else {
        uintptr_t stack = (uintptr_t)exec_env->stack;
        uint32 stack_size;

        /* Set to 8 bytes align */
        stack = (stack + 7) & ~7;
        stack_size = exec_env->stack_size - (stack - (uintptr_t)exec_env->stack);

        if (!exec_env->stack || exec_env->stack_size <= 0
            || exec_env->stack_size < stack - (uintptr_t)exec_env->stack) {
            wasm_runtime_set_exception(module_inst, "Invalid execution stack info.");
            return false;
        }

        init_wasm_stack(&module_inst->main_tlr.wasm_stack, (uint8*)stack, stack_size);
    }

    wasm_interp_call_wasm(function, argc, argv);
    return !wasm_runtime_get_exception(module_inst) ? true : false;
}

void
wasm_runtime_set_exception(WASMModuleInstance *module_inst,
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
wasm_runtime_get_exception(WASMModuleInstance *module_inst)
{
    if (module_inst->cur_exception[0] == '\0')
        return NULL;
    else
        return module_inst->cur_exception;
}

void
wasm_runtime_clear_exception(WASMModuleInstance *module_inst)
{
    wasm_runtime_set_exception(module_inst, NULL);
}

WASMModule*
wasm_runtime_load(const uint8 *buf, uint32 size,
                  char *error_buf, uint32 error_buf_size)
{
    return wasm_loader_load(buf, size, error_buf, error_buf_size);
}

WASMModule*
wasm_runtime_load_from_sections(WASMSection *section_list,
                                char *error_buf, uint32_t error_buf_size)
{
    return wasm_loader_load_from_sections(section_list,
                                          error_buf, error_buf_size);
}

void
wasm_runtime_unload(WASMModule *module)
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
                wasm_free(memories[i]);
            }
        wasm_free(memories);
  }
}

static WASMMemoryInstance*
memory_instantiate(uint32 init_page_count, uint32 max_page_count,
                   uint32 addr_data_size, uint32 global_data_size,
                   uint32 heap_size,
                   char *error_buf, uint32 error_buf_size)
{
    WASMMemoryInstance *memory;
    uint32 total_size = offsetof(WASMMemoryInstance, base_addr) +
        NumBytesPerPage * init_page_count +
        addr_data_size + global_data_size +
        heap_size;

    if (!(memory = wasm_malloc(total_size))) {
        set_error_buf(error_buf, error_buf_size,
                      "Instantiate memory failed: allocate memory failed.");
        return NULL;
    }

    memset(memory, 0, total_size);
    memory->cur_page_count = init_page_count;
    memory->max_page_count = max_page_count;
    memory->addr_data = memory->base_addr;
    memory->addr_data_size = addr_data_size;

    memory->memory_data = memory->addr_data + addr_data_size;

    memory->heap_data = memory->memory_data +
        NumBytesPerPage * memory->cur_page_count;;
    memory->heap_data_size = heap_size;

    memory->global_data = memory->heap_data + memory->heap_data_size;
    memory->global_data_size = global_data_size;

    memory->end_addr = memory->global_data + global_data_size;

    /* Initialize heap */
    if (!(memory->heap_handle = mem_allocator_create
                (memory->heap_data, memory->heap_data_size))) {
        wasm_free(memory);
        return NULL;
    }

    return memory;
}

/**
 * Instantiate memories in a module.
 */
static WASMMemoryInstance**
memories_instantiate(const WASMModule *module, uint32 addr_data_size,
                     uint32 global_data_size, uint32 heap_size,
                     char *error_buf, uint32 error_buf_size)
{
    WASMImport *import;
    uint32 mem_index = 0, i, memory_count =
        module->import_memory_count + module->memory_count;
    uint32 total_size;
    WASMMemoryInstance **memories, *memory;

    if (memory_count == 0 && global_data_size > 0)
        memory_count = 1;

    total_size = sizeof(WASMMemoryInstance*) * memory_count;
    memories = wasm_malloc(total_size);

    if (!memories) {
        set_error_buf(error_buf, error_buf_size,
                      "Instantiate memory failed: "
                      "allocate memory failed.");
        return NULL;
    }

    memset(memories, 0, total_size);

    /* instantiate memories from import section */
    import = module->import_memories;
    for (i = 0; i < module->import_memory_count; i++, import++) {
        if (!(memory = memories[mem_index++] =
                    memory_instantiate(import->u.memory.init_page_count,
                                       import->u.memory. max_page_count,
                                       addr_data_size, global_data_size,
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
                    memory_instantiate(module->memories[i].init_page_count,
                                       module->memories[i].max_page_count,
                                       addr_data_size, global_data_size,
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
                    memory_instantiate(0, 0, addr_data_size, global_data_size,
                                       heap_size, error_buf, error_buf_size))) {
            set_error_buf(error_buf, error_buf_size,
                          "Instantiate memory failed: "
                          "allocate memory failed.\n");
            memories_deinstantiate(memories, memory_count);
            return NULL;
        }
    }

    wasm_assert(mem_index == memory_count);
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
                wasm_free(tables[i]);
        wasm_free(tables);
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
    uint32 total_size = sizeof(WASMTableInstance*) * table_count;
    WASMTableInstance **tables = wasm_malloc(total_size), *table;

    if (!tables) {
        set_error_buf(error_buf, error_buf_size,
                      "Instantiate table failed: "
                      "allocate memory failed.");
        return NULL;
    }

    memset(tables, 0, total_size);

    /* instantiate tables from import section */
    import = module->import_tables;
    for (i = 0; i < module->import_table_count; i++, import++) {
        total_size = offsetof(WASMTableInstance, base_addr) +
                     sizeof(uint32) * import->u.table.init_size;
        if (!(table = tables[table_index++] = wasm_malloc(total_size))) {
            set_error_buf(error_buf, error_buf_size,
                          "Instantiate table failed: "
                          "allocate memory failed.");
            tables_deinstantiate(tables, table_count);
            return NULL;
        }

        memset(table, 0, total_size);
        table->cur_size = import->u.table.init_size;
        table->max_size = import->u.table.max_size;
    }

    /* instantiate tables from table section */
    for (i = 0; i < module->table_count; i++) {
        total_size = offsetof(WASMTableInstance, base_addr) +
                     sizeof(uint32) * module->tables[i].init_size;
        if (!(table = tables[table_index++] = wasm_malloc(total_size))) {
            set_error_buf(error_buf, error_buf_size,
                          "Instantiate table failed: "
                          "allocate memory failed.");
            tables_deinstantiate(tables, table_count);
            return NULL;
        }

        memset(table, 0, total_size);
        table->cur_size = module->tables[i].init_size;
        table->max_size = module->tables[i].max_size;
    }

    wasm_assert(table_index == table_count);
    return tables;
}

/**
 * Destroy function instances.
 */
static void
functions_deinstantiate(WASMFunctionInstance *functions, uint32 count)
{
    if (functions) {
        uint32 i;

        for (i = 0; i < count; i++)
            if (functions[i].local_offsets)
                wasm_free(functions[i].local_offsets);
        wasm_free(functions);
    }
}

static bool
function_init_local_offsets(WASMFunctionInstance *func)
{
    uint16 local_offset = 0;
    WASMType *param_type = func->u.func->func_type;
    uint32 param_count = param_type->param_count;
    uint8 *param_types = param_type->types;
    uint32 local_count = func->u.func->local_count;
    uint8 *local_types = func->u.func->local_types;
    uint32 i, total_size = (param_count + local_count) * sizeof(uint16);

    if (!(func->local_offsets = wasm_malloc(total_size)))
        return false;

    for (i = 0; i < param_count; i++) {
        func->local_offsets[i] = local_offset;
        local_offset += wasm_value_type_cell_num(param_types[i]);
    }

    for (i = 0; i < local_count; i++) {
        func->local_offsets[param_count + i] = local_offset;
        local_offset += wasm_value_type_cell_num(local_types[i]);
    }

    wasm_assert(local_offset == func->param_cell_num + func->local_cell_num);
    return true;
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
    uint32 total_size = sizeof(WASMFunctionInstance) * function_count;
    WASMFunctionInstance *functions = wasm_malloc(total_size), *function;

    if (!functions) {
        set_error_buf(error_buf, error_buf_size,
                      "Instantiate function failed: "
                      "allocate memory failed.");
        return NULL;
    }

    memset(functions, 0, total_size);

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

        function++;
    }

    /* instantiate functions from function section */
    for (i = 0; i < module->function_count; i++) {
        function->is_import_func = false;
        function->u.func = module->functions[i];

        function->param_cell_num =
            wasm_type_param_cell_num(function->u.func->func_type);
        function->ret_cell_num =
            wasm_type_return_cell_num(function->u.func->func_type);
        function->local_cell_num =
            wasm_get_cell_num(function->u.func->local_types,
                              function->u.func->local_count);

        if (!function_init_local_offsets(function)) {
            functions_deinstantiate(functions, function_count);
            return NULL;
        }

        function++;
    }

    wasm_assert((uint32)(function - functions) == function_count);
    return functions;
}

/**
 * Destroy global instances.
 */
static void
globals_deinstantiate(WASMGlobalInstance *globals)
{
    if (globals)
        wasm_free(globals);
}

/**
 * Instantiate globals in a module.
 */
static WASMGlobalInstance*
globals_instantiate(const WASMModule *module,
                    uint32 *p_addr_data_size,
                    uint32 *p_global_data_size,
                    char *error_buf, uint32 error_buf_size)
{
    WASMImport *import;
    uint32 addr_data_offset = 0, global_data_offset = 0;
    uint32 i, global_count =
        module->import_global_count + module->global_count;
    uint32 total_size = sizeof(WASMGlobalInstance) * global_count;
    WASMGlobalInstance *globals = wasm_malloc(total_size), *global;

    if (!globals) {
        set_error_buf(error_buf, error_buf_size,
                      "Instantiate global failed: "
                      "allocate memory failed.");
        return NULL;
    }

    memset(globals, 0, total_size);

    /* instantiate globals from import section */
    global = globals;
    import = module->import_globals;
    for (i = 0; i < module->import_global_count; i++, import++) {
        WASMGlobalImport *global_import = &import->u.global;
        global->type = global_import->type;
        global->is_mutable = global_import->is_mutable;
        global->is_addr = global_import->is_addr;
        global->initial_value = global_import->global_data_linked;
        global->data_offset = global_data_offset;
        global_data_offset += wasm_value_type_size(global->type);

        if (global->is_addr)
            addr_data_offset += sizeof(uint32);

        global++;
    }

    /* instantiate globals from global section */
    for (i = 0; i < module->global_count; i++) {
        global->type = module->globals[i].type;
        global->is_mutable = module->globals[i].is_mutable;
        global->is_addr = module->globals[i].is_addr;

        global->data_offset = global_data_offset;
        global_data_offset += wasm_value_type_size(global->type);

        if (global->is_addr)
            addr_data_offset += sizeof(uint32);

        global++;
    }

    wasm_assert((uint32)(global - globals) == global_count);
    *p_addr_data_size = addr_data_offset;
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
                global->initial_value.i32 =
                    NumBytesPerPage * module_inst->default_memory->cur_page_count;
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
            wasm_assert(init_expr->u.global_index < module->import_global_count);
            global->initial_value = globals[init_expr->u.global_index].initial_value;
        }
        else {
            memcpy(&global->initial_value, &init_expr->u, sizeof(int64));
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
        wasm_free(functions);
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
    uint32 i, total_size = sizeof(WASMExportFuncInstance) * export_func_count;

    if (!(export_func = export_funcs = wasm_malloc(total_size))) {
        set_error_buf(error_buf, error_buf_size,
                      "Instantiate export function failed: "
                      "allocate memory failed.");
        return NULL;
    }

    memset(export_funcs, 0, total_size);

    for (i = 0; i < module->export_count; i++, export++)
        if (export->kind == EXPORT_KIND_FUNC) {
            wasm_assert(export->index >= module->import_function_count
                        && export->index < module->import_function_count
                           + module->function_count);
            export_func->name = export->name;
            export_func->function = &module_inst->functions[export->index];
            export_func++;
        }

    wasm_assert((uint32)(export_func - export_funcs) == export_func_count);
    return export_funcs;
}

void
wasm_runtime_deinstantiate(WASMModuleInstance *module_inst);

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

    return wasm_runtime_call_wasm(module_inst, NULL, post_inst_func, 0, NULL);
}

static bool
execute_start_function(WASMModuleInstance *module_inst)
{
    WASMFunctionInstance *func = module_inst->start_function;

    if (!func)
        return true;

    wasm_assert(!func->is_import_func && func->param_cell_num == 0
            && func->ret_cell_num == 0);

    return wasm_runtime_call_wasm(module_inst, NULL, func, 0, NULL);
}

/**
 * Instantiate module
 */
WASMModuleInstance*
wasm_runtime_instantiate(WASMModule *module,
                         uint32 stack_size, uint32 heap_size,
                         char *error_buf, uint32 error_buf_size)
{
    WASMModuleInstance *module_inst;
    WASMTableSeg *table_seg;
    WASMDataSeg *data_seg;
    WASMGlobalInstance *globals = NULL, *global;
    uint32 global_count, addr_data_size = 0, global_data_size = 0, i;
    uint32 base_offset, length, memory_size;
    uint8 *global_data, *global_data_end, *addr_data, *addr_data_end;
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
        !(globals = globals_instantiate(module, &addr_data_size,
                                        &global_data_size,
                                        error_buf, error_buf_size)))
        return NULL;

    /* Allocate the memory */
    if (!(module_inst = wasm_malloc(sizeof(WASMModuleInstance)))) {
        set_error_buf(error_buf, error_buf_size,
                      "Instantiate module failed: allocate memory failed.");
        globals_deinstantiate(globals);
        return NULL;
    }

    memset(module_inst, 0, sizeof(WASMModuleInstance));
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
             memories_instantiate(module, addr_data_size, global_data_size,
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
        wasm_runtime_deinstantiate(module_inst);
        return NULL;
    }

    if (module_inst->memory_count || global_count > 0) {
        WASMMemoryInstance *memory;

        memory = module_inst->default_memory = module_inst->memories[0];
        memory_data = module_inst->default_memory->memory_data;

        /* fix import memoryBase */
        globals_instantiate_fix(globals, module, module_inst);

        /* Initialize the global data */
        addr_data = memory->addr_data;
        addr_data_end = addr_data + addr_data_size;
        global_data = memory->global_data;
        global_data_end = global_data + global_data_size;
        global = globals;
        for (i = 0; i < global_count; i++, global++) {
            switch (global->type) {
                case VALUE_TYPE_I32:
                case VALUE_TYPE_F32:
                    if (!global->is_addr)
                        *(int32*)global_data = global->initial_value.i32;
                    else {
                        *(int32*)addr_data = global->initial_value.i32;
                        /* Store the offset to memory data for global of addr */
                        *(int32*)global_data = addr_data - memory_data;
                        addr_data += sizeof(int32);
                    }
                    global_data += sizeof(int32);
                    break;
                case VALUE_TYPE_I64:
                case VALUE_TYPE_F64:
                    wasm_assert(!global->is_addr);
                    memcpy(global_data, &global->initial_value.i64, sizeof(int64));
                    global_data += sizeof(int64);
                    break;
                default:
                    wasm_assert(0);
            }
        }
        wasm_assert(addr_data == addr_data_end);
        wasm_assert(global_data == global_data_end);

        global = globals + module->import_global_count;
        for (i = 0; i < module->global_count; i++, global++) {
            InitializerExpression *init_expr = &module->globals[i].init_expr;

            if (init_expr->init_expr_type == INIT_EXPR_TYPE_GET_GLOBAL
                && globals[init_expr->u.global_index].is_addr) {
                uint8 *global_data_dst = memory->global_data + global->data_offset;
                uint8 *global_data_src =
                    memory->global_data + globals[init_expr->u.global_index].data_offset;
                *(uintptr_t*)global_data_dst = *(uintptr_t*)global_data_src;
            }
        }

        /* Initialize the memory data with data segment section */
        if (module_inst->default_memory->cur_page_count > 0) {
            for (i = 0; i < module->data_seg_count; i++) {
                data_seg = module->data_segments[i];
                wasm_assert(data_seg->memory_index == 0);
                wasm_assert(data_seg->base_offset.init_expr_type ==
                            INIT_EXPR_TYPE_I32_CONST
                            || data_seg->base_offset.init_expr_type ==
                            INIT_EXPR_TYPE_GET_GLOBAL);

                if (data_seg->base_offset.init_expr_type == INIT_EXPR_TYPE_GET_GLOBAL) {
                    wasm_assert(data_seg->base_offset.u.global_index < global_count
                                && globals[data_seg->base_offset.u.global_index].type ==
                                VALUE_TYPE_I32);
                    data_seg->base_offset.u.i32 =
                        globals[data_seg->base_offset.u.global_index].initial_value.i32;
                }

                base_offset = (uint32)data_seg->base_offset.u.i32;
                length = data_seg->data_length;
                memory_size = NumBytesPerPage * module_inst->default_memory->cur_page_count;

                if (base_offset >= memory_size
                    || base_offset + length > memory_size) {
                    set_error_buf(error_buf, error_buf_size,
                            "Instantiate module failed: data segment out of range.");
                    wasm_runtime_deinstantiate(module_inst);
                    return NULL;
                }

                memcpy(memory_data + base_offset, data_seg->data, length);
            }
        }
    }

    if (module_inst->table_count) {
        module_inst->default_table = module_inst->tables[0];

        /* Initialize the table data with table segment section */
        table_data = (uint32*)module_inst->default_table->base_addr;
        table_seg = module->table_segments;
        for (i = 0; i < module->table_seg_count; i++, table_seg++) {
            wasm_assert(table_seg->table_index == 0);
            wasm_assert(table_seg->base_offset.init_expr_type ==
                        INIT_EXPR_TYPE_I32_CONST
                        || table_seg->base_offset.init_expr_type ==
                        INIT_EXPR_TYPE_GET_GLOBAL);

            if (table_seg->base_offset.init_expr_type ==
                    INIT_EXPR_TYPE_GET_GLOBAL) {
                wasm_assert(table_seg->base_offset.u.global_index < global_count
                            && globals[table_seg->base_offset.u.global_index].type ==
                            VALUE_TYPE_I32);
                table_seg->base_offset.u.i32 =
                    globals[table_seg->base_offset.u.global_index].initial_value.i32;
            }
            if ((uint32)table_seg->base_offset.u.i32 <
                    module_inst->default_table->cur_size) {
                length = table_seg->function_count;
                if (table_seg->base_offset.u.i32 + length >
                        module_inst->default_table->cur_size)
                    length = module_inst->default_table->cur_size
                             - table_seg->base_offset.u.i32;
                memcpy(table_data + table_seg->base_offset.u.i32,
                       table_seg->func_indexes, length * sizeof(uint32));
            }
        }
    }

    if (module->start_function != (uint32)-1) {
        wasm_assert(module->start_function >= module->import_function_count);
        module_inst->start_function =
            &module_inst->functions[module->start_function];
    }

    module_inst->module = module;

    /* module instance type */
    module_inst->module_type = Wasm_Module_Bytecode;

    /* Initialize the thread related data */
    if (stack_size == 0)
        stack_size = DEFAULT_WASM_STACK_SIZE;
    module_inst->wasm_stack_size = stack_size;
    module_inst->main_tlr.module_inst = module_inst;

    /* Bind thread data with current native thread:
       set thread local root to current thread. */
    wasm_runtime_set_tlr(&module_inst->main_tlr);
    module_inst->main_tlr.handle = ws_self_thread();

    /* Execute __post_instantiate function */
    if (!execute_post_inst_function(module_inst)) {
        const char *exception = wasm_runtime_get_exception(module_inst);
        wasm_printf("%s\n", exception);
        wasm_runtime_deinstantiate(module_inst);
        return NULL;
    }

    /* Execute start function */
    if (!execute_start_function(module_inst)) {
        const char *exception = wasm_runtime_get_exception(module_inst);
        wasm_printf("%s\n", exception);
        wasm_runtime_deinstantiate(module_inst);
        return NULL;
    }

    (void)addr_data_end;
    (void)global_data_end;
    return module_inst;
}

void
wasm_runtime_deinstantiate(WASMModuleInstance *module_inst)
{
    if (!module_inst)
        return;

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

    if (module_inst->wasm_stack)
        wasm_free(module_inst->wasm_stack);

    wasm_free(module_inst);
}

bool
wasm_runtime_enlarge_memory(WASMModuleInstance *module, int inc_page_count)
{
#if 1
    wasm_runtime_set_exception(module, "unsupported operation: enlarge memory.");
    return false;
#else
    WASMMemoryInstance *memory = module->default_memory;
    WASMMemoryInstance *new_memory;
    uint32 total_page_count = inc_page_count + memory->cur_page_count;
    uint32 total_size = offsetof(WASMMemoryInstance, base_addr) +
                        memory->addr_data_size +
                        NumBytesPerPage * total_page_count +
                        memory->global_data_size +
                        memory->thunk_argv_data_size +
                        sizeof(uint32) * memory->thunk_argc;

    if (!(new_memory = wasm_malloc(total_size))) {
        wasm_runtime_set_exception(module, "alloc memory for enlarge memory failed.");
        return false;
    }

    new_memory->cur_page_count = total_page_count;
    new_memory->max_page_count = memory->max_page_count > total_page_count
                                 ? memory->max_page_count : total_page_count;
    new_memory->addr_data = new_memory->base_addr;
    new_memory->addr_data_size = memory->addr_data_size;

    new_memory->thunk_argv_data = new_memory->addr_data + memory->addr_data_size;
    new_memory->thunk_argv_data_size = memory->thunk_argv_data_size;
    new_memory->thunk_argc = memory->thunk_argc;
    new_memory->thunk_argv_offsets = new_memory->thunk_argv_data +
                                     memory->thunk_argv_data_size;

    new_memory->memory_data = new_memory->thunk_argv_offsets +
                              sizeof(uint32) * memory->thunk_argc;
    new_memory->global_data = new_memory->memory_data +
                              NumBytesPerPage * new_memory->cur_page_count;
    new_memory->global_data_size = memory->global_data_size;

    new_memory->end_addr = new_memory->global_data + memory->global_data_size;

    /* Copy addr data, thunk argv data, thunk argv offsets and memory data */
    memcpy(new_memory->addr_data, memory->addr_data,
           memory->global_data - memory->addr_data);
    /* Copy global data */
    memcpy(new_memory->global_data, memory->global_data,
            memory->end_addr - memory->global_data);
    /* Init free space of new memory */
    memset(new_memory->memory_data + NumBytesPerPage * memory->cur_page_count,
            0, NumBytesPerPage * (total_page_count - memory->cur_page_count));

    wasm_free(memory);
    module->memories[0] = module->default_memory = new_memory;
    return true;
#endif
}

PackageType
get_package_type(const uint8 *buf, uint32 size)
{
    if (buf && size > 4) {
        if (buf[0] == '\0' && buf[1] == 'a' && buf[2] == 's' && buf[3] == 'm')
            return Wasm_Module_Bytecode;
        if (buf[0] == '\0' && buf[1] == 'a' && buf[2] == 'o' && buf[3] == 't')
            return Wasm_Module_AoT;
    }
    return Package_Type_Unknown;
}

WASMExecEnv*
wasm_runtime_create_exec_env(uint32 stack_size)
{
    WASMExecEnv *exec_env = wasm_malloc(sizeof(WASMExecEnv));
    if (exec_env) {
        if (!(exec_env->stack = wasm_malloc(stack_size))) {
            wasm_free(exec_env);
            return NULL;
        }
        exec_env->stack_size = stack_size;
    }
    return exec_env;
}

void
wasm_runtime_destory_exec_env(WASMExecEnv *env)
{
    if (env) {
        wasm_free(env->stack);
        wasm_free(env);
    }
}

bool
wasm_runtime_attach_current_thread(WASMModuleInstance *module_inst,
                                   void *thread_data)
{
    wasm_runtime_set_tlr(&module_inst->main_tlr);
    module_inst->main_tlr.handle = ws_self_thread();
    module_inst->thread_data = thread_data;
    return true;
}

void
wasm_runtime_detach_current_thread(WASMModuleInstance *module_inst)
{
    module_inst->thread_data = NULL;
}

void*
wasm_runtime_get_current_thread_data()
{
    WASMThread *tlr = wasm_runtime_get_self();
    return (tlr && tlr->module_inst) ? tlr->module_inst->thread_data : NULL;
}

WASMModuleInstance *
wasm_runtime_get_current_module_inst()
{
    WASMThread *tlr = wasm_runtime_get_self();
    return tlr ? tlr->module_inst : NULL;
}

int32
wasm_runtime_module_malloc(WASMModuleInstance *module_inst, uint32 size)
{
    uint8 *memory_base = module_inst->default_memory->memory_data;
    void *heap = module_inst->default_memory->heap_handle;
    uint8 *addr = mem_allocator_malloc(heap, size);
    if (!addr)
        wasm_runtime_set_exception(module_inst, "out of memory");
    return addr ? addr - memory_base : 0;
}

void
wasm_runtime_module_free(WASMModuleInstance *module_inst, int32 ptr)
{
    uint8 *memory_base = module_inst->default_memory->memory_data;
    uint8 *heap_base = module_inst->default_memory->heap_data;
    uint32 heap_size = module_inst->default_memory->heap_data_size;
    void *heap = module_inst->default_memory->heap_handle;
    uint8 *addr = ptr ? memory_base + ptr : NULL;
    if (addr && (heap_base < addr && addr < heap_base + heap_size))
        mem_allocator_free(heap, addr);
}

    int32
wasm_runtime_module_dup_data(WASMModuleInstance *module_inst,
        const char *src, uint32 size)
{
    int32 buffer_offset = wasm_runtime_module_malloc(module_inst, size);
    if (buffer_offset != 0) {
        char *buffer;
        buffer = wasm_runtime_addr_app_to_native(module_inst, buffer_offset);
        memcpy(buffer, src, size);
    }
    return buffer_offset;
}

bool
wasm_runtime_validate_app_addr(WASMModuleInstance *module_inst,
                               int32 app_offset, uint32 size)
{
    /* integer overflow check */
    if(app_offset < 0 ||
       app_offset + size < size) {
        wasm_runtime_set_exception(module_inst, "out of bounds memory access");
        return false;
    }

    uint8 *memory_base = module_inst->default_memory->memory_data;
    uint8 *addr = memory_base + app_offset;
    uint8 *base_addr = module_inst->default_memory->base_addr;
    uint8 *end_addr = module_inst->default_memory->end_addr;
    bool ret = (base_addr <= addr
                && addr + size <= end_addr);
    if (!ret)
        wasm_runtime_set_exception(module_inst, "out of bounds memory access");
    return ret;
}

bool
wasm_runtime_validate_native_addr(WASMModuleInstance *module_inst,
                                  void *native_ptr, uint32 size)
{
    uint8 *addr = native_ptr;
    uint8 *base_addr = module_inst->default_memory->base_addr;
    uint8 *end_addr = module_inst->default_memory->end_addr;
    bool ret = (base_addr <= addr && addr + size <= end_addr);
    if (!ret || (addr + size < addr)/* integer overflow */)
        wasm_runtime_set_exception(module_inst, "out of bounds memory access");
    return ret;
}

void *
wasm_runtime_addr_app_to_native(WASMModuleInstance *module_inst,
                                int32 app_offset)
{
    return module_inst->default_memory->memory_data + app_offset;
}

int32
wasm_runtime_addr_native_to_app(WASMModuleInstance *module_inst,
                                void *native_ptr)
{
    return (uint8*)native_ptr - module_inst->default_memory->memory_data;
}

uint32
wasm_runtime_get_temp_ret(WASMModuleInstance *module_inst)
{
    return module_inst->temp_ret;
}

void
wasm_runtime_set_temp_ret(WASMModuleInstance *module_inst,
                          uint32 temp_ret)
{
    module_inst->temp_ret = temp_ret;
}

uint32
wasm_runtime_get_llvm_stack(WASMModuleInstance *module_inst)
{
    return module_inst->llvm_stack;
}

void
wasm_runtime_set_llvm_stack(WASMModuleInstance *module_inst,
                            uint32 llvm_stack)
{
    module_inst->llvm_stack = llvm_stack;
}

WASMModuleInstance*
wasm_runtime_load_aot(uint8 *aot_file, uint32 aot_file_size,
                      uint32 heap_size,
                      char *error_buf, uint32 error_buf_size)
{
    (void)aot_file;
    (void)aot_file_size;
    (void)heap_size;
    (void)error_buf;
    (void)error_buf_size;
    return NULL;
}

