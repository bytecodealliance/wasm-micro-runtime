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

#ifndef _WASM_RUNTIME_H
#define _WASM_RUNTIME_H

#include "wasm.h"
#include "wasm-thread.h"
#include "wasm_hashmap.h"

#ifdef __cplusplus
extern "C" {
#endif


#define DEFAULT_WASM_STACK_SIZE (8 * 1024)
#define DEFAULT_WASM_HEAP_SIZE  (8 * 1024)
#define MIN_WASM_HEAP_SIZE      (1 * 1024)

typedef struct WASMMemoryInstance {
    /* Current page count */
    uint32 cur_page_count;
    /* Maximum page count */
    uint32 max_page_count;
    /* Data of import globals with address info, like _stdin/_stdout/_stderr,
       stdin/stdout/stderr is stored here, but the actual addr info, or offset
       to memory_data is stored in global_data section */
    uint8 *addr_data;
    /* Size of addr_data */
    uint32 addr_data_size;

    /* Thunk data of argument strings */
    uint8 *thunk_argv_data;
    uint32 thunk_argv_data_size;
    /* Thunk argument count */
    uint32 thunk_argc;
    /* Thunk argument offsets */
    uint8 *thunk_argv_offsets;

    /* Heap data */
    uint8 *heap_data;
    /* Heap size */
    uint32 heap_data_size;
    /* The heap created */
    void *heap_handle;

    /* Memory data */
    uint8 *memory_data;
    /* Global data of global instances */
    uint8 *global_data;
    uint32 global_data_size;

    /* End address of memory */
    uint8 *end_addr;

    /* Base address, the layout is:
       addr_data + thunk_argv data + thunk arg offsets +
       heap data + memory data + global data
       memory data init size is: NumBytesPerPage * cur_page_count
       addr data size and global data size is calculated in module instantiating
       Note: when memory is re-allocated, the addr data, thunk argv data, thunk
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
    bool is_addr;
    /* data offset to base_addr of WASMMemoryInstance */
    uint32 data_offset;
    /* initial value */
    WASMValue initial_value;
} WASMGlobalInstance;

typedef struct WASMFunctionInstance {
    /* whether it is import function or WASM function */
    bool is_import_func;
    /* cell num of parameters */
    uint16 param_cell_num;
    /* cell num of return type */
    uint16 ret_cell_num;
    /* cell num of local variables, 0 for import function */
    uint16 local_cell_num;
    uint16 *local_offsets;
    union {
        WASMFunctionImport *func_import;
        WASMFunction *func;
    } u;
} WASMFunctionInstance;

typedef struct WASMExportFuncInstance {
    char *name;
    WASMFunctionInstance *function;
} WASMExportFuncInstance;

/* Package Type */
typedef enum {
    Wasm_Module_Bytecode = 0,
    Wasm_Module_AoT,
    Package_Type_Unknown = 0xFFFF
} PackageType;

typedef struct WASMModuleInstance {
    /* Module instance type, for module instance loaded from
       WASM bytecode binary, this field is Wasm_Module_Bytecode;
       for module instance loaded from AOT package, this field is
       Wasm_Module_AoT, and this structure should be treated as
       WASMAOTContext structure. */
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

    HashMap *branch_set;
    const WASMModule *module;

    uint32 DYNAMICTOP_PTR_offset;
    uint32 temp_ret;
    uint32 llvm_stack;

    /* Default WASM stack size of threads of this Module instance. */
    uint32 wasm_stack_size;

    /* Default WASM stack */
    uint8 *wasm_stack;

    /* The exception buffer of wasm interpreter for current thread. */
    char cur_exception[128];

    /* The thread data of the attaching thread */
    void *thread_data;

    /* Main Thread */
    WASMThread main_tlr;
} WASMModuleInstance;

/* Execution environment, e.g. stack info */
typedef struct WASMExecEnv {
    uint8_t *stack;
    uint32_t stack_size;
} WASMExecEnv;

struct WASMInterpFrame;
typedef struct WASMInterpFrame WASMRuntimeFrame;

/**
 * Return the current thread.
 *
 * @return the current thread
 */
static inline WASMThread*
wasm_runtime_get_self()
{
    return (WASMThread*)ws_tls_get();
}

/**
 * Set self as the current thread.
 *
 * @param self the thread to be set as current thread
 */
static inline void
wasm_runtime_set_tlr(WASMThread *self)
{
    ws_tls_put(self);
}

/**
 * Return the code block of a function.
 *
 * @param func the WASM function instance
 *
 * @return the code block of the function
 */
static inline uint8*
wasm_runtime_get_func_code(WASMFunctionInstance *func)
{
    return func->is_import_func ? NULL : func->u.func->code;
}

/**
 * Return the code block end of a function.
 *
 * @param func the WASM function instance
 *
 * @return the code block end of the function
 */
static inline uint8*
wasm_runtime_get_func_code_end(WASMFunctionInstance *func)
{
    return func->is_import_func
           ? NULL : func->u.func->code + func->u.func->code_size;
}

/**
 * Call the given WASM function of a WASM module instance with arguments (bytecode and AoT).
 *
 * @param module_inst the WASM module instance which the function belongs to
 * @param exec_env the execution environment to call the function. If the module instance
 *   is created by AoT mode, it is ignored and just set it to NULL. If the module instance
 *   is created by bytecode mode and it is NULL, a temporary env object will be created
 * @param function the function to be called
 * @param argc the number of arguments
 * @param argv the arguments.  If the function method has return value,
 *   the first (or first two in case 64-bit return value) element of
 *   argv stores the return value of the called WASM function after this
 *   function returns.
 *
 * @return true if success, false otherwise and exception will be thrown,
 *   the caller can call wasm_runtime_get_exception to get exception info.
 */
bool
wasm_runtime_call_wasm(WASMModuleInstance *module,
                       WASMExecEnv *exec_env,
                       WASMFunctionInstance *function,
                       unsigned argc, uint32 argv[]);

/**
 * Set current exception string to global exception string.
 *
 * @param module the wasm module instance
 *
 * @param exception current exception string
 */
void
wasm_runtime_set_exception(WASMModuleInstance *module,
                           const char *exception);

/**
 * Get current exception string.
 *
 * @param module the wasm module instance
 *
 * @return return exception string if exception is thrown, NULL otherwise
 */
const char*
wasm_runtime_get_exception(WASMModuleInstance *module);

/**
 * Enlarge wasm memory data space.
 *
 * @param module the wasm module instance
 * @param inc_page_count denote the page number to increase
 * @return return true if enlarge successfully, false otherwise
 */
bool
wasm_runtime_enlarge_memory(WASMModuleInstance *module, int inc_page_count);

/* See wasm-export.h for description */
WASMModuleInstance *
wasm_runtime_get_current_module_inst();

/* See wasm-export.h for description */
int32_t
wasm_runtime_module_malloc(WASMModuleInstance *module_inst, uint32_t size);

/* See wasm-export.h for description */
void
wasm_runtime_module_free(WASMModuleInstance *module_inst, int32_t ptr);

/* See wasm-export.h for description */
bool
wasm_runtime_validate_app_addr(WASMModuleInstance *module_inst,
                               int32_t app_offset, uint32_t size);

/* See wasm-export.h for description */
bool
wasm_runtime_validate_native_addr(WASMModuleInstance *module_inst,
                                  void *native_ptr, uint32_t size);

/* See wasm-export.h for description */
void *
wasm_runtime_addr_app_to_native(WASMModuleInstance *module_inst,
                                int32_t app_offset);

/* See wasm-export.h for description */
int32_t
wasm_runtime_addr_native_to_app(WASMModuleInstance *module_inst,
                                void *native_ptr);

#ifdef __cplusplus
}
#endif

#endif /* end of _WASM_RUNTIME_H */

