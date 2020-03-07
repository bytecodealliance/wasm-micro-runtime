/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WASM_RUNTIME_H
#define _WASM_RUNTIME_H

#include "wasm.h"
#include "bh_hashmap.h"
#include "../common/wasm_runtime_common.h"
#include "../common/wasm_exec_env.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WASMMemoryInstance {
    /* Number bytes per page */
    uint32 num_bytes_per_page;
    /* Current page count */
    uint32 cur_page_count;
    /* Maximum page count */
    uint32 max_page_count;

    /* Heap data base address */
    uint8 *heap_data;
    /* Heap data end address */
    uint8 *heap_data_end;
    /* The heap created */
    void *heap_handle;
    /* Heap base offset of wasm app */
    int32 heap_base_offset;

    /* Memory data */
    uint8 *memory_data;
    /* Global data of global instances */
    uint8 *global_data;
    uint32 global_data_size;

    /* End address of memory */
    uint8 *end_addr;

    /* Base address, the layout is:
       thunk_argv data + thunk arg offsets +
       memory data + global data
       memory data init size is: num_bytes_per_page * cur_page_count
       global data size is calculated in module instantiating
       Note: when memory is re-allocated, the thunk argv data, thunk
             argv offsets and memory data must be copied to new memory also.
     */
    uint8 base_addr[1];
} WASMMemoryInstance;

typedef struct WASMTableInstance {
    /* The element type, TABLE_ELEM_TYPE_ANY_FUNC currently */
    uint8 elem_type;
    /* Current size */
    uint32 cur_size;
    /* Maximum size */
    uint32 max_size;
    /* Base address */
    uint8 base_addr[1];
} WASMTableInstance;

typedef struct WASMGlobalInstance {
    /* value type, VALUE_TYPE_I32/I64/F32/F64 */
    uint8 type;
    /* mutable or constant */
    bool is_mutable;
    /* data offset to base_addr of WASMMemoryInstance */
    uint32 data_offset;
    /* initial value */
    WASMValue initial_value;
} WASMGlobalInstance;

typedef struct WASMFunctionInstance {
    /* whether it is import function or WASM function */
    bool is_import_func;
    /* parameter count */
    uint16 param_count;
    /* local variable count, 0 for import function */
    uint16 local_count;
    /* cell num of parameters */
    uint16 param_cell_num;
    /* cell num of return type */
    uint16 ret_cell_num;
    /* cell num of local variables, 0 for import function */
    uint16 local_cell_num;
#if WASM_ENABLE_FAST_INTERP != 0
    /* cell num of consts */
    uint16 const_cell_num;
#endif
    uint16 *local_offsets;
    /* parameter types */
    uint8 *param_types;
    /* local types, NULL for import function */
    uint8 *local_types;
    union {
        WASMFunctionImport *func_import;
        WASMFunction *func;
    } u;
} WASMFunctionInstance;

typedef struct WASMExportFuncInstance {
    char *name;
    WASMFunctionInstance *function;
} WASMExportFuncInstance;

typedef struct WASMModuleInstance {
    /* Module instance type, for module instance loaded from
       WASM bytecode binary, this field is Wasm_Module_Bytecode;
       for module instance loaded from AOT file, this field is
       Wasm_Module_AoT, and this structure should be treated as
       AOTModuleInstance structure. */
    uint32 module_type;

    uint32 memory_count;
    uint32 table_count;
    uint32 global_count;
    uint32 function_count;
    uint32 export_func_count;

    WASMMemoryInstance **memories;
    WASMTableInstance **tables;
    WASMGlobalInstance *globals;
    WASMFunctionInstance *functions;
    WASMExportFuncInstance *export_functions;

    WASMMemoryInstance *default_memory;
    WASMTableInstance *default_table;

    WASMFunctionInstance *start_function;

    WASMModule *module;

#if WASM_ENABLE_LIBC_WASI != 0
    WASIContext *wasi_ctx;
#endif

    uint32 DYNAMICTOP_PTR_offset;
    uint32 temp_ret;
    uint32 llvm_stack;

    /* Default WASM stack size of threads of this Module instance. */
    uint32 default_wasm_stack_size;

    /* The exception buffer of wasm interpreter for current thread. */
    char cur_exception[128];

    /* The custom data that can be set/get by
     * wasm_set_custom_data/wasm_get_custom_data */
    void *custom_data;

    /* Main exec env */
    WASMExecEnv *main_exec_env;
} WASMModuleInstance;

struct WASMInterpFrame;
typedef struct WASMInterpFrame WASMRuntimeFrame;

/**
 * Return the code block of a function.
 *
 * @param func the WASM function instance
 *
 * @return the code block of the function
 */
static inline uint8*
wasm_get_func_code(WASMFunctionInstance *func)
{
#if WASM_ENABLE_FAST_INTERP == 0
    return func->is_import_func ? NULL : func->u.func->code;
#else
    return func->is_import_func ? NULL : func->u.func->code_compiled;
#endif
}

/**
 * Return the code block end of a function.
 *
 * @param func the WASM function instance
 *
 * @return the code block end of the function
 */
static inline uint8*
wasm_get_func_code_end(WASMFunctionInstance *func)
{
#if WASM_ENABLE_FAST_INTERP == 0
    return func->is_import_func
           ? NULL : func->u.func->code + func->u.func->code_size;
#else
    return func->is_import_func
           ? NULL : func->u.func->code_compiled + func->u.func->code_compiled_size;
#endif
}

WASMModule *
wasm_load(const uint8 *buf, uint32 size,
          char *error_buf, uint32 error_buf_size);

WASMModule *
wasm_load_from_sections(WASMSection *section_list,
                        char *error_buf, uint32_t error_buf_size);

void
wasm_unload(WASMModule *module);

WASMModuleInstance *
wasm_instantiate(WASMModule *module,
                 uint32 stack_size, uint32 heap_size,
                 char *error_buf, uint32 error_buf_size);

void
wasm_deinstantiate(WASMModuleInstance *module_inst);

WASMFunctionInstance *
wasm_lookup_function(const WASMModuleInstance *module_inst,
                             const char *name, const char *signature);

bool
wasm_call_function(WASMExecEnv *exec_env,
                   WASMFunctionInstance *function,
                   unsigned argc, uint32 argv[]);

bool
wasm_create_exec_env_and_call_function(WASMModuleInstance *module_inst,
                                       WASMFunctionInstance *function,
                                       unsigned argc, uint32 argv[]);

void
wasm_set_exception(WASMModuleInstance *module, const char *exception);

const char*
wasm_get_exception(WASMModuleInstance *module);

int32
wasm_module_malloc(WASMModuleInstance *module_inst, uint32 size,
                   void **p_native_addr);

void
wasm_module_free(WASMModuleInstance *module_inst, int32 ptr);

int32
wasm_module_dup_data(WASMModuleInstance *module_inst,
                     const char *src, uint32 size);

bool
wasm_validate_app_addr(WASMModuleInstance *module_inst,
                       int32 app_offset, uint32 size);

bool
wasm_validate_app_str_addr(WASMModuleInstance *module_inst,
                           int32 app_offset);

bool
wasm_validate_native_addr(WASMModuleInstance *module_inst,
                          void *native_ptr, uint32 size);

void *
wasm_addr_app_to_native(WASMModuleInstance *module_inst,
                        int32 app_offset);

int32
wasm_addr_native_to_app(WASMModuleInstance *module_inst,
                        void *native_ptr);

bool
wasm_get_app_addr_range(WASMModuleInstance *module_inst,
                        int32 app_offset,
                        int32 *p_app_start_offset,
                        int32 *p_app_end_offset);

bool
wasm_get_native_addr_range(WASMModuleInstance *module_inst,
                           uint8_t *native_ptr,
                           uint8_t **p_native_start_addr,
                           uint8_t **p_native_end_addr);

bool
wasm_enlarge_memory(WASMModuleInstance *module, uint32 inc_page_count);

#ifdef __cplusplus
}
#endif

#endif /* end of _WASM_RUNTIME_H */

