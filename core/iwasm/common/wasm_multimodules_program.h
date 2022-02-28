#ifndef _WASM_MULTIMODULES_PROGRAM_H_
#define _WASM_MULTIMODULES_PROGRAM_H_

#include "../include/wasm_export.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ConstStrDescription ConstStrDescription;

#define TABLE_SPACE_BITS_LEN 10
#define TABLE_SPACE_SLOT_SIZE (1 << TABLE_SPACE_BITS_LEN)

#define TABLE_SPACE_FOR_INSTS_TOP_BOUNDARY 0x0fff0000u
#define MAX_INST_ID ((TABLE_SPACE_FOR_INSTS_TOP_BOUNDARY >> TABLE_SPACE_BITS_LEN) + 1)

#define BUILTIN_LIBC_INST_ID (MAX_INST_ID + 10u) //keep a gap
#define TABLE_SPACE_FOR_BUILTIN_LIBC ((BUILTIN_LIBC_INST_ID - 1) << 10)

typedef struct WASMModule WASMModule;

struct WASMModuleInstance;
typedef struct WASMModuleCommon WASMModuleCommon;
typedef struct WASMModuleInstanceCommon WASMModuleInstanceCommon;
typedef struct WASMModuleInstance WASMModuleInstance;
typedef struct AOTModuleInstance AOTModuleInstance;
typedef struct WASMTableInstance WASMTableInstance;
typedef struct WASMGlobalInstance WASMGlobalInstance;
typedef struct WASMFunctionInstance WASMFunctionInstance;
typedef struct AOTExportFunctionInstance AOTExportFunctionInstance;
typedef struct WASMExecEnv WASMExecEnv;
typedef struct AOTModule AOTModule;
typedef struct AOTImportFunc AOTImportFunc;

typedef struct WASMRuntime WASMRuntime;
typedef struct WASMProgramInstance WASMProgramInstance;
typedef struct WASMModuleInstanceHead WASMModuleInstanceHead;
typedef struct WASMExport WASMExport;
typedef struct WASMType WASMType;
typedef WASMType AOTFuncType;

typedef struct wasm_runtime_config_ {
    bool need_load_dependencies;
    bool auto_update_extension;
    bool launch_AS_module;
} wasm_runtime_config;

struct WASMRuntime {
    wasm_runtime_config config;
    // save all const strings and hash values.
    HashMap * global_const_str_pool;
    // save all const strings pointers based on string index,
    // can compare pointers instead of content.
    ConstStrDescription * global_const_str_index_array;
    uint32 csp_size;
    uint32 csp_strings_count;
    uint32 csp_free_index;

    HashMap * all_loaded_modules;
#if WASM_ENABLE_DYNAMIC_LINKING != 0
    WASMProgramInstance * cur_loading_program;
#endif

    module_reader reader;
    module_destroyer destroyer;
};

#if WASM_ENABLE_DYNAMIC_LINKING != 0
typedef enum {
    EARLY_BINDING = 0,
    LAZY_BINDING
} WASM_BINDING_MODE;

typedef enum {
    FROM_BUILTIN_LIBC = 0, // when root module and dependencies modules independently link against builtin libc.
    FROM_ROOT              // import malloc/free/realloc from root module exports.
} WASM_IMPORT_MEMOP_MODE;

typedef struct wasm_program_config_ {
    WASM_BINDING_MODE binding_mode;
    WASM_IMPORT_MEMOP_MODE import_memop_mode;
    bool use_tbl_as_cache; // allocate extra table space as resolve cache to store exports resolving result.
    bool root_is_AS_module;
    bool use_resolve_cache;
} wasm_program_config;

#define BINDING_MODE_MASK 0x1 // 0 is lazy_binding
#define MEM_ALLOCATOR_MASK (0x1 << 1) // 0 is from builtin libc, 1 is from root module.
#define USE_TBL_AS_CACHE_MASK (0x1 << 2)
#define ROOT_IS_AS_MODULE_MASK (0x1 << 3)
#define USE_RESOLVE_CACHE_MASK (0x1 << 4)
#define PROGRAM_RESOLVING_CACHE_LINE_LEN 2
#define PROGRAM_RESOLVING_CACHE_LINE_COUNT 16

typedef struct wasm_resolving_cache_entry_ {
    int32 index;
    void * func_inst;
#if WASM_ENABLE_AOT != 0
    void * module_inst; // interp doesn't need it, we saved the module inst in wasmfunctioninstance.
#endif
} wasm_resolving_cache_entry;

struct WASMProgramInstance {
    /* explicit dependency modules, means loading by dlopen */
    /* implicit dependency modules, means these modules in dylink section
       they will be recorded in WASMModule */
    WASMRuntime * runtime;
    wasm_program_config config;
    /* current program's root module instance, used to allocate init
       memory space for other modules */
    WASMModuleInstanceCommon * root_module_inst;
    // bh_list loading_modules_list;

    /* all dependency modules of current instance, avoid loading same module at runtime */
    /* they includes implicit modules comes from root module and explicit modules added
       at runtime by dlopen() */
    HashMap * global_modules_inst_name_hmap;

    wasm_resolving_cache_entry * resolving_cache;

    // alloc module instance id and table space according to instance id.
    HashMap * global_modules_inst_id_hmap;
    uint32 next_free_inst_id;

    // here, use a function array to simulate a internal libc module.
    // a little bit ugly, seems we should create a real libc wasm module internally.
#if WASM_ENABLE_LIBC_BUILTIN != 0
    void ** builtin_libc_funcs;
    uint32 builtin_libc_funcs_count;
    uint32 builtin_libc_funcs_size;
    /* memory allocated in user linear space, it's wasm linear addr, uint32 */
    uint32 ctype_tolower_loc_space;
#endif

    bool clean_all;

    WASMModuleInstanceCommon * exception_inst;
    char *error_buf;
    uint32 error_buf_size;
};
#endif

void
wasm_runtime_set_module_reader(const module_reader reader_cb,
                               const module_destroyer destroyer_cb);

module_reader
wasm_runtime_get_module_reader();

module_destroyer
wasm_runtime_get_module_destroyer();

WASMModuleCommon *
wasm_runtime_get_module_by_name(WASMRuntime * runtime, const ConstStrDescription * module_name);

bool
wasm_runtime_is_system_symbol(WASMRuntime * runtime, const ConstStrDescription * key);

bool
wasm_runtime_is_memop_symbol(WASMRuntime * runtime, const ConstStrDescription * key);

uint32
wasm_runtime_get_syssymbol_id(WASMRuntime * runtime, const ConstStrDescription * key);

const ConstStrDescription *
wasm_runtime_records_const_filename_string(WASMRuntime * runtime,
                                const char * str, const uint32 len,
                                char* error_buf, uint32 error_buf_size);

const ConstStrDescription *
wasm_runtime_records_const_string(WASMRuntime * runtime,
                                const char * str, const uint32 len,
                                char* error_buf, uint32 error_buf_size);

const ConstStrDescription *
upgrade_module_extension(const WASMRuntime *runtime,
                        const ConstStrDescription * key_module_name,
                        const package_type_t expected_module_type,
                        char * error_buf,
                        uint32 error_buf_size);

WASMRuntime *
wasm_runtime_get_runtime();

bool
wasm_runtime_runtime_init(bool standalone, bool auto_ext_name);

void
wasm_runtime_runtime_destroy();

WASMModuleInstanceCommon *
wasm_program_get_root_module_from_inst(const WASMModuleInstanceCommon * module_inst);

#if WASM_ENABLE_DYNAMIC_LINKING != 0

#if WASM_ENABLE_LIBC_BUILTIN != 0
uint32
wasm_program_get_ctype_tolower_mem(WASMModuleInstanceCommon * module_inst);

void
wasm_program_set_ctype_tolower_mem(WASMModuleInstanceCommon * module_inst, uint32 addr);
#endif

WASMProgramInstance *
wasm_runtime_create_program_internal(
                    char * error_buf, uint32 error_buf_size,
                    uint32 dlopen_mode);

void
wasm_runtime_destroy_program_internal(WASMProgramInstance * program);


bool
wasm_program_is_root_module(const WASMModuleInstanceCommon * module_inst);

WASMModuleInstanceCommon *
wasm_program_get_root_module(const WASMProgramInstance * program);

WASMModuleInstanceCommon *
wasm_program_get_root_module_from_env(const wasm_exec_env_t env);

void
wasm_program_set_root_module(WASMProgramInstance * program, const WASMModuleInstanceCommon * module_inst);

void
wasm_program_remove_module_inst_from_name_hmap(WASMProgramInstance * program, WASMModuleInstance * module_inst);

void
wasm_program_cache_resolve_result(WASMProgramInstance * program, int32 id, void * result_func, void * module_inst);

void *
wasm_program_lookup_cached_resolving_func(WASMProgramInstance * program, int32 id);

void
wasm_program_invalidate_cached_wasm_func(WASMProgramInstance * program, wasm_module_inst_t module_inst);

uint32
wasm_program_alloc_module_instance_id(WASMProgramInstance * program, WASMModuleInstanceCommon * module_inst);

void
wasm_program_free_module_instance_id(WASMProgramInstance * program, uint32 inst_id);

WASMModuleInstanceCommon *
wasm_program_get_module_inst_by_id(WASMProgramInstance * program, uint32 inst_idx);

WASMModuleInstanceCommon *
wasm_program_get_module_inst_by_name(WASMProgramInstance * program, const ConstStrDescription * module_name);

WASMModuleInstanceCommon *
wasm_program_get_dep_module_inst_by_name(WASMModuleInstanceCommon * caller_module_inst, const ConstStrDescription * module_name);

bool
wasm_program_insert_module_inst_by_name(WASMProgramInstance * program,
                                        WASMModuleInstanceCommon * module_inst,
                                        const ConstStrDescription * module_name);

WASMModuleInstanceCommon *
wasm_program_instantiate_dependencies(WASMRuntime * runtime,
                                WASMProgramInstance * program_inst,
                                WASMModuleInstanceCommon * caller_module_inst,
                                WASMModuleCommon * module);

void
wasm_program_close_dependencies(wasm_module_inst_t module_inst,
                                uint32 inst_id);

WASMModuleInstanceCommon *
wasm_program_open_dependencies_general(WASMRuntime * runtime,
                                WASMProgramInstance * program_inst,
                                WASMModuleInstanceCommon * caller_module_inst,
                                const char * path);
uint32
wasm_program_open_dependencies(wasm_module_inst_t module_inst,
                                const char * path);

uint32
wasm_program_lookup_symbol_from_module(wasm_module_inst_t caller_module,
                                        uint32 module_handle, const char * symbol);


bool
wasm_program_resolve_op_call(WASMProgramInstance * program,
                            WASMModuleInstance * caller_module_inst,
                            WASMModuleInstance ** p_callee_module_inst,
                            WASMFunctionInstance ** p_call_func);

bool
wasm_program_resolve_aot_function(WASMProgramInstance * program,
                            AOTModuleInstance * caller_module_inst,
                            AOTModuleInstance ** p_callee_module_inst,
                            uint32 import_func_id);

uint32
wasm_program_alloc_table_space_by_size(uint32 inst_id,
                            uint32 needed_size);

uint32
wasm_program_alloc_table_space_by_table(WASMModuleInstance * module_inst,
                        WASMTableInstance * table);

bool
wasm_program_link_sp_wasm_globals(WASMProgramInstance * program,
                                WASMGlobalInstance * global,
                                const ConstStrDescription * field_name);

bool
wasm_program_link_got_globals(WASMProgramInstance * program,
                                WASMModuleInstance * module_inst,
                                WASMGlobalInstance * global,
                                const ConstStrDescription * field_name);

#if WASM_ENABLE_AOT != 0
bool
wasm_program_link_sp_aot_globals(WASMProgramInstance * program,
                                uint8 * p_global_data,
                                const ConstStrDescription * global_name);

bool
wasm_program_link_aot_got_globals(WASMProgramInstance * program,
                                AOTModuleInstance * module_inst,
                                uint8 * p_global_data,
                                const ConstStrDescription * field_name);
#endif

bool
wasm_program_resolve_op_call_indirect(WASMProgramInstance * program,
                                    WASMModuleInstance * module_inst,
                                    uint32 tbl_idx,
                                    int32 tbl_slot_id,
                                    WASMType * func_type,
                                    WASMModuleInstance ** p_callee_module_inst,
                                    WASMFunctionInstance ** p_call_func);
#if WASM_ENABLE_AOT != 0
bool
wasm_program_resolve_aot_op_call_indirect(WASMExecEnv *exec_env,
                                    WASMProgramInstance * program,
                                    AOTModuleInstance * module_inst,
                                    uint32 tbl_idx,
                                    int32 table_elem_idx,
                                    uint32 expected_type_idx,
                                    AOTFuncType * expected_type,
                                    AOTModuleInstance ** p_callee_module_inst,
                                    AOTModule ** p_callee_module,
                                    uint32 * p_call_func_index,
                                    AOTImportFunc ** p_import_func);
#endif

bool
wasm_program_validate_mode_compatiability(WASMProgramInstance * program);

WASMModuleCommon *
load_implicit_dependency_module(const WASMModuleCommon *parent_module,
                                const ConstStrDescription * key,
                                char * error_buf,
                                uint32 error_buf_size);

#endif
#ifdef __cplusplus
}
#endif

#endif
