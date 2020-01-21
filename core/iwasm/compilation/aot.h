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

/**
 * A segment of memory init data
 */
typedef struct AOTMemInitData {
  /* Start address of init data */
  AOTInitExpr offset;
  /* Byte count */
  uint32 byte_count;
  /* Byte array */
  uint8 bytes[1];
} AOTMemInitData;

/**
 * A segment of table init data
 */
typedef struct AOTTableInitData {
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
} AOTImportFunc;

/**
 * Function
 */
typedef struct AOTFunc {
  AOTFuncType *func_type;
  uint32 func_type_index;
  uint32 local_count;
  uint8 *local_types;
  uint32 code_size;
  uint8 *code;
} AOTFunc;

/**
 * Export function
 */
typedef struct AOTExportFunc {
  char *func_name;
  AOTFuncType *func_type;
  /* function pointer linked */
  void *func_ptr;
  uint32 func_index;
} AOTExportFunc;

typedef struct AOTCompData {
  /* Memory and memory init data info */
  uint32 mem_init_page_count;
  uint32 mem_max_page_count;
  uint32 mem_init_data_count;
  AOTMemInitData **mem_init_data_list;

  /* Table and table init data info */
  uint32 table_size;
  AOTTableInitData **table_init_data_list;
  uint32 table_init_data_count;

  AOTImportGlobal *import_globals;
  uint32 import_global_count;

  AOTGlobal *globals;
  uint32 global_count;

  AOTFuncType **func_types;
  uint32 func_type_count;

  AOTImportFunc *import_funcs;
  uint32 import_func_count;

  AOTFunc **funcs;
  uint32 func_count;

  AOTExportFunc *export_funcs;
  uint32 export_func_count;

  uint32 start_func_index;
  uint32 addr_data_size;
  uint32 global_data_size;

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

