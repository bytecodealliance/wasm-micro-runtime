/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _AOT_RUNTIME_H_
#define _AOT_RUNTIME_H_

#include "bh_platform.h"
#include "../common/wasm_runtime_common.h"
#include "../interpreter/wasm_runtime.h"
#include "../compilation/aot.h"
#if WASM_ENABLE_JIT != 0
#include "../compilation/aot_llvm.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum AOTExceptionID {
    EXCE_UNREACHABLE = 0,
    EXCE_OUT_OF_MEMORY,
    EXCE_OUT_OF_BOUNDS_MEMORY_ACCESS,
    EXCE_INTEGER_OVERFLOW,
    EXCE_INTEGER_DIVIDE_BY_ZERO,
    EXCE_INVALID_CONVERSION_TO_INTEGER,
    EXCE_INVALID_FUNCTION_TYPE_INDEX,
    EXCE_INVALID_FUNCTION_INDEX,
    EXCE_UNDEFINED_ELEMENT,
    EXCE_UNINITIALIZED_ELEMENT,
    EXCE_CALL_UNLINKED_IMPORT_FUNC,
    EXCE_NATIVE_STACK_OVERFLOW,
    EXCE_UNALIGNED_ATOMIC,
    EXCE_NUM,
} AOTExceptionID;

typedef enum AOTSectionType {
    AOT_SECTION_TYPE_TARGET_INFO = 0,
    AOT_SECTION_TYPE_INIT_DATA,
    AOT_SECTION_TYPE_TEXT,
    AOT_SECTION_TYPE_FUNCTION,
    AOT_SECTION_TYPE_EXPORT,
    AOT_SECTION_TYPE_RELOCATION,
    AOT_SECTION_TYPE_SIGANATURE
} AOTSectionType;

typedef struct AOTObjectDataSection {
    char *name;
    uint8 *data;
    uint32 size;
} AOTObjectDataSection;

/* Relocation info */
typedef struct AOTRelocation {
    uint64 relocation_offset;
    uint64 relocation_addend;
    uint32 relocation_type;
    char *symbol_name;
    /* index in the symbol offset field */
    uint32 symbol_index;
} AOTRelocation;

/* Relocation Group */
typedef struct AOTRelocationGroup {
    char *section_name;
    /* index in the symbol offset field */
    uint32 name_index;
    uint32 relocation_count;
    AOTRelocation *relocations;
} AOTRelocationGroup;

/* AOT function instance */
typedef struct AOTFunctionInstance {
    char *func_name;
    uint32 func_index;
    bool is_import_func;
    union {
        struct {
            AOTFuncType *func_type;
            /* function pointer linked */
            void *func_ptr;
        } func;
        AOTImportFunc *func_import;
    } u;
} AOTFunctionInstance;

typedef struct AOTModule {
    uint32 module_type;

    /* import memories */
    uint32 import_memory_count;
    AOTImportMemory *import_memories;

    /* memory info */
    uint32 memory_count;
    AOTMemory *memories;

    /* init data */
    uint32 mem_init_data_count;
    AOTMemInitData **mem_init_data_list;

    /* import tables */
    uint32 import_table_count;
    AOTImportTable *import_tables;

    /* tables */
    uint32 table_count;
    AOTTable *tables;

    /* table init data info */
    uint32 table_init_data_count;
    AOTTableInitData **table_init_data_list;

    /* function type info */
    uint32 func_type_count;
    AOTFuncType **func_types;

    /* import global varaible info */
    uint32 import_global_count;
    AOTImportGlobal *import_globals;

    /* global variable info */
    uint32 global_count;
    AOTGlobal *globals;

    /* total global variable size */
    uint32 global_data_size;

    /* import function info */
    uint32 import_func_count;
    AOTImportFunc *import_funcs;

    /* function info */
    uint32 func_count;
    /* point to AOTed/JITed functions */
    void **func_ptrs;
    /* function type indexes */
    uint32 *func_type_indexes;

    /* export info */
    uint32 export_count;
    AOTExport *exports;

    /* start function index, -1 denotes no start function */
    uint32 start_func_index;
    /* start function, point to AOTed/JITed function */
    void *start_function;

    /* AOTed code, NULL for JIT mode */
    void *code;
    uint32 code_size;

    /* literal for AOTed code, NULL for JIT mode */
    uint8 *literal;
    uint32 literal_size;

    /* data sections in AOT object file, including .data, .rodata
     * and .rodata.cstN. NULL for JIT mode. */
    AOTObjectDataSection *data_sections;
    uint32 data_section_count;

    /* constant string set */
    HashMap *const_str_set;

    uint32 llvm_aux_data_end;
    uint32 llvm_aux_stack_bottom;
    uint32 llvm_aux_stack_size;
    uint32 llvm_aux_stack_global_index;

    /* is jit mode or not */
    bool is_jit_mode;

#if WASM_ENABLE_JIT != 0
    WASMModule *wasm_module;
    AOTCompContext *comp_ctx;
    AOTCompData *comp_data;
#endif

#if WASM_ENABLE_LIBC_WASI != 0
    WASIArguments wasi_args;
    bool is_wasi_module;
#endif
} AOTModule;

typedef union {
    uint64 _make_it_8_bytes_;
    void *ptr;
} AOTPointer;

typedef struct AOTMemoryInstance {
    uint32 module_type;
    /* shared memory flag */
    bool is_shared;
    /* memory space info */
    uint32 mem_cur_page_count;
    uint32 mem_max_page_count;
    uint32 memory_data_size;
    uint32 __padding__;
    AOTPointer memory_data;
    AOTPointer memory_data_end;

    /* heap space info */
    int32 heap_base_offset;
    uint32 heap_data_size;
    AOTPointer heap_data;
    AOTPointer heap_data_end;
    AOTPointer heap_handle;

    /* boundary check constants for aot code */
    int64 mem_bound_check_heap_base;
    int64 mem_bound_check_1byte;
    int64 mem_bound_check_2bytes;
    int64 mem_bound_check_4bytes;
    int64 mem_bound_check_8bytes;
} AOTMemoryInstance;

typedef struct AOTModuleInstance {
    uint32 module_type;

    /* memories */
    uint32 memory_count;
    AOTPointer memories;

    /* global and table info */
    uint32 global_data_size;
    uint32 table_size;
    AOTPointer global_data;
    AOTPointer table_data;

    /* funciton pointer array */
    AOTPointer func_ptrs;
    /* function type indexes */
    AOTPointer func_type_indexes;

    /* export info */
    uint32 export_func_count;
    uint32 export_global_count;
    uint32 export_mem_count;
    uint32 export_tab_count;
    AOTPointer export_funcs;
    AOTPointer export_globals;
    AOTPointer export_memories;
    AOTPointer export_tables;

    /* The exception buffer for current thread. */
    char cur_exception[128];
    /* The custom data that can be set/get by
     * wasm_runtime_set_custom_data/wasm_runtime_get_custom_data */
    AOTPointer custom_data;
    /* The AOT module */
    AOTPointer aot_module;
    /* WASI context */
    AOTPointer wasi_ctx;

    /* others */
    int32 temp_ret;
    uint32 llvm_stack;
    uint32 default_wasm_stack_size;

    /* reserved */
    uint32 reserved[11];

    union {
        uint64 _make_it_8_byte_aligned_;
        AOTMemoryInstance memory_instances[1];
        uint8 bytes[1];
    } global_table_data;
} AOTModuleInstance;

/* Target info, read from ELF header of object file */
typedef struct AOTTargetInfo {
    /* Binary type, elf32l/elf32b/elf64l/elf64b */
    uint16 bin_type;
    /* ABI type */
    uint16 abi_type;
    /* Object file type */
    uint16 e_type;
    /* Architecture */
    uint16 e_machine;
    /* Object file version */
    uint32 e_version;
    /* Processor-specific flags */
    uint32 e_flags;
    /* Reserved */
    uint32 reserved;
    /* Arch name */
    char arch[16];
} AOTTargetInfo;

/**
 * Load a AOT module from aot file buffer
 * @param buf the byte buffer which contains the AOT file data
 * @param size the size of the buffer
 * @param error_buf output of the error info
 * @param error_buf_size the size of the error string
 *
 * @return return AOT module loaded, NULL if failed
 */
AOTModule*
aot_load_from_aot_file(const uint8 *buf, uint32 size,
                       char *error_buf, uint32 error_buf_size);

/**
 * Load a AOT module from a specified AOT section list.
 *
 * @param section_list the section list which contains each section data
 * @param error_buf output of the error info
 * @param error_buf_size the size of the error string
 *
 * @return return AOT module loaded, NULL if failed
 */
AOTModule*
aot_load_from_sections(AOTSection *section_list,
                       char *error_buf, uint32 error_buf_size);

#if WASM_ENABLE_JIT != 0
/**
 * Convert WASM module to AOT module
 *
 * @param wasm_module the WASM module to convert
 * @param error_buf output of the error info
 * @param error_buf_size the size of the error string
 *
 * @return return AOT module loaded, NULL if failed
 */
AOTModule*
aot_convert_wasm_module(WASMModule *wasm_module,
                        char *error_buf, uint32 error_buf_size);
#endif

/**
 * Unload a AOT module.
 *
 * @param module the module to be unloaded
 */
void
aot_unload(AOTModule *module);

/**
 * Instantiate a AOT module.
 *
 * @param module the AOT module to instantiate
 * @param is_sub_inst the flag of sub instance
 * @param heap_size the default heap size of the module instance, a heap will
 *        be created besides the app memory space. Both wasm app and native
 *        function can allocate memory from the heap. If heap_size is 0, the
 *        default heap size will be used.
 * @param error_buf buffer to output the error info if failed
 * @param error_buf_size the size of the error buffer
 *
 * @return return the instantiated AOT module instance, NULL if failed
 */
AOTModuleInstance*
aot_instantiate(AOTModule *module, bool is_sub_inst,
                uint32 stack_size, uint32 heap_size,
                char *error_buf, uint32 error_buf_size);

/**
 * Deinstantiate a AOT module instance, destroy the resources.
 *
 * @param module_inst the AOT module instance to destroy
 * @param is_sub_inst the flag of sub instance
 */
void
aot_deinstantiate(AOTModuleInstance *module_inst, bool is_sub_inst);

/**
 * Lookup an exported function in the AOT module instance.
 *
 * @param module_inst the module instance
 * @param name the name of the function
 * @param signature the signature of the function, use "i32"/"i64"/"f32"/"f64"
 *        to represent the type of i32/i64/f32/f64, e.g. "(i32i64)" "(i32)f32"
 *
 * @return the function instance found
 */
AOTFunctionInstance*
aot_lookup_function(const AOTModuleInstance *module_inst,
                    const char *name, const char *signature);
/**
 * Call the given AOT function of a AOT module instance with
 * arguments.
 *
 * @param exec_env the execution environment
 * @param function the function to be called
 * @param argc the number of arguments
 * @param argv the arguments.  If the function method has return value,
 *   the first (or first two in case 64-bit return value) element of
 *   argv stores the return value of the called AOT function after this
 *   function returns.
 *
 * @return true if success, false otherwise and exception will be thrown,
 *   the caller can call aot_get_exception to get exception info.
 */
bool
aot_call_function(WASMExecEnv *exec_env,
                  AOTFunctionInstance *function,
                  unsigned argc, uint32 argv[]);

bool
aot_create_exec_env_and_call_function(AOTModuleInstance *module_inst,
                                      AOTFunctionInstance *function,
                                      unsigned argc, uint32 argv[]);
/**
 * Set AOT module instance exception with exception string
 *
 * @param module the AOT module instance
 *
 * @param exception current exception string
 */
void
aot_set_exception(AOTModuleInstance *module_inst,
                  const char *exception);

void
aot_set_exception_with_id(AOTModuleInstance *module_inst,
                          uint32 id);

/**
 * Get exception info of the AOT module instance.
 *
 * @param module_inst the AOT module instance
 *
 * @return the exception string
 */
const char*
aot_get_exception(AOTModuleInstance *module_inst);

/**
 * Clear exception info of the AOT module instance.
 *
 * @param module_inst the AOT module instance
 */
void
aot_clear_exception(AOTModuleInstance *module_inst);

int32
aot_module_malloc(AOTModuleInstance *module_inst, uint32 size,
                  void **p_native_addr);

void
aot_module_free(AOTModuleInstance *module_inst, int32 ptr);

int32
aot_module_dup_data(AOTModuleInstance *module_inst,
                    const char *src, uint32 size);

bool
aot_validate_app_addr(AOTModuleInstance *module_inst,
                      int32 app_offset, uint32 size);


bool
aot_validate_native_addr(AOTModuleInstance *module_inst,
                         void *native_ptr, uint32 size);

void *
aot_addr_app_to_native(AOTModuleInstance *module_inst, int32 app_offset);

int32
aot_addr_native_to_app(AOTModuleInstance *module_inst, void *native_ptr);

bool
aot_get_app_addr_range(AOTModuleInstance *module_inst,
                       int32 app_offset,
                       int32 *p_app_start_offset,
                       int32 *p_app_end_offset);

bool
aot_get_native_addr_range(AOTModuleInstance *module_inst,
                          uint8 *native_ptr,
                          uint8 **p_native_start_addr,
                          uint8 **p_native_end_addr);

bool
aot_enlarge_memory(AOTModuleInstance *module_inst, uint32 inc_page_count);

/**
 * Compare whether two wasm types are equal according to the indexs
 *
 * @param module_inst the AOT module instance
 * @param type1_idx index of the first wasm type
 * @param type2_idx index of the second wasm type
 *
 * @return true if equal, false otherwise
 */
bool
aot_is_wasm_type_equal(AOTModuleInstance *module_inst,
                       uint32 type1_idx, uint32 type2_idx);

/**
 * Invoke native function from aot code
 */
bool
aot_invoke_native(WASMExecEnv *exec_env, uint32 func_idx,
                  uint32 argc, uint32 *argv);

bool
aot_call_indirect(WASMExecEnv *exec_env,
                  bool check_func_type, uint32 func_type_idx,
                  uint32 table_elem_idx,
                  uint32 argc, uint32 *argv);

uint32
aot_get_plt_table_size();

#if WASM_ENABLE_BULK_MEMORY != 0
bool
aot_memory_init(AOTModuleInstance *module_inst, uint32 seg_index,
                uint32 offset, uint32 len, uint32 dst);

bool
aot_data_drop(AOTModuleInstance *module_inst, uint32 seg_index);
#endif

#if WASM_ENABLE_THREAD_MGR != 0
bool
aot_set_aux_stack(WASMExecEnv *exec_env,
                  uint32 start_offset, uint32 size);

bool
aot_get_aux_stack(WASMExecEnv *exec_env,
                  uint32 *start_offset, uint32 *size);
#endif

#ifdef OS_ENABLE_HW_BOUND_CHECK
bool
aot_signal_init();

void
aot_signal_destroy();
#endif

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _AOT_RUNTIME_H_ */

