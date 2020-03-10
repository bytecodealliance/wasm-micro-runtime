/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "aot.h"


static char aot_error[128];

char*
aot_get_last_error()
{
  return aot_error[0] == '\0' ? "" : aot_error;
}

void
aot_set_last_error(const char *error)
{
  if (error)
    snprintf(aot_error, sizeof(aot_error), "Error: %s", error);
  else
    aot_error[0] = '\0';
}

static void
aot_destroy_mem_init_data_list(AOTMemInitData **data_list, uint32 count)
{
  uint32 i;
  for (i = 0; i < count; i++)
    if (data_list[i])
      wasm_runtime_free(data_list[i]);
  wasm_runtime_free(data_list);
}

static AOTMemInitData **
aot_create_mem_init_data_list(const WASMModule *module)
{
  AOTMemInitData **data_list;
  uint64 size;
  uint32 i;

  /* Allocate memory */
  size = sizeof(AOTMemInitData *) * (uint64)module->data_seg_count;
  if (size >= UINT32_MAX
      || !(data_list = wasm_runtime_malloc((uint32)size))) {
    aot_set_last_error("allocate memory failed.");
    return NULL;
  }

  memset(data_list, 0, size);

  /* Create each memory data segment */
  for (i = 0; i < module->data_seg_count; i++) {
    size = offsetof(AOTMemInitData, bytes) +
                (uint64)module->data_segments[i]->data_length;
    if (size >= UINT32_MAX
        || !(data_list[i] = wasm_runtime_malloc((uint32)size))) {
      aot_set_last_error("allocate memory failed.");
      goto fail;
    }

    data_list[i]->offset = module->data_segments[i]->base_offset;
    data_list[i]->byte_count = module->data_segments[i]->data_length;
    memcpy(data_list[i]->bytes, module->data_segments[i]->data,
           module->data_segments[i]->data_length);
  }

  return data_list;

fail:
  aot_destroy_mem_init_data_list(data_list, module->data_seg_count);
  return NULL;
}

static void
aot_destroy_table_init_data_list(AOTTableInitData **data_list, uint32 count)
{
  uint32 i;
  for (i = 0; i < count; i++)
    if (data_list[i])
      wasm_runtime_free(data_list[i]);
  wasm_runtime_free(data_list);
}

static AOTTableInitData **
aot_create_table_init_data_list(const WASMModule *module)
{
  AOTTableInitData **data_list;
  uint64 size;
  uint32 i;

  /* Allocate memory */
  size = sizeof(AOTTableInitData *) * (uint64)module->table_seg_count;
  if (size >= UINT32_MAX
      || !(data_list = wasm_runtime_malloc((uint32)size))) {
    aot_set_last_error("allocate memory failed.");
    return NULL;
  }

  memset(data_list, 0, size);

  /* Create each table data segment */
  for (i = 0; i < module->table_seg_count; i++) {
    size = offsetof(AOTTableInitData, func_indexes) +
           sizeof(uint32) * (uint64)module->table_segments[i].function_count;
    if (size >= UINT32_MAX
        || !(data_list[i] = wasm_runtime_malloc((uint32)size))) {
      aot_set_last_error("allocate memory failed.");
      goto fail;
    }

    data_list[i]->offset = module->table_segments[i].base_offset;
    data_list[i]->func_index_count = module->table_segments[i].function_count;
    memcpy(data_list[i]->func_indexes, module->table_segments[i].func_indexes,
           sizeof(uint32) * module->table_segments[i].function_count);
  }

  return data_list;

fail:
  aot_destroy_table_init_data_list(data_list, module->table_seg_count);
  return NULL;
}

static AOTImportGlobal *
aot_create_import_globals(const WASMModule *module,
                          uint32 *p_import_global_data_size)
{
  AOTImportGlobal *import_globals;
  uint64 size;
  uint32 i, data_offset = 0;

  /* Allocate memory */
  size = sizeof(AOTImportGlobal) * (uint64)module->import_global_count;
  if (size >= UINT32_MAX
      || !(import_globals = wasm_runtime_malloc((uint32)size))) {
    aot_set_last_error("allocate memory failed.");
    return NULL;
  }

  memset(import_globals, 0, (uint32)size);

  /* Create each import global */
  for (i = 0; i < module->import_global_count; i++) {
    WASMGlobalImport *import_global = &module->import_globals[i].u.global;
    import_globals[i].module_name = import_global->module_name;
    import_globals[i].global_name = import_global->field_name;
    import_globals[i].type = import_global->type;
    import_globals[i].is_mutable = import_global->is_mutable;
    import_globals[i].global_data_linked = import_global->global_data_linked;
    import_globals[i].size = wasm_value_type_size(import_global->type);
    /* Calculate data offset */
    import_globals[i].data_offset = data_offset;
    data_offset += wasm_value_type_size(import_global->type);
  }

  *p_import_global_data_size = data_offset;
  return import_globals;
}

static AOTGlobal *
aot_create_globals(const WASMModule *module,
                   uint32 global_data_start_offset,
                   uint32 *p_global_data_size)
{
  AOTGlobal *globals;
  uint64 size;
  uint32 i, data_offset = global_data_start_offset;

  /* Allocate memory */
  size = sizeof(AOTGlobal) * (uint64)module->global_count;
  if (size >= UINT32_MAX
      || !(globals = wasm_runtime_malloc((uint32)size))) {
    aot_set_last_error("allocate memory failed.");
    return NULL;
  }

  memset(globals, 0, (uint32)size);

  /* Create each global */
  for (i = 0; i < module->global_count; i++) {
    WASMGlobal *global = &module->globals[i];
    globals[i].type = global->type;
    globals[i].is_mutable = global->is_mutable;
    globals[i].size = wasm_value_type_size(global->type);
    memcpy(&globals[i].init_expr, &global->init_expr,
           sizeof(global->init_expr));
    /* Calculate data offset */
    globals[i].data_offset = data_offset;
    data_offset += wasm_value_type_size(global->type);
  }

  *p_global_data_size = data_offset - global_data_start_offset;
  return globals;
}

static void
aot_destroy_func_types(AOTFuncType **func_types, uint32 count)
{
  uint32 i;
  for (i = 0; i < count; i++)
    if (func_types[i])
      wasm_runtime_free(func_types[i]);
  wasm_runtime_free(func_types);
}

static AOTFuncType **
aot_create_func_types(const WASMModule *module)
{
  AOTFuncType **func_types;
  uint64 size;
  uint32 i;

  /* Allocate memory */
  size = sizeof(AOTFuncType*) * (uint64)module->type_count;
  if (size >= UINT32_MAX
      || !(func_types = wasm_runtime_malloc((uint32)size))) {
    aot_set_last_error("allocate memory failed.");
    return NULL;
  }

  memset(func_types, 0, size);

  /* Create each function type */
  for (i = 0; i < module->type_count; i++) {
    size = offsetof(AOTFuncType, types) +
           (uint64)module->types[i]->param_count +
           (uint64)module->types[i]->result_count;
    if (size >= UINT32_MAX
        || !(func_types[i] = wasm_runtime_malloc((uint32)size))) {
      aot_set_last_error("allocate memory failed.");
      goto fail;
    }
    memcpy(func_types[i], module->types[i], size);
  }

  return func_types;

fail:
  aot_destroy_func_types(func_types, module->type_count);
  return NULL;
}

static AOTImportFunc *
aot_create_import_funcs(const WASMModule *module)
{
  AOTImportFunc *import_funcs;
  uint64 size;
  uint32 i, j;

  /* Allocate memory */
  size = sizeof(AOTImportFunc) * (uint64)module->import_function_count;
  if (size >= UINT32_MAX
      || !(import_funcs = wasm_runtime_malloc((uint32)size))) {
    aot_set_last_error("allocate memory failed.");
    return NULL;
  }

  /* Create each import function */
  for (i = 0; i < module->import_function_count; i++) {
    WASMFunctionImport *import_func = &module->import_functions[i].u.function;
    import_funcs[i].module_name = import_func->module_name;
    import_funcs[i].func_name = import_func->field_name;
    import_funcs[i].func_ptr_linked = import_func->func_ptr_linked;
    import_funcs[i].func_type = import_func->func_type;
    import_funcs[i].signature = import_func->signature;
    /* Resolve function type index */
    for (j = 0; j < module->type_count; j++)
      if (import_func->func_type == module->types[j]) {
        import_funcs[i].func_type_index = j;
        break;
      }
  }

  return import_funcs;
}

static void
aot_destroy_funcs(AOTFunc **funcs, uint32 count)
{
  uint32 i;

  for (i = 0; i < count; i++)
    if (funcs[i])
      wasm_runtime_free(funcs[i]);
  wasm_runtime_free(funcs);
}

static AOTFunc **
aot_create_funcs(const WASMModule *module)
{
  AOTFunc **funcs;
  uint64 size;
  uint32 i, j;

  /* Allocate memory */
  size = sizeof(AOTFunc*) * (uint64)module->function_count;
  if (size >= UINT32_MAX
      || !(funcs = wasm_runtime_malloc((uint32)size))) {
    aot_set_last_error("allocate memory failed.");
    return NULL;
  }

  memset(funcs, 0, size);

  /* Create each function */
  for (i = 0; i < module->function_count; i++) {
    WASMFunction *func = module->functions[i];
    size = sizeof (AOTFunc);
    if (!(funcs[i] = wasm_runtime_malloc((uint32)size))) {
      aot_set_last_error("allocate memory failed.");
      goto fail;
    }

    funcs[i]->func_type = func->func_type;

    /* Resolve function type index */
    for (j = 0; j < module->type_count; j++)
      if (func->func_type == module->types[j]) {
        funcs[i]->func_type_index = j;
        break;
      }

    /* Resolve local variable info and code info */
    funcs[i]->local_count = func->local_count;
    funcs[i]->local_types = func->local_types;
    funcs[i]->code = func->code;
    funcs[i]->code_size = func->code_size;
  }

  return funcs;

fail:
  aot_destroy_funcs(funcs, module->function_count);
  return NULL;
}

static AOTExportFunc *
aot_create_export_funcs(const WASMModule *module,
                        uint32 export_func_count)
{
  AOTExportFunc *export_funcs;
  uint64 size;
  uint32 i, j = 0;

  /* Allocate memory */
  size = sizeof(AOTExportFunc) * (uint64)export_func_count;
  if (size >= UINT32_MAX
      || !(export_funcs = wasm_runtime_malloc((uint32)size))) {
    aot_set_last_error("allocate memory failed.");
    return NULL;
  }

  /* Create each export function */
  for (i = 0; i < module->export_count; i++) {
    if (module->exports[i].kind == EXPORT_KIND_FUNC) {
      export_funcs[j].func_name = module->exports[i].name;
      export_funcs[j].func_index = module->exports[i].index;
      export_funcs[j].func_type =
          module->functions[module->exports[i].index
                            - module->import_function_count]->func_type;
      /* Function pointer to be linked in JIT mode */
      export_funcs[j].func_ptr = NULL;
      j++;
    }
  }
  return export_funcs;
}

AOTCompData*
aot_create_comp_data(WASMModule *module)
{
  AOTCompData *comp_data;
  uint32 import_global_data_size = 0, global_data_size = 0, i;

  /* Allocate memory */
  if (!(comp_data = wasm_runtime_malloc(sizeof(AOTCompData)))) {
    aot_set_last_error("create compile data failed.\n");
    return NULL;
  }

  memset(comp_data, 0, sizeof(AOTCompData));

  /* Set memory page count */
  if (module->import_memory_count) {
    comp_data->num_bytes_per_page =
      module->import_memories[0].u.memory.num_bytes_per_page;
    comp_data->mem_init_page_count =
      module->import_memories[0].u.memory.init_page_count;
    comp_data->mem_max_page_count =
      module->import_memories[0].u.memory.max_page_count;
  }
  else if (module->memory_count) {
    comp_data->num_bytes_per_page =
      module->memories[0].num_bytes_per_page;
    comp_data->mem_init_page_count =
      module->memories[0].init_page_count;
    comp_data->mem_max_page_count =
      module->memories[0].max_page_count;
  }

  /* Create memory data segments */
  comp_data->mem_init_data_count = module->data_seg_count;
  if (comp_data->mem_init_data_count > 0
      && !(comp_data->mem_init_data_list =
            aot_create_mem_init_data_list(module)))
    goto fail;

  /* Set table size */
  if (module->import_table_count)
    comp_data->table_size = module->import_tables[0].u.table.init_size;
  else if (module->table_count)
    comp_data->table_size = module->tables[0].init_size;

  /* Create table data segments */
  comp_data->table_init_data_count = module->table_seg_count;
  if (comp_data->table_init_data_count > 0
      && !(comp_data->table_init_data_list =
            aot_create_table_init_data_list(module)))
    goto fail;

  /* Create import globals */
  comp_data->import_global_count = module->import_global_count;
  if (comp_data->import_global_count > 0
      && !(comp_data->import_globals =
        aot_create_import_globals(module, &import_global_data_size)))
    goto fail;

  /* Create globals */
  comp_data->global_count = module->global_count;
  if (comp_data->global_count
      && !(comp_data->globals = aot_create_globals
            (module, import_global_data_size, &global_data_size)))
    goto fail;

  comp_data->global_data_size = import_global_data_size +
                                global_data_size;

  /* Create function types */
  comp_data->func_type_count = module->type_count;
  if (comp_data->func_type_count
      && !(comp_data->func_types = aot_create_func_types(module)))
    goto fail;

  /* Create import functions */
  comp_data->import_func_count = module->import_function_count;
  if (comp_data->import_func_count
      && !(comp_data->import_funcs = aot_create_import_funcs(module)))
    goto fail;

  /* Create functions */
  comp_data->func_count = module->function_count;
  if (comp_data->func_count
      && !(comp_data->funcs = aot_create_funcs(module)))
    goto fail;

  /* Create export functions */
  for (i = 0; i < module->export_count; i++)
    if (module->exports[i].kind == EXPORT_KIND_FUNC)
      comp_data->export_func_count++;

  if (comp_data->export_func_count
      && !(comp_data->export_funcs = aot_create_export_funcs
            (module, comp_data->export_func_count)))
    goto fail;

  comp_data->start_func_index = module->start_function;
  comp_data->wasm_module = module;

  return comp_data;

fail:

  aot_destroy_comp_data(comp_data);
  return NULL;
}

void
aot_destroy_comp_data(AOTCompData *comp_data)
{
  if (!comp_data)
    return;

  if (comp_data->mem_init_data_list)
    aot_destroy_mem_init_data_list(comp_data->mem_init_data_list,
                                   comp_data->mem_init_data_count);

  if (comp_data->table_init_data_list)
    aot_destroy_table_init_data_list(comp_data->table_init_data_list,
                                     comp_data->table_init_data_count);

  if (comp_data->import_globals)
    wasm_runtime_free(comp_data->import_globals);

  if (comp_data->globals)
    wasm_runtime_free(comp_data->globals);

  if (comp_data->func_types)
    aot_destroy_func_types(comp_data->func_types,
                           comp_data->func_type_count);

  if (comp_data->import_funcs)
    wasm_runtime_free(comp_data->import_funcs);

  if (comp_data->funcs)
    aot_destroy_funcs(comp_data->funcs, comp_data->func_count);

  if (comp_data->export_funcs)
    wasm_runtime_free(comp_data->export_funcs);

  wasm_runtime_free(comp_data);
}

