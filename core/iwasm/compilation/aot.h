/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _AOT_H_
#define _AOT_H_

#include "bh_platform.h"
#include "bh_assert.h"
#include "../common/wasm_runtime_common.h"
#include "../interpreter/wasm.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AOT_FUNC_PREFIX "aot_func#"

typedef InitializerExpression AOTInitExpr;
typedef WASMType AOTFuncType;
typedef WASMExport AOTExport;

/**
 * Import memory
 */
typedef struct AOTImportMemory {
  char *module_name;
  char *memory_name;
  uint32 memory_flags;
  uint32 num_bytes_per_page;
  uint32 mem_init_page_count;
  uint32 mem_max_page_count;
} AOTImportMemory;

/**
 * Memory information
 */
typedef struct AOTMemory {
  /* memory info */
  uint32 memory_flags;
  uint32 num_bytes_per_page;
  uint32 mem_init_page_count;
  uint32 mem_max_page_count;
} AOTMemory;

/**
 * A segment of memory init data
 */
typedef struct AOTMemInitData {
#if WASM_ENABLE_BULK_MEMORY != 0
  /* Passive flag */
  bool is_passive;
  /* memory index */
  uint32 memory_index;
#endif
  /* Start address of init data */
  AOTInitExpr offset;
  /* Byte count */
  uint32 byte_count;
  /* Byte array */
  uint8 bytes[1];
} AOTMemInitData;

/**
 * Import table
 */
typedef struct AOTImportTable {
  char *module_name;
  char *table_name;
  uint32 table_flags;
  uint32 table_init_size;
  uint32 table_max_size;
} AOTImportTable;

/**
 * Table
 */
typedef struct AOTTable {
  uint32 elem_type;
  uint32 table_flags;
  uint32 table_init_size;
  uint32 table_max_size;
} AOTTable;

/**
 * A segment of table init data
 */
typedef struct AOTTableInitData {
  uint32 table_index;
  /* Start address of init data */
  AOTInitExpr offset;
  /* Function index count */
  uint32 func_index_count;
  /* Function index array */
  uint32 func_indexes[1];
} AOTTableInitData;

/**
 * Import global variable
 */
typedef struct AOTImportGlobal {
  char *module_name;
  char *global_name;
  /* VALUE_TYPE_I32/I64/F32/F64 */
  uint8 type;
  bool is_mutable;
  uint32 size;
  /* The data offset of current global in global data */
  uint32 data_offset;
  /* global data after linked */
  WASMValue global_data_linked;
} AOTImportGlobal;

/**
 * Global variable
 */
typedef struct AOTGlobal {
  /* VALUE_TYPE_I32/I64/F32/F64 */
  uint8 type;
  bool is_mutable;
  uint32 size;
  /* The data offset of current global in global data */
  uint32 data_offset;
  AOTInitExpr init_expr;
} AOTGlobal;

/**
 * Import function
 */
typedef struct AOTImportFunc {
  char *module_name;
  char *func_name;
  AOTFuncType *func_type;
  uint32 func_type_index;
  /* function pointer after linked */
  void *func_ptr_linked;
  /* signature from registered native symbols */
  const char *signature;
  /* attachment */
  void *attachment;
  bool call_conv_raw;
} AOTImportFunc;

/**
 * Function
 */
typedef struct AOTFunc {
  AOTFuncType *func_type;
  uint32 func_type_index;
  uint32 local_count;
  uint8 *local_types;
  uint16 param_cell_num;
  uint16 local_cell_num;
  uint32 code_size;
  uint8 *code;
} AOTFunc;

typedef struct AOTCompData {
  /* Import memories */
  uint32 import_memory_count;
  AOTImportMemory *import_memories;

  /* Memories */
  uint32 memory_count;
  AOTMemory *memories;

  /* Memory init data info */
  uint32 mem_init_data_count;
  AOTMemInitData **mem_init_data_list;

  /* Import tables */
  uint32 import_table_count;
  AOTImportTable *import_tables;

  /* Tables */
  uint32 table_count;
  AOTTable *tables;

  /* Table init data info */
  uint32 table_init_data_count;
  AOTTableInitData **table_init_data_list;

  /* Import globals */
  uint32 import_global_count;
  AOTImportGlobal *import_globals;

  /* Globals */
  uint32 global_count;
  AOTGlobal *globals;

  /* Function types */
  uint32 func_type_count;
  AOTFuncType **func_types;

  /* Import functions */
  uint32 import_func_count;
  AOTImportFunc *import_funcs;

  /* Functions */
  uint32 func_count;
  AOTFunc **funcs;

  uint32 global_data_size;

  uint32 start_func_index;
  uint32 malloc_func_index;
  uint32 free_func_index;

  uint32 aux_data_end_global_index;
  uint32 aux_data_end;
  uint32 aux_heap_base_global_index;
  uint32 aux_heap_base;
  uint32 aux_stack_top_global_index;
  uint32 aux_stack_bottom;
  uint32 aux_stack_size;

  WASMModule *wasm_module;
} AOTCompData;

AOTCompData*
aot_create_comp_data(WASMModule *module);

void
aot_destroy_comp_data(AOTCompData *comp_data);

char*
aot_get_last_error();

void
aot_set_last_error(const char *error);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _AOT_H_ */

