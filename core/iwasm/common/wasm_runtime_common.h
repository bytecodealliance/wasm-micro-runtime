/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WASM_COMMON_H
#define _WASM_COMMON_H

#include "bh_platform.h"
#include "bh_common.h"
#include "wasm_exec_env.h"
#include "wasm_native.h"
#include "../include/wasm_export.h"
#include "../interpreter/wasm.h"
#if WASM_ENABLE_LIBC_WASI != 0
#include "wasmtime_ssp.h"
#include "posix.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct WASMModuleCommon {
    /* Module type, for module loaded from WASM bytecode binary,
       this field is Wasm_Module_Bytecode, and this structure should
       be treated as WASMModule structure;
       for module loaded from AOT binary, this field is
       Wasm_Module_AoT, and this structure should be treated as
       AOTModule structure. */
    uint32 module_type;
    uint8 module_data[1];
} WASMModuleCommon;

typedef struct WASMModuleInstanceCommon {
    /* Module instance type, for module instance loaded from WASM
       bytecode binary, this field is Wasm_Module_Bytecode, and this
       structure should be treated as WASMModuleInstance structure;
       for module instance loaded from AOT binary, this field is
       Wasm_Module_AoT, and this structure should be treated as
       AOTModuleInstance structure. */
    uint32 module_type;
    uint8 module_inst_data[1];
} WASMModuleInstanceCommon;

typedef struct WASMModuleMemConsumption {
    uint32 total_size;
    uint32 module_struct_size;
    uint32 types_size;
    uint32 imports_size;
    uint32 functions_size;
    uint32 tables_size;
    uint32 memories_size;
    uint32 globals_size;
    uint32 exports_size;
    uint32 table_segs_size;
    uint32 data_segs_size;
    uint32 const_strs_size;
#if WASM_ENABLE_AOT != 0
    uint32 aot_code_size;
#endif
} WASMModuleMemConsumption;

typedef struct WASMModuleInstMemConsumption {
    uint32 total_size;
    uint32 module_inst_struct_size;
    uint32 memories_size;
    uint32 app_heap_size;
    uint32 tables_size;
    uint32 globals_size;
    uint32 functions_size;
    uint32 exports_size;
} WASMModuleInstMemConsumption;

#if WASM_ENABLE_LIBC_WASI != 0
typedef struct WASIContext {
    /* Use offset but not native address, since these fields are
       allocated from app's heap, and the heap space may be re-allocated
       after memory.grow opcode is executed, the original native address
       cannot be accessed again. */
    uint32 curfds_offset;
    uint32 prestats_offset;
    uint32 argv_environ_offset;
    uint32 argv_buf_offset;
    uint32 argv_offsets_offset;
    uint32 env_buf_offset;
    uint32 env_offsets_offset;
} WASIContext;
#endif

#if WASM_ENABLE_MULTI_MODULE != 0
typedef struct WASMRegisteredModule {
    bh_list_link l;
    /* point to a string pool */
    const char *module_name;
    WASMModuleCommon *module;
    /* to store the original module file buffer address */
    uint8 *orig_file_buf;
    uint32 orig_file_buf_size;
} WASMRegisteredModule;
#endif

typedef struct WASMMemoryInstanceCommon {
    uint32 module_type;
    uint8 memory_inst_data[1];
} WASMMemoryInstanceCommon;

typedef package_type_t PackageType;
typedef wasm_section_t WASMSection, AOTSection;

/* See wasm_export.h for description */
bool
wasm_runtime_init(void);

/* See wasm_export.h for description */
bool
wasm_runtime_full_init(RuntimeInitArgs *init_args);

/* See wasm_export.h for description */
void
wasm_runtime_destroy(void);

/* See wasm_export.h for description */
PackageType
get_package_type(const uint8 *buf, uint32 size);


/* See wasm_export.h for description */
WASMModuleCommon *
wasm_runtime_load(const uint8 *buf, uint32 size,
                  char *error_buf, uint32 error_buf_size);

/* See wasm_export.h for description */
WASMModuleCommon *
wasm_runtime_load_from_sections(WASMSection *section_list, bool is_aot,
                                char *error_buf, uint32 error_buf_size);

/* See wasm_export.h for description */
void
wasm_runtime_unload(WASMModuleCommon *module);

/* Internal API */
WASMModuleInstanceCommon *
wasm_runtime_instantiate_internal(WASMModuleCommon *module, bool is_sub_inst,
                                  uint32 stack_size, uint32 heap_size,
                                  char *error_buf, uint32 error_buf_size);

/* Internal API */
void
wasm_runtime_deinstantiate_internal(WASMModuleInstanceCommon *module_inst,
                                    bool is_sub_inst);

/* See wasm_export.h for description */
WASMModuleInstanceCommon *
wasm_runtime_instantiate(WASMModuleCommon *module,
                         uint32 stack_size, uint32 heap_size,
                         char *error_buf, uint32 error_buf_size);

/* See wasm_export.h for description */
void
wasm_runtime_deinstantiate(WASMModuleInstanceCommon *module_inst);

/* See wasm_export.h for description */
WASMFunctionInstanceCommon *
wasm_runtime_lookup_function(WASMModuleInstanceCommon * const module_inst,
                             const char *name, const char *signature);

/* See wasm_export.h for description */
WASMExecEnv *
wasm_runtime_create_exec_env(WASMModuleInstanceCommon *module_inst,
                             uint32 stack_size);

/* See wasm_export.h for description */
void
wasm_runtime_destroy_exec_env(WASMExecEnv *exec_env);

/* See wasm_export.h for description */
WASMModuleInstanceCommon *
wasm_runtime_get_module_inst(WASMExecEnv *exec_env);

/* See wasm_export.h for description */
void *
wasm_runtime_get_function_attachment(WASMExecEnv *exec_env);

/* See wasm_export.h for description */
void
wasm_runtime_set_user_data(WASMExecEnv *exec_env, void *user_data);

/* See wasm_export.h for description */
void *
wasm_runtime_get_user_data(WASMExecEnv *exec_env);

/* See wasm_export.h for description */
bool
wasm_runtime_call_wasm(WASMExecEnv *exec_env,
                       WASMFunctionInstanceCommon *function,
                       uint32 argc, uint32 argv[]);

bool
wasm_runtime_call_wasm_a(WASMExecEnv *exec_env,
                         WASMFunctionInstanceCommon *function,
                         uint32 num_results, wasm_val_t *results,
                         uint32 num_args, wasm_val_t *args);

bool
wasm_runtime_call_wasm_v(WASMExecEnv *exec_env,
                         WASMFunctionInstanceCommon *function,
                         uint32 num_results, wasm_val_t *results,
                         uint32 num_args, ...);

/**
 * Call a function reference of a given WASM runtime instance with
 * arguments.
 *
 * @param exec_env the execution environment to call the function
 *   which must be created from wasm_create_exec_env()
 * @param element_indices the function ference indicies, usually
 *   prvovided by the caller of a registed native function
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
wasm_runtime_call_indirect(WASMExecEnv *exec_env,
                           uint32 element_indices,
                           uint32 argc, uint32 argv[]);

bool
wasm_runtime_create_exec_env_and_call_wasm(WASMModuleInstanceCommon *module_inst,
                                           WASMFunctionInstanceCommon *function,
                                           uint32 argc, uint32 argv[]);

/* See wasm_export.h for description */
bool
wasm_application_execute_main(WASMModuleInstanceCommon *module_inst,
                              int32 argc, char *argv[]);

/* See wasm_export.h for description */
bool
wasm_application_execute_func(WASMModuleInstanceCommon *module_inst,
                              const char *name, int32 argc, char *argv[]);

/* See wasm_export.h for description */
void
wasm_runtime_set_exception(WASMModuleInstanceCommon *module,
                           const char *exception);

/* See wasm_export.h for description */
const char *
wasm_runtime_get_exception(WASMModuleInstanceCommon *module);

/* See wasm_export.h for description */
void
wasm_runtime_clear_exception(WASMModuleInstanceCommon *module_inst);

/* See wasm_export.h for description */
void
wasm_runtime_set_custom_data(WASMModuleInstanceCommon *module_inst,
                             void *custom_data);

/* See wasm_export.h for description */
void *
wasm_runtime_get_custom_data(WASMModuleInstanceCommon *module_inst);

/* See wasm_export.h for description */
uint32
wasm_runtime_module_malloc(WASMModuleInstanceCommon *module_inst, uint32 size,
                           void **p_native_addr);

/* See wasm_export.h for description */
void
wasm_runtime_module_free(WASMModuleInstanceCommon *module_inst, uint32 ptr);

/* See wasm_export.h for description */
uint32
wasm_runtime_module_dup_data(WASMModuleInstanceCommon *module_inst,
                             const char *src, uint32 size);

/* See wasm_export.h for description */
bool
wasm_runtime_validate_app_addr(WASMModuleInstanceCommon *module_inst,
                               uint32 app_offset, uint32 size);

/* See wasm_export.h for description */
bool
wasm_runtime_validate_app_str_addr(WASMModuleInstanceCommon *module_inst,
                                   uint32 app_str_offset);

/* See wasm_export.h for description */
bool
wasm_runtime_validate_native_addr(WASMModuleInstanceCommon *module_inst,
                                  void *native_ptr, uint32 size);

/* See wasm_export.h for description */
void *
wasm_runtime_addr_app_to_native(WASMModuleInstanceCommon *module_inst,
                                uint32 app_offset);

/* See wasm_export.h for description */
uint32
wasm_runtime_addr_native_to_app(WASMModuleInstanceCommon *module_inst,
                                void *native_ptr);

/* See wasm_export.h for description */
bool
wasm_runtime_get_app_addr_range(WASMModuleInstanceCommon *module_inst,
                                uint32 app_offset,
                                uint32 *p_app_start_offset,
                                uint32 *p_app_end_offset);

/* See wasm_export.h for description */
bool
wasm_runtime_get_native_addr_range(WASMModuleInstanceCommon *module_inst,
                                   uint8 *native_ptr,
                                   uint8 **p_native_start_addr,
                                   uint8 **p_native_end_addr);

uint32
wasm_runtime_get_temp_ret(WASMModuleInstanceCommon *module_inst);

void
wasm_runtime_set_temp_ret(WASMModuleInstanceCommon *module_inst,
                          uint32 temp_ret);

uint32
wasm_runtime_get_llvm_stack(WASMModuleInstanceCommon *module_inst);

void
wasm_runtime_set_llvm_stack(WASMModuleInstanceCommon *module_inst,
                            uint32 llvm_stack);

#if WASM_ENABLE_MULTI_MODULE != 0
void
wasm_runtime_set_module_reader(const module_reader reader,
                               const module_destroyer destroyer);

module_reader
wasm_runtime_get_module_reader();

module_destroyer
wasm_runtime_get_module_destroyer();

bool
wasm_runtime_register_module_internal(const char *module_name,
                                      WASMModuleCommon *module,
                                      uint8 *orig_file_buf,
                                      uint32 orig_file_buf_size,
                                      char *error_buf,
                                      uint32 error_buf_size);

void
wasm_runtime_unregister_module(const WASMModuleCommon *module);

bool
wasm_runtime_is_module_registered(const char *module_name);

bool
wasm_runtime_add_loading_module(const char *module_name,
                                char *error_buf, uint32 error_buf_size);

void
wasm_runtime_delete_loading_module(const char *module_name);

bool
wasm_runtime_is_loading_module(const char *module_name);

void
wasm_runtime_destroy_loading_module_list();
#endif /* WASM_ENALBE_MULTI_MODULE */

bool
wasm_runtime_is_built_in_module(const char *module_name);

#if WASM_ENABLE_THREAD_MGR != 0
bool
wasm_exec_env_get_aux_stack(WASMExecEnv *exec_env,
                            uint32 *start_offset, uint32 *size);

bool
wasm_exec_env_set_aux_stack(WASMExecEnv *exec_env,
                            uint32 start_offset, uint32 size);
#endif

#if WASM_ENABLE_LIBC_WASI != 0
/* See wasm_export.h for description */
void
wasm_runtime_set_wasi_args(WASMModuleCommon *module,
                           const char *dir_list[], uint32 dir_count,
                           const char *map_dir_list[], uint32 map_dir_count,
                           const char *env_list[], uint32 env_count,
                           char *argv[], int argc);

/* See wasm_export.h for description */
bool
wasm_runtime_is_wasi_mode(WASMModuleInstanceCommon *module_inst);

/* See wasm_export.h for description */
WASMFunctionInstanceCommon *
wasm_runtime_lookup_wasi_start_function(WASMModuleInstanceCommon *module_inst);

bool
wasm_runtime_init_wasi(WASMModuleInstanceCommon *module_inst,
                       const char *dir_list[], uint32 dir_count,
                       const char *map_dir_list[], uint32 map_dir_count,
                       const char *env[], uint32 env_count,
                       char *argv[], uint32 argc,
                       char *error_buf, uint32 error_buf_size);

void
wasm_runtime_destroy_wasi(WASMModuleInstanceCommon *module_inst);

void
wasm_runtime_set_wasi_ctx(WASMModuleInstanceCommon *module_inst,
                          WASIContext *wasi_ctx);

WASIContext *
wasm_runtime_get_wasi_ctx(WASMModuleInstanceCommon *module_inst);

#endif /* end of WASM_ENABLE_LIBC_WASI */

/* Get module of the current exec_env */
WASMModuleCommon*
wasm_exec_env_get_module(WASMExecEnv *exec_env);

/**
 * Enlarge wasm memory data space.
 *
 * @param module the wasm module instance
 * @param inc_page_count denote the page number to increase
 * @return return true if enlarge successfully, false otherwise
 */
bool
wasm_runtime_enlarge_memory(WASMModuleInstanceCommon *module, uint32 inc_page_count);

/* See wasm_export.h for description */
bool
wasm_runtime_register_natives(const char *module_name,
                              NativeSymbol *native_symbols,
                              uint32 n_native_symbols);

/* See wasm_export.h for description */
bool
wasm_runtime_register_natives_raw(const char *module_name,
                                  NativeSymbol *native_symbols,
                                  uint32 n_native_symbols);

bool
wasm_runtime_invoke_native(WASMExecEnv *exec_env, void *func_ptr,
                           const WASMType *func_type, const char *signature,
                           void *attachment,
                           uint32 *argv, uint32 argc, uint32 *ret);

bool
wasm_runtime_invoke_native_raw(WASMExecEnv *exec_env, void *func_ptr,
                               const WASMType *func_type, const char *signature,
                               void *attachment,
                               uint32 *argv, uint32 argc, uint32 *ret);

void
wasm_runtime_dump_module_mem_consumption(const WASMModuleCommon *module);

void
wasm_runtime_dump_module_inst_mem_consumption(const WASMModuleInstanceCommon
                                              *module_inst);

void
wasm_runtime_dump_exec_env_mem_consumption(const WASMExecEnv *exec_env);

#ifdef __cplusplus
}
#endif

#endif /* end of _WASM_COMMON_H */

