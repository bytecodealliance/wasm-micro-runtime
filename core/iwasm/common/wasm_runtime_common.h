/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WASM_COMMON_H
#define _WASM_COMMON_H

#include "bh_platform.h"
#include "bh_common.h"
#include "bh_thread.h"
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

#if WASM_ENABLE_LIBC_WASI != 0
typedef struct WASIContext {
    struct fd_table *curfds;
    struct fd_prestats *prestats;
    struct argv_environ_values *argv_environ;
} WASIContext;
#endif

typedef package_type_t PackageType;
typedef wasm_section_t WASMSection, AOTSection;

/* See wasm_export.h for description */
bool
wasm_runtime_init();

/* See wasm_export.h for description */
bool
wasm_runtime_full_init(RuntimeInitArgs *init_args);

/* See wasm_export.h for description */
void
wasm_runtime_destroy();

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
                                char *error_buf, uint32_t error_buf_size);

/* See wasm_export.h for description */
void
wasm_runtime_unload(WASMModuleCommon *module);

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
bool
wasm_runtime_call_wasm(WASMExecEnv *exec_env,
                       WASMFunctionInstanceCommon *function,
                       unsigned argc, uint32 argv[]);

bool
wasm_runtime_create_exec_env_and_call_wasm(WASMModuleInstanceCommon *module_inst,
                                           WASMFunctionInstanceCommon *function,
                                           unsigned argc, uint32 argv[]);

/* See wasm_export.h for description */
bool
wasm_application_execute_main(WASMModuleInstanceCommon *module_inst,
                              int argc, char *argv[]);

/* See wasm_export.h for description */
bool
wasm_application_execute_func(WASMModuleInstanceCommon *module_inst,
                              const char *name, int argc, char *argv[]);

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
int32
wasm_runtime_module_malloc(WASMModuleInstanceCommon *module_inst, uint32 size,
                           void **p_native_addr);

/* See wasm_export.h for description */
void
wasm_runtime_module_free(WASMModuleInstanceCommon *module_inst, int32 ptr);

/* See wasm_export.h for description */
int32
wasm_runtime_module_dup_data(WASMModuleInstanceCommon *module_inst,
                             const char *src, uint32 size);

/* See wasm_export.h for description */
bool
wasm_runtime_validate_app_addr(WASMModuleInstanceCommon *module_inst,
                               int32 app_offset, uint32 size);

/* See wasm_export.h for description */
bool
wasm_runtime_validate_app_str_addr(WASMModuleInstanceCommon *module_inst,
                                   int32 app_str_offset);

/* See wasm_export.h for description */
bool
wasm_runtime_validate_native_addr(WASMModuleInstanceCommon *module_inst,
                                  void *native_ptr, uint32 size);

/* See wasm_export.h for description */
void *
wasm_runtime_addr_app_to_native(WASMModuleInstanceCommon *module_inst,
                                int32 app_offset);

/* See wasm_export.h for description */
int32
wasm_runtime_addr_native_to_app(WASMModuleInstanceCommon *module_inst,
                                void *native_ptr);

/* See wasm_export.h for description */
bool
wasm_runtime_get_app_addr_range(WASMModuleInstanceCommon *module_inst,
                                int32 app_offset,
                                int32 *p_app_start_offset,
                                int32 *p_app_end_offset);

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

bool
wasm_runtime_invoke_native(WASMExecEnv *exec_env, void *func_ptr,
                           const WASMType *func_type, const char *signature,
                           uint32 *argv, uint32 argc, uint32 *ret);


#ifdef __cplusplus
}
#endif

#endif /* end of _WASM_COMMON_H */

