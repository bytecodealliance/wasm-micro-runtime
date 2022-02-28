#include "bh_common.h"
#include "bh_assert.h"
#include "bh_log.h"
#include "wasm_multimodules_program.h"
#if WASM_ENABLE_INTERP != 0
#include "../interpreter/wasm_runtime.h"
#endif
#if WASM_ENABLE_AOT != 0
#include "../aot/aot_runtime.h"
#endif

#define RUNTIME_CONST_STR_POOL_INIT_SIZE 128
#define RUNTIME_NAME_MODULE_MAP_INIT_SIZE 32
#define PROGRAM_NAME_MODULE_INST_INIT_SIZE 16
#define PROGRAM_INST_IDX_VECTOR_INIT_SIZE 64
#define PROGRAM_INST_ID_HMAP_INIT_SIZE 32
#define PROGRAM_INST_ID_TOP_BOUNARY MAX_INST_ID

static WASMRuntime * g_runtime = NULL;

inline static uint32
inst_id_hash(void * node)
{
    uint32 h = ((uintptr_t)node) & (PROGRAM_INST_ID_HMAP_INIT_SIZE - 1);

    return h;
}

inline static bool
inst_id_equal(void * node1, void * node2)
{
    return ((uintptr_t)node1 == (uintptr_t)node2);
}

static void
set_error_buf(char *error_buf, uint32 error_buf_size, const char *string)
{
    if (error_buf != NULL)
        snprintf(error_buf, error_buf_size, "%s", string);
}

#if WASM_ENABLE_DYNAMIC_LINKING != 0
static void
set_error_buf_v(char *error_buf, uint32 error_buf_size,
                const char *format, ...)
{
    va_list args;
    char buf[128];

    if (error_buf != NULL) {
        va_start(args, format);
        vsnprintf(buf, sizeof(buf), format, args);
        va_end(args);
        snprintf(error_buf, error_buf_size,
                 "WASM module instantiate failed: %s", buf);
    }
}
#endif

static void *
runtime_malloc(uint64 size, char *error_buf, uint32 error_buf_size)
{
    void *mem;

    if (size >= UINT32_MAX
        || !(mem = wasm_runtime_malloc((uint32)size))) {
        set_error_buf(error_buf, error_buf_size,
                      "allocate memory failed");
        return NULL;
    }

    memset(mem, 0, (uint32)size);
    return mem;
}

inline static WASMModuleInstanceCommon *
dylib_entries_map_find(const ConstStrDescription * key, HashMap *map)
{
    return bh_hash_map_find(map, (void*)key);
}

inline static bool
dylib_entries_map_insert(const ConstStrDescription * key,
                        const WASMModuleInstanceCommon * module_inst,
                        HashMap *map)
{
    //char *value;
    //ConstStrDescription * key = NULL;

    //if ((bh_hash_map_find(map, (void*)tmp_key))) {
    //    return true;
    //}

    //if (!(key = runtime_malloc(sizeof(ConstStrDescription),
    //                            NULL, 0))) {
    //    return false;
    //}

    //bh_memcpy_s(key, sizeof(ConstStrDescription), tmp_key, sizeof(ConstStrDescription));

    if (!bh_hash_map_insert_with_dup(map, (void*)key, (void*)module_inst)) {
        // wasm_runtime_free(key);
        return false;
    }

    return true;
}

void
wasm_runtime_set_module_reader(const module_reader reader_cb,
                               const module_destroyer destroyer_cb)
{
    g_runtime->reader = (void*)reader_cb;
    g_runtime->destroyer = (void*)destroyer_cb;
}

module_reader
wasm_runtime_get_module_reader()
{
    return (module_reader)g_runtime->reader;
}

module_destroyer
wasm_runtime_get_module_destroyer()
{
    return (module_destroyer)g_runtime->destroyer;
}

bool
init_const_string_index_array(WASMRuntime * runtime)
{
    ConstStrDescription * key = NULL;
    const char * c_str = NULL;
    uint32 index_array_size = WAMR_CSP_SYMBOLS_end;
    uint32 i = 0;

    runtime->global_const_str_index_array =
                wasm_runtime_malloc(sizeof(ConstStrDescription) * index_array_size);
    if (!runtime->global_const_str_index_array) {
        return false;
    }

    memset(runtime->global_const_str_index_array, 0, sizeof(ConstStrDescription) * index_array_size);

    c_str = wasm_init_symbols;
    for (i = 0; i < WAMR_CSP_SYMBOLS_end; i ++) {
        key = &runtime->global_const_str_index_array[i];

        key->len = strlen(c_str);
        key->str = c_str;
        key->hash = 0;
        key->is_sys_symbol = true;

        if (!bh_hash_map_insert_with_dup(runtime->global_const_str_pool, key, (void*)key))
            return false;

        c_str += (strlen(c_str) + 1);
    }

    runtime->csp_free_index = i;
    runtime->csp_strings_count = i;
    runtime->csp_size = index_array_size;

    return true;
}

bool
wasm_runtime_is_system_symbol(WASMRuntime * runtime, const ConstStrDescription * key)
{
    if (key >= &runtime->global_const_str_index_array[0] &&
        key <= &runtime->global_const_str_index_array[WAMR_CSP_SYMBOLS_end - 1])
        return true;

    return false;
}

bool
wasm_runtime_is_memop_symbol(WASMRuntime * runtime, const ConstStrDescription * key)
{
    if (key == CONST_STR_POOL_DESC(runtime, WAMR_CSP_malloc) ||
        key == CONST_STR_POOL_DESC(runtime, WAMR_CSP_free) ||
        key == CONST_STR_POOL_DESC(runtime, WAMR_CSP_realloc))
        return true;

    return false;
}

uint32
wasm_runtime_get_syssymbol_id(WASMRuntime * runtime, const ConstStrDescription * key)
{
    return (uint32)(key - runtime->global_const_str_index_array);
}

WASMModuleCommon *
wasm_runtime_get_module_by_name(WASMRuntime * runtime, const ConstStrDescription * module_name)
{
    WASMModuleCommon * module = NULL;
    module = bh_hash_map_find(runtime->all_loaded_modules, (void*)module_name);
    return module;
}

bool
wasm_runtime_const_str_pool_init(WASMRuntime * runtime)
{
    if (!runtime)
        return false;

    runtime->global_const_str_pool =
            bh_hash_map_create(RUNTIME_CONST_STR_POOL_INIT_SIZE,
                                false,
                                (HashFunc)const_str_hash,
                                (KeyEqualFunc)const_str_equal,
                                (ValueDestroyFunc)const_str_destroy_key,
                                NULL);

    if (!runtime->global_const_str_pool)
        return false;

    if (!init_const_string_index_array(runtime)) {
        bh_hash_map_destroy(runtime->global_const_str_pool);
        return false;
    }

    return true;
}

void
wasm_runtime_const_str_pool_destroy(WASMRuntime * runtime)
{
    if (!runtime)
        return;

    if (runtime->global_const_str_pool)
        bh_hash_map_destroy(runtime->global_const_str_pool);

    if (runtime->global_const_str_index_array)
        wasm_runtime_free(runtime->global_const_str_index_array);

    runtime->global_const_str_pool = NULL;
    runtime->global_const_str_index_array = NULL;
    runtime->csp_size = 0;
    runtime->csp_free_index = 0;
    runtime->csp_strings_count = 0;
}

uint32
records_const_string_index(WASMRuntime * runtime,
                            const ConstStrDescription * csp_desc)
{
#if 0
    uint32 ret_index = 0;
    uint32 new_size = 0;

    if (runtime->csp_strings_count >= runtime->csp_size) {
        new_size = (runtime->csp_size * 3) / 2;
        runtime->global_const_str_index_array =
                        wasm_runtime_realloc(runtime->global_const_str_index_array,
                                            sizeof(ConstStrDescription*) * new_size);

        if (!runtime->global_const_str_index_array)
            return 0;

        runtime->csp_size = new_size;
    }

    bh_assert(runtime->csp_strings_count < runtime->csp_size);

    ret_index = runtime->csp_free_index;
    runtime->global_const_str_index_array[ret_index] = (ConstStrDescription *)csp_desc;
    runtime->csp_strings_count ++;
    runtime->csp_free_index ++ ;

    return ret_index;
#endif
    return 0;
}

static bool
check_utf8_str(const uint8* str, uint32 len)
{
    /* The valid ranges are taken from page 125, below link
       https://www.unicode.org/versions/Unicode9.0.0/ch03.pdf */
    const uint8 *p = str, *p_end = str + len;
    uint8 chr;

    while (p < p_end) {
        chr = *p;
        if (chr < 0x80) {
            p++;
        }
        else if (chr >= 0xC2 && chr <= 0xDF && p + 1 < p_end) {
            if (p[1] < 0x80 || p[1] > 0xBF) {
                return false;
            }
            p += 2;
        }
        else if (chr >= 0xE0 && chr <= 0xEF && p + 2 < p_end) {
            if (chr == 0xE0) {
                if (p[1] < 0xA0 || p[1] > 0xBF
                    || p[2] < 0x80 || p[2] > 0xBF) {
                    return false;
                }
            }
            else if (chr == 0xED) {
                if (p[1] < 0x80 || p[1] > 0x9F
                    || p[2] < 0x80 || p[2] > 0xBF) {
                    return false;
                }
            }
            else if (chr >= 0xE1 && chr <= 0xEF) {
                if (p[1] < 0x80 || p[1] > 0xBF
                    || p[2] < 0x80 || p[2] > 0xBF) {
                    return false;
                }
            }
            p += 3;
        }
        else if (chr >= 0xF0 && chr <= 0xF4 && p + 3 < p_end) {
            if (chr == 0xF0) {
                if (p[1] < 0x90 || p[1] > 0xBF
                    || p[2] < 0x80 || p[2] > 0xBF
                    || p[3] < 0x80 || p[3] > 0xBF) {
                    return false;
                }
            }
            else if (chr >= 0xF1 && chr <= 0xF3) {
                if (p[1] < 0x80 || p[1] > 0xBF
                    || p[2] < 0x80 || p[2] > 0xBF
                    || p[3] < 0x80 || p[3] > 0xBF) {
                    return false;
                }
            }
            else if (chr == 0xF4) {
                if (p[1] < 0x80 || p[1] > 0x8F
                    || p[2] < 0x80 || p[2] > 0xBF
                    || p[3] < 0x80 || p[3] > 0xBF) {
                    return false;
                }
            }
            p += 4;
        }
        else {
            return false;
        }
    }
    return (p == p_end);
}

const ConstStrDescription *
wasm_runtime_records_const_filename_string(WASMRuntime * runtime,
                                const char * str, const uint32 len,
                                char* error_buf, uint32 error_buf_size)
{
    int no_postfix_len = len;

    for (int i = len - 1; i >= 0; i --) {
        if (str[i] == '.') {
            no_postfix_len = i;
            break;
        }
    }

    if (!no_postfix_len)
        no_postfix_len = len;

    return wasm_runtime_records_const_string(runtime, str, no_postfix_len,
                                            error_buf, error_buf_size);
}

const ConstStrDescription *
wasm_runtime_records_const_string(WASMRuntime * runtime,
                                const char * str, const uint32 len,
                                char* error_buf, uint32 error_buf_size)
{
    HashMap * pool = NULL;
    char *c_str, *value;
    ConstStrDescription temp_key;
    ConstStrDescription * key = NULL;
    uint32 req_size = 0;

    if (!runtime)
        return NULL;

    pool = runtime->global_const_str_pool;
    if (!pool)
        return NULL;

    if (!check_utf8_str((const uint8 *)str, len)) {
        set_error_buf(error_buf, error_buf_size,
                      "invalid UTF-8 encoding");
        return NULL;
    }

    bh_assert(len < (UINT_MAX/2));
    temp_key.str = (void*)str;
    temp_key.is_sys_symbol = false;
    temp_key.len = len;
    temp_key.hash = 0;

    if ((value = bh_hash_map_find(pool, &temp_key))) {
        return (ConstStrDescription *)value;
    }

    req_size = sizeof(ConstStrDescription) + (uint32)len + 1;

    if (!(key = runtime_malloc(req_size, error_buf, error_buf_size))) {
        return NULL;
    }

    memset(key, 0, req_size);

    //c_str = key->data;
    c_str = (char*)(key + 1);
    bh_memcpy_s(c_str, (uint32)(len + 1), str, (uint32)len);

    key->str = c_str;
    key->len = len;
    key->hash = temp_key.hash;
    key->is_sys_symbol = false;

    if (!bh_hash_map_insert(pool, key, key)) {
        set_error_buf(error_buf, error_buf_size,
                      "failed to insert string to hash map");
        wasm_runtime_free(key);
        return NULL;
    }

    return key;
}

WASMRuntime *
wasm_runtime_get_runtime()
{
    return g_runtime;
}

bool
wasm_runtime_runtime_init(bool standalone, bool auto_ext_name)
{
    if (g_runtime)
        return false;

    WASMRuntime * runtime = NULL;
    runtime = wasm_runtime_malloc(sizeof(WASMRuntime));
    if (!runtime)
        return false;

    memset(runtime, 0, sizeof(WASMRuntime));

    runtime->config.need_load_dependencies = !standalone;
    runtime->config.auto_update_extension = auto_ext_name;
#if WASM_ENABLE_DYNAMIC_LINKING != 0
    runtime->cur_loading_program = NULL;
#endif
    runtime->reader = NULL;
    runtime->destroyer = NULL;
    runtime->all_loaded_modules = NULL;

    if (!standalone) {
        runtime->all_loaded_modules =
                bh_hash_map_create(RUNTIME_NAME_MODULE_MAP_INIT_SIZE,
                                    false,
                                    (HashFunc)const_str_hash,
                                    (KeyEqualFunc)const_str_equal,
                                    NULL,
                                    (ValueDestroyFunc)const_str_destroy_module);
        if (!runtime->all_loaded_modules) {
            wasm_runtime_free(runtime);
            return false;
        }
    }

    if (!wasm_runtime_const_str_pool_init(runtime)) {
        wasm_runtime_runtime_destroy();
        return false;
    }

    g_runtime = runtime;
    return true;
}

void
wasm_runtime_runtime_destroy()
{
    if (!g_runtime)
        return;

    if (g_runtime->all_loaded_modules)
        bh_hash_map_destroy(g_runtime->all_loaded_modules);

    wasm_runtime_const_str_pool_destroy(g_runtime);

    wasm_runtime_free(g_runtime);
    g_runtime = NULL;
}

// shared module means built with -Wl,--shared.
// dependency module means opened explicitly or implicitly by other module.
// root module means the first module instantiated by a program.
bool
wasm_runtime_is_shared_module(const WASMModuleCommon * module)
{
    WASMDylinkSection * dylink_section = NULL;

    if (module->module_type == Wasm_Module_Bytecode) {
        dylink_section = ((WASMModule*)module)->dylink_section;
    } else {
#if WASM_ENABLE_AOT != 0
        dylink_section = ((AOTModule*)module)->dylink_section;
#endif
    }

    if (dylink_section)
        return true;

    return false;
}

bool
wasm_runtime_is_shared_module_instance(const WASMModuleInstanceCommon * module_inst)
{
    WASMDylinkSection * dylink_section = NULL;

    if (module_inst->module_type == Wasm_Module_Bytecode) {
        WASMModule * module = (WASMModule *)(((WASMModuleInstance*)module_inst)->module);
        dylink_section = (module)->dylink_section;
    } else {
#if WASM_ENABLE_AOT != 0
        AOTModule * module = (AOTModule*)(((AOTModuleInstance*)module_inst)->aot_module.ptr);
        dylink_section = (module)->dylink_section;
#endif
    }

    if (dylink_section)
        return true;

    return false;
}

#if WASM_ENABLE_DYNAMIC_LINKING != 0
inline bool
wasm_program_is_root_module(const WASMModuleInstanceCommon * module_inst)
{
    const WASMModuleInstanceHead * module_inst_head = (WASMModuleInstanceHead *)module_inst;
    if (module_inst_head->program) {
        if (module_inst_head->program->root_module_inst)
            return module_inst_head->program->root_module_inst == module_inst;
        else
            return true;
    } else
        return true;
}

inline WASMModuleInstanceCommon *
wasm_program_get_root_module_from_inst(const WASMModuleInstanceCommon * module_inst)
{
    WASMProgramInstance * program = ((WASMModuleInstanceHead*)module_inst)->program;
    if (!program)
        return (WASMModuleInstanceCommon *)module_inst;

    return program->root_module_inst;
}

inline WASMModuleInstanceCommon *
wasm_program_get_root_module(const WASMProgramInstance * program)
{
    return program->root_module_inst;
}

inline void
wasm_program_set_root_module(WASMProgramInstance * program, const WASMModuleInstanceCommon * module_inst)
{
    program->root_module_inst = (WASMModuleInstanceCommon*)module_inst;
}

bool
wasm_program_validate_mode_compatiability(WASMProgramInstance * program)
{
    WASMRuntime * runtime = program->runtime;
    WASMModuleInstanceCommon * root_module_inst = program->root_module_inst;
    const char * symbol_name = NULL;
    bool malloc_exist = false;
    bool free_exist = false;
    bool realloc_exist = false;
    bool export_sp_exist = false;
    WASMModuleInstance * wasm_module_inst = NULL;
#if WASM_ENABLE_AOT != 0
    AOTModuleInstance * aot_module_inst = NULL;
#endif

    if (root_module_inst->module_type == Wasm_Module_Bytecode) {
        wasm_module_inst = (WASMModuleInstance *)root_module_inst;

        for (uint32 i = 0; i < wasm_module_inst->export_func_count; i ++) {
            symbol_name = wasm_module_inst->export_functions[i].name;
            if (symbol_name) {
                if (!strcmp(symbol_name, CONST_STR_POOL_STR(runtime, WAMR_CSP_malloc)))
                    malloc_exist = true;
                else if (!strcmp(symbol_name, CONST_STR_POOL_STR(runtime, WAMR_CSP_free)))
                    free_exist = true;
                else if (!strcmp(symbol_name, CONST_STR_POOL_STR(runtime, WAMR_CSP_realloc)))
                    realloc_exist = true;

                if (program->config.root_is_AS_module) {
                    if (!strcmp(symbol_name, CONST_STR_POOL_STR(runtime, WAMR_CSP___alloc)))
                        malloc_exist = true;
                    else if (!strcmp(symbol_name, CONST_STR_POOL_STR(runtime, WAMR_CSP___free)))
                        free_exist = true;
                    else if (!strcmp(symbol_name, CONST_STR_POOL_STR(runtime, WAMR_CSP___realloc)))
                        realloc_exist = true;
                }

                if (malloc_exist && free_exist && realloc_exist)
                    break;
            }
        }

        for (uint32 i = 0; i < wasm_module_inst->export_glob_count; i++) {
            symbol_name = wasm_module_inst->export_globals[i].name;
            if (!program->config.root_is_AS_module) {
                if (!strcmp(symbol_name, CONST_STR_POOL_STR(runtime, WAMR_CSP_var_stack_pointer)))
                    export_sp_exist = true;
            } else {
                if (!strcmp(symbol_name, CONST_STR_POOL_STR(runtime, WAMR_CSP_var_user_stack_pointer)))
                    export_sp_exist = true;
            }

            if (export_sp_exist)
                break;
        }

    } else {
#if WASM_ENABLE_AOT != 0
        aot_module_inst = (AOTModuleInstance *)root_module_inst;
        AOTModule * aot_module = (AOTModule*)(aot_module_inst->aot_module.ptr);
        uint32 export_func_count = aot_module->export_func_count;
        uint32 exports_count = aot_module->export_count;

        AOTExportFunctionInstance * export_funcs = aot_module_inst->export_funcs.ptr;

        for (uint32 i = 0; i < export_func_count; i ++) {
            symbol_name = export_funcs[i].func_name;
            if (symbol_name) {
                if (!strcmp(symbol_name, CONST_STR_POOL_STR(runtime, WAMR_CSP_malloc)))
                    malloc_exist = true;
                else if (!strcmp(symbol_name, CONST_STR_POOL_STR(runtime, WAMR_CSP_free)))
                    free_exist = true;
                else if (!strcmp(symbol_name, CONST_STR_POOL_STR(runtime, WAMR_CSP_realloc)))
                    realloc_exist = true;

                if (program->config.root_is_AS_module) {
                    if (!strcmp(symbol_name, CONST_STR_POOL_STR(runtime, WAMR_CSP___alloc)))
                        malloc_exist = true;
                    else if (!strcmp(symbol_name, CONST_STR_POOL_STR(runtime, WAMR_CSP___free)))
                        free_exist = true;
                    else if (!strcmp(symbol_name, CONST_STR_POOL_STR(runtime, WAMR_CSP___realloc)))
                        realloc_exist = true;
                }

                if (malloc_exist && free_exist && realloc_exist)
                    break;
            }
        }

        for (uint32 i = 0; i < exports_count; i++) {
            if (aot_module->exports[i].kind != EXPORT_KIND_GLOBAL)
                continue;

            symbol_name = aot_module->exports[i].name;
            if (!program->config.root_is_AS_module) {
                if (!strcmp(symbol_name, CONST_STR_POOL_STR(runtime, WAMR_CSP_var_stack_pointer)))
                    export_sp_exist = true;
            } else {
                if (!strcmp(symbol_name, CONST_STR_POOL_STR(runtime, WAMR_CSP_var_user_stack_pointer)))
                    export_sp_exist = true;
            }

            if (export_sp_exist)
                break;
        }

#endif
    }


    if (program->config.import_memop_mode == FROM_ROOT) {
        if (!malloc_exist || !free_exist || !realloc_exist) {
            LOG_WARNING("Select memory from root module, but not found the functions exported:");
            if (!malloc_exist)
                LOG_WARNING("malloc");
            if (!free_exist)
                LOG_WARNING("free");
            if (!realloc_exist)
                LOG_WARNING("realloc");

            return false;
        }
    } else {
        if (malloc_exist || free_exist || realloc_exist) {
            LOG_WARNING("Select memory from VM, but found memory functions exported:");
            if (malloc_exist)
                LOG_WARNING("malloc");
            if (free_exist)
                LOG_WARNING("free");
            if (realloc_exist)
                LOG_WARNING("realloc");
        }
        return false;
    }

    if (!export_sp_exist) {
        LOG_WARNING("Can't find global __stack_pointer exported, link error might happen.");
        return false;
    }

    return true;
}

WASMProgramInstance *
wasm_runtime_create_program_internal(char * error_buf, uint32 error_buf_size, uint32 dlopen_mode)
{
    WASMProgramInstance * program = NULL;
    if (!(program = runtime_malloc(sizeof(WASMProgramInstance), error_buf, error_buf_size)))
        return NULL;

    program->clean_all = false;
    program->config.binding_mode = LAZY_BINDING; // currently always lazy binding, ignore user setting
    program->config.import_memop_mode = (dlopen_mode & MEM_ALLOCATOR_MASK) ? FROM_ROOT: FROM_BUILTIN_LIBC;
    program->config.use_tbl_as_cache = (dlopen_mode & USE_TBL_AS_CACHE_MASK);
    program->config.root_is_AS_module = (dlopen_mode & ROOT_IS_AS_MODULE_MASK);
    program->config.use_resolve_cache = false; // currently alwasy false, no optimiation implemented.

    program->runtime = wasm_runtime_get_runtime();
    // bh_list_init(&program->loading_modules_list);

    program->resolving_cache = runtime_malloc(sizeof(wasm_resolving_cache_entry) * PROGRAM_RESOLVING_CACHE_LINE_LEN * PROGRAM_RESOLVING_CACHE_LINE_COUNT,
                                        error_buf, error_buf_size);
    if (!program->resolving_cache) {
        wasm_runtime_free(program);
        return NULL;
    }

    // create a local map to record dependencies.
    // hash map is not responsible to reclaim the key
    // they are allocated/free by const string pool.
    program->global_modules_inst_name_hmap =
                                bh_hash_map_create(PROGRAM_NAME_MODULE_INST_INIT_SIZE,
                                                false,
                                                (HashFunc)const_str_hash,
                                                (KeyEqualFunc)const_str_equal,
                                                NULL,
                                                (ValueDestroyFunc)const_str_destroy_module_inst);
    if (!program->global_modules_inst_name_hmap) {
        wasm_runtime_free(program->resolving_cache);
        wasm_runtime_free(program);
        return NULL;
    }

    program->global_modules_inst_id_hmap =
                        bh_hash_map_create(PROGRAM_INST_ID_HMAP_INIT_SIZE,
                                        false,
                                        (HashFunc)inst_id_hash,
                                        (KeyEqualFunc)inst_id_equal,
                                        NULL,
                                        NULL);
    if (!program->global_modules_inst_id_hmap) {
        bh_hash_map_destroy(program->global_modules_inst_name_hmap);
        wasm_runtime_free(program->resolving_cache);
        wasm_runtime_free(program);
        return NULL;
    }
    // index 0 is reserved for error handling.
    program->next_free_inst_id = 1;
    program->builtin_libc_funcs = NULL;
    program->builtin_libc_funcs_count = 0;
    program->builtin_libc_funcs_size = 0;

    program->exception_inst = NULL;
    program->error_buf = error_buf;
    program->error_buf_size = error_buf_size;

    return program;
}

void
wasm_program_remove_module_inst_from_name_hmap(WASMProgramInstance * program, WASMModuleInstance * module_inst)
{
    bh_hash_map_remove(program->global_modules_inst_name_hmap, (void*)module_inst->module->module_name, NULL, NULL);
}

void
wasm_program_cache_resolve_result(WASMProgramInstance * program, int32 id, void * result_func, void * module_inst)
{
    wasm_resolving_cache_entry * cache_line = NULL;
    wasm_resolving_cache_entry * p_empty_entry = NULL;
    int32 index = id & (PROGRAM_RESOLVING_CACHE_LINE_COUNT - 1);
    if (!program)
        return;

    cache_line = program->resolving_cache + (PROGRAM_RESOLVING_CACHE_LINE_LEN * index);

    if (!cache_line[0].func_inst) {
        p_empty_entry = &cache_line[0];
    } else if (!cache_line[1].func_inst)
        p_empty_entry = &cache_line[1];
    else
        p_empty_entry = &cache_line[0];

    p_empty_entry->index = id;
    p_empty_entry->func_inst = result_func;
#if WASM_ENABLE_AOT != 0
    p_empty_entry->module_inst = module_inst;
#endif
}

void *
wasm_program_lookup_cached_resolving_func(WASMProgramInstance * program, int32 id)
{
    wasm_resolving_cache_entry * cache_line = NULL;
    int32 index = id & (PROGRAM_RESOLVING_CACHE_LINE_COUNT - 1);

    if (!program)
        return NULL;

    cache_line = program->resolving_cache + (PROGRAM_RESOLVING_CACHE_LINE_LEN * index);

    if (cache_line[0].index == id)
        return cache_line[0].func_inst;
    else if (cache_line[1].index == id)
        return cache_line[1].func_inst;

    return NULL;
}

void
wasm_program_invalidate_cached_wasm_func(WASMProgramInstance * program, wasm_module_inst_t module_inst)
{
    if (!program)
        return;

    wasm_resolving_cache_entry * cache_entry = program->resolving_cache;
    if (!module_inst)
        return;

    for (int i = 0; i < PROGRAM_RESOLVING_CACHE_LINE_LEN * PROGRAM_RESOLVING_CACHE_LINE_COUNT; i ++) {
        WASMFunctionInstance * func_inst = cache_entry[i].func_inst;
        if (!func_inst)
            continue;

        if ((wasm_module_inst_t)func_inst->module_inst != module_inst)
            continue;

        memset(&cache_entry[i], 0, sizeof(wasm_resolving_cache_entry));
    }
}

uint32
wasm_program_create_dlopen_session(WASMProgramInstance * program, WASMModuleInstanceCommon * module_inst)
{
    WASMModuleInstanceHead * module_inst_head = (WASMModuleInstanceHead *)module_inst;
    module_inst_head->exp_ref_cnt ++;

    //if (module_inst->module_type == Wasm_Module_Bytecode) {
    //    WASMModuleInstance * wasm_module_inst = (WASMModuleInstance *)module_inst;
    //    printf("module %s, exp ref = %d, imp ref = %d\n", wasm_module_inst->module->module_name->str,
    //        wasm_module_inst->exp_ref_cnt, wasm_module_inst->imp_ref_cnt);
    //} else {
    //    AOTModuleInstance * aot_module_inst = (AOTModuleInstance *)module_inst;
    //    printf("module %s, exp ref = %d, imp ref = %d\n", ((AOTModule*)(aot_module_inst->aot_module.ptr))->module_name->str,
    //        aot_module_inst->exp_ref_cnt, aot_module_inst->imp_ref_cnt);
    //}

    return module_inst_head->inst_id;
}

WASMModuleInstanceCommon *
wasm_program_destroy_dlopen_session(WASMProgramInstance * program, uint32 inst_id)
{
    WASMModuleInstanceCommon * module_inst = wasm_program_get_module_inst_by_id(program, inst_id);
    WASMModuleInstanceHead * module_inst_head = (WASMModuleInstanceHead *)module_inst;
    if (!module_inst)
        return NULL;

    if (module_inst_head->exp_ref_cnt > 0)
        module_inst_head->exp_ref_cnt -- ;

    //if (module_inst->module_type == Wasm_Module_Bytecode)
    //    printf("module %s, exp ref = %d, imp ref = %d\n", ((WASMModuleInstance*)module_inst)->module->module_name->str,
    //        module_inst_head->exp_ref_cnt, module_inst_head->imp_ref_cnt);
    //else
    //    printf("module %s, exp ref = %d, imp ref = %d\n", ((AOTModule*)((AOTModuleInstance*)module_inst)->aot_module.ptr)->module_name->str,
    //        module_inst_head->exp_ref_cnt, module_inst_head->imp_ref_cnt);

    return module_inst;
}

uint32
wasm_program_alloc_module_instance_id(WASMProgramInstance * program, WASMModuleInstanceCommon * module_inst)
{
    uint32 id = 0;
    void * p = NULL;
    WASMModuleInstanceHead * module_inst_head = (WASMModuleInstanceHead *)module_inst;

    id = program->next_free_inst_id;

    p = (void*)(uintptr_t)id;

    while (bh_hash_map_find(program->global_modules_inst_id_hmap, p)) {
        id ++;
        if (id == PROGRAM_INST_ID_TOP_BOUNARY)
            id = 2;
        p = (void*)(uintptr_t)id;
    }

    if (!bh_hash_map_insert(program->global_modules_inst_id_hmap, p, (void*)module_inst))
        return 0;

    program->next_free_inst_id = id + 1;
    if (program->next_free_inst_id == PROGRAM_INST_ID_TOP_BOUNARY)
        program->next_free_inst_id = 2;

    module_inst_head->inst_id = id;

    return id;
}

void
wasm_program_free_module_instance_id(WASMProgramInstance * program, uint32 inst_id)
{
    void * key = (void*)(uintptr_t)inst_id;
    bh_hash_map_remove(program->global_modules_inst_id_hmap, key, NULL, NULL);
}

WASMModuleInstanceCommon *
wasm_program_get_module_inst_by_id(WASMProgramInstance * program, uint32 inst_idx)
{
    WASMModuleInstanceCommon * module_inst = NULL;
    void * key = (void*)(uintptr_t)inst_idx;

    module_inst = bh_hash_map_find(program->global_modules_inst_id_hmap, key);
    if (!module_inst) {
        return NULL;
    }

    return module_inst;
}

WASMModuleInstanceCommon *
wasm_program_get_module_inst_by_name(WASMProgramInstance * program, const ConstStrDescription * module_name)
{
    WASMModuleInstanceCommon * module_inst = NULL;
    if (module_name == CONST_STR_POOL_DESC(program->runtime, WAMR_CSP_env))
        return NULL;

    module_inst = bh_hash_map_find(program->global_modules_inst_name_hmap, (void*)module_name);
    return module_inst;
}

WASMModuleInstanceCommon *
wasm_program_get_dep_module_inst_by_name(WASMModuleInstanceCommon * caller_module_inst, const ConstStrDescription * module_name)
{
    WASMModuleInstanceCommon * module_inst = NULL;
    WASMModuleInstanceHead * caller_module_inst_head = (WASMModuleInstanceHead *)caller_module_inst;

    if (module_name == CONST_STR_POOL_DESC(caller_module_inst_head->runtime, WAMR_CSP_env))
        return NULL;

    if (caller_module_inst->module_type == Wasm_Module_Bytecode)
        module_inst = bh_hash_map_find(
            caller_module_inst_head->local_implicit_dependency_modules_name_hmap,
            (void*)module_name);
    else {
        module_inst = bh_hash_map_find(
            caller_module_inst_head->local_implicit_dependency_modules_name_hmap,
            (void*)module_name);
    }
    return module_inst;
}

bool
wasm_program_insert_module_inst_by_name(WASMProgramInstance * program,
                                        WASMModuleInstanceCommon * module_inst,
                                        const ConstStrDescription * module_name)
{
    return bh_hash_map_insert_with_dup(program->global_modules_inst_name_hmap,
                                (void*)module_name,
                                (void*)module_inst);
}

#if WASM_ENABLE_LIBC_BUILTIN != 0
static void
wasm_program_destroy_internal_libc_module(WASMProgramInstance * program);
#endif

void
wasm_runtime_destroy_program_internal(WASMProgramInstance * program)
{
    program->clean_all = true;

#if WASM_ENABLE_LIBC_BUILTIN != 0
    wasm_program_destroy_internal_libc_module(program);
#endif

    if (program->resolving_cache)
        wasm_runtime_free(program->resolving_cache);

    if (program->global_modules_inst_name_hmap)
        bh_hash_map_destroy(program->global_modules_inst_name_hmap);

    if (program->global_modules_inst_id_hmap)
        bh_hash_map_destroy(program->global_modules_inst_id_hmap);

    wasm_runtime_free(program);
}

const ConstStrDescription *
upgrade_module_extension(const WASMRuntime *runtime,
                        const ConstStrDescription * key_module_name,
                        const package_type_t expected_module_type,
                        char * error_buf,
                        uint32 error_buf_size)
{
    uint32 offset = 0, new_name_len = 0;
    const ConstStrDescription * key_new_module_name = NULL;
    char * extension_name = NULL, *new_module_name = NULL;

    if (!runtime->config.auto_update_extension)
        return key_module_name;

    offset = key_module_name->len;
    extension_name = strrchr(key_module_name->str, '.');
    if (!extension_name)
        return key_module_name;

    offset = extension_name - key_module_name->str;

    new_name_len = offset + sizeof(".wasm") + 1;

    new_module_name = (char*)wasm_runtime_malloc(new_name_len);
    memset(new_module_name, 0, new_name_len);
    memcpy(new_module_name, key_module_name->str, offset);

    if (expected_module_type == Wasm_Module_Bytecode)
        strncat(new_module_name, ".wasm", new_name_len);
    else
        strncat(new_module_name, ".aot", new_name_len);

    key_new_module_name = wasm_runtime_records_const_string((WASMRuntime*)runtime, new_module_name,
            strlen(new_module_name), error_buf, error_buf_size);

    wasm_runtime_free(new_module_name);

    return key_new_module_name;
}

static WASMModuleCommon *
load_dependency_module(const WASMRuntime *runtime,
                    const ConstStrDescription * key,
                    const package_type_t expected_module_type,
                    char * error_buf,
                    uint32 error_buf_size)
{
    WASMModuleCommon * new_module = NULL;
    // bh_list * new_modules_list = NULL;
    const module_reader reader = wasm_runtime_get_module_reader();
    const module_destroyer destroyer = wasm_runtime_get_module_destroyer();
    // LoadingModuleElem * loading_module_elem = NULL;

    if (!reader || !destroyer) {
        return NULL;
    }

    // check if already loaded
    new_module = bh_hash_map_find(runtime->all_loaded_modules, (void*)key);
    if (new_module &&
        new_module->module_type == expected_module_type) {

        if (!wasm_runtime_is_shared_module(new_module))
            return NULL;

        return new_module;
    }

    if (new_module &&
        new_module->module_type != expected_module_type) {

        return NULL; //currently, VM won't replace the extension automatically on user's behalf

        //key = upgrade_module_extension(runtime, key, expected_module_type, error_buf, error_buf_size);

        //new_module = bh_hash_map_find(runtime->all_loaded_modules, (void*)key);
        //if (new_module &&
        //    new_module->module_type == expected_module_type) {
        //    return new_module;
        //}
    }

    bh_assert(!new_module);

    if (!(new_module = load_dependency_module_internal(reader, destroyer, key->str,
        key->len, expected_module_type, error_buf, error_buf_size))) {
        return NULL;
    }

    if (new_module->module_type == Wasm_Module_Bytecode)
        ((WASMModule*)new_module)->module_name = key;
    else
        ((AOTModule*)new_module)->module_name = key;

    if (!bh_hash_map_insert(runtime->all_loaded_modules, (void*)key, new_module)) {
        return NULL;
    }

    return new_module;
}

/**
 * Return export function count in module export section.
 */
static uint32
get_export_count_by_kind(const WASMModuleCommon *module, const uint8 kind)
{
    uint32 count = 0;
    if (module->module_type == Wasm_Module_Bytecode) {
        WASMExport *export = ((WASMModule*)module)->exports;

        for (uint32 i = 0; i < ((WASMModule*)module)->export_count; i++, export++) {
            if (export->kind == kind)
                count ++;
        }
    } else {
        AOTExport * export = ((AOTModule*)module)->exports;

        for (uint32 i = 0; i < ((AOTModule*)module)->export_count; i++, export++) {
            if (export->kind == kind)
                count ++;
        }
    }

    return count;
}

WASMModuleCommon *
load_explicit_dependency_module(const WASMModuleInstanceCommon *parent_module,
                                const ConstStrDescription * key)
{
    WASMModuleInstanceHead * parent_module_inst_head = (WASMModuleInstanceHead *)parent_module;
    WASMRuntime * runtime = parent_module_inst_head->runtime;
    WASMProgramInstance * program = parent_module_inst_head->program;

    // all new loaded modules will be recorded into program, so that
    // can be easy to instantiate them later.
    runtime->cur_loading_program = program;

    return load_dependency_module(runtime, key,
                                parent_module_inst_head->module_type,
                                program->error_buf,
                                program->error_buf_size);
}

WASMModuleCommon *
load_implicit_dependency_module(const WASMModuleCommon *parent_module,
                                const ConstStrDescription * key,
                                char * error_buf,
                                uint32 error_buf_size)
{
    WASMRuntime * runtime = NULL;
    if (parent_module->module_type == Wasm_Module_Bytecode)
        runtime = ((WASMModule*)parent_module)->runtime;
    else {
        runtime = ((AOTModule*)parent_module)->runtime;
        key = upgrade_module_extension(runtime, key, Wasm_Module_AoT, error_buf, error_buf_size);
    }

    return load_dependency_module(runtime, key, parent_module->module_type, error_buf, error_buf_size);
}

void
decrement_ref_module_inst_callback(ConstStrDescription * key, void * value, void * user_data)
{
    ConstStrDescription ** unused_module_list = (ConstStrDescription **)user_data;
    WASMModuleInstanceCommon * module_inst = (WASMModuleInstanceCommon *)value;
    WASMModuleInstanceHead * module_inst_head = (WASMModuleInstanceHead *)module_inst;
    const ConstStrDescription * module_name = NULL;

    if (module_inst_head && module_inst_head->imp_ref_cnt ) {
        module_inst_head->imp_ref_cnt --;
        if (module_inst_head->module_type == Wasm_Module_Bytecode) {
            module_name = ((WASMModuleInstance*)module_inst)->module->module_name;
        } else {
            module_name = ((AOTModule*)((AOTModuleInstance*)module_inst)->aot_module.ptr)->module_name;
        }
        (void)module_name;
        //printf("module %s, exp ref = %d, imp ref = %d\n",
        //    module_name->str,
        //    module_inst_head->exp_ref_cnt,
        //    module_inst_head->imp_ref_cnt);
        if (!module_inst_head->imp_ref_cnt) {
            if (!(*unused_module_list)) {
                *unused_module_list = key;
            } else {
                (*unused_module_list)->next = key;
                (*unused_module_list) = key;
            }
        }
    }
}

void
wasm_program_close_dependencies(wasm_module_inst_t module_inst,
                                uint32 inst_id)
{
    WASMModuleInstance * caller_module_inst = (WASMModuleInstance*)module_inst;
    WASMProgramInstance * program = caller_module_inst->program;
    WASMModuleInstanceCommon * callee_module_inst = NULL, * iter_module_inst = NULL;
    WASMModuleInstanceHead * callee_module_inst_head = NULL, *iter_module_inst_head = NULL;
    HashMap * implicit_dep_hmap = NULL;
    const ConstStrDescription * list_end = NULL, * list_head = NULL,
                                * cur_processing_node = NULL;
    ConstStrDescription * key = NULL;
    const ConstStrDescription *module_name = NULL;
    void * p = NULL;

    if (!program)
        return;

    callee_module_inst = wasm_program_destroy_dlopen_session(program, inst_id);
    callee_module_inst_head = (WASMModuleInstanceHead *)callee_module_inst;

    if (!callee_module_inst)
        return;

    // a FIFO list stores the modules on which no modules depends implicitly.
    // implement a breath first ref count update.
    if (!callee_module_inst_head->exp_ref_cnt &&
        !callee_module_inst_head->imp_ref_cnt) {
            implicit_dep_hmap = callee_module_inst_head->local_implicit_dependency_modules_name_hmap;
            if (callee_module_inst_head->module_type == Wasm_Module_Bytecode) {
                module_name = ((WASMModuleInstance*)callee_module_inst)->module->module_name;
            } else {
                module_name = ((AOTModule*)((AOTModuleInstance*)callee_module_inst)->aot_module.ptr)->module_name;
            }
            //printf("************** possible to clear deps of %s ***********************\n",
            //    module_name->str);
            list_head = list_end = module_name;
            if (implicit_dep_hmap) {
                //list_head = list_end = module_name;

                // handle current module's dependency
                bh_hash_map_traverse(implicit_dep_hmap,
                    (TraverseCallbackFunc)decrement_ref_module_inst_callback, &list_end);
            }

            // we have handle the callee module, so start from its deps in the following loop.
            cur_processing_node = list_head->next;

            // handle dependencies's deps recursively
            while (cur_processing_node) {
                key = (ConstStrDescription*)cur_processing_node;

                iter_module_inst = bh_hash_map_find(program->global_modules_inst_name_hmap, (void*)key);
                iter_module_inst_head = (WASMModuleInstanceHead*)iter_module_inst;
                if (iter_module_inst_head) {
                    bh_assert(iter_module_inst_head->imp_ref_cnt == 0);
                    if (iter_module_inst_head->local_implicit_dependency_modules_name_hmap)
                        bh_hash_map_traverse(iter_module_inst_head->local_implicit_dependency_modules_name_hmap,
                            (TraverseCallbackFunc)decrement_ref_module_inst_callback, &list_end);
                }

                cur_processing_node = cur_processing_node->next;
            }
    }

    // remove the modules which both exp_ref and imp_ref are zero.
    // Note: currently, we don't handle cycle-dependency case, it's responsible for application user.
    cur_processing_node = list_head;
    while(cur_processing_node) {
        key = (ConstStrDescription*)cur_processing_node;

        iter_module_inst = bh_hash_map_find(program->global_modules_inst_name_hmap, (void*)key);
        iter_module_inst_head = (WASMModuleInstanceHead*)iter_module_inst;
        if (iter_module_inst_head && !iter_module_inst_head->exp_ref_cnt) {
            p = (void*)(uintptr_t)iter_module_inst_head->inst_id;
            bh_hash_map_remove(program->global_modules_inst_id_hmap,
                p,
                NULL,
                NULL);
            bh_hash_map_remove(program->global_modules_inst_name_hmap, (void*)key, NULL, NULL);
            if (iter_module_inst_head->module_type == Wasm_Module_Bytecode) {
                module_name = ((WASMModuleInstance*)iter_module_inst)->module->module_name;
            } else {
                module_name = ((AOTModule*)((AOTModuleInstance*)iter_module_inst)->aot_module.ptr)->module_name;
            }
            //printf("module %s deinstantiating\n", module_name->str);

            // wasm_program_free_module_instance_id(program, iter_module_inst->inst_id);
            // wasm_program_remove_module_inst_from_name_hmap(program, iter_module_inst);
            wasm_program_invalidate_cached_wasm_func(program, (wasm_module_inst_t)iter_module_inst);
            wasm_runtime_module_free(program->root_module_inst, iter_module_inst_head->init_globals.actual_memory_base);
            wasm_runtime_deinstantiate((wasm_module_inst_t)iter_module_inst);
        }

        cur_processing_node = cur_processing_node->next;
        // unlink current working list
        key->next = NULL;
    }

}
// Note: it will be implemented as thread-safe.
// only one thread could open a dependency wasm and its dependency per time.
// will add contention protection (lock/mutex etc) later.
uint32
wasm_program_open_dependencies(wasm_module_inst_t module_inst,
                                const char * path)
{
    WASMModuleInstanceHead * module_inst_head = (WASMModuleInstanceHead *)module_inst;
    WASMProgramInstance * program_inst = module_inst_head->program;
    WASMRuntime * runtime = module_inst_head->runtime;
    WASMModuleInstanceCommon * callee_module_inst = NULL;

    if (!program_inst)
        return 0;

    callee_module_inst = wasm_program_open_dependencies_general(runtime,
                                            program_inst,
                                            module_inst,
                                            path);

    if (!callee_module_inst)
        return 0;

    if (callee_module_inst == module_inst)
        return 0;

    return wasm_program_create_dlopen_session(program_inst, callee_module_inst);
}

WASMModuleInstanceCommon *
wasm_program_instantiate_dependencies(WASMRuntime * runtime,
                                WASMProgramInstance * program_inst,
                                WASMModuleInstanceCommon * caller_module_inst,
                                WASMModuleCommon * module)
{
    WASMModuleInstanceCommon * new_module_inst = NULL;
    WASMModuleInstanceCommon *root_module_inst = NULL;
    WASMModule * wasm_module = (WASMModule*)module;
    AOTModule * aot_module = (AOTModule*)module;
    DependencyModuleInitGlobals init_globals;
    int32 init_size = 0, export_func_count = 0, stack_size = 0;
    uint32 offset = 0;
    void * native_addr = NULL;
    bool is_aot = !(module->module_type == Wasm_Module_Bytecode);

    root_module_inst = program_inst->root_module_inst;

    if (!is_aot && !wasm_module->dylink_section) {
        set_error_buf_v(program_inst->error_buf, program_inst->error_buf_size,
                      "%s isn't a valid shared wasm module.\n", wasm_module->module_name->str);
        return NULL;
    } else if (is_aot && !aot_module->dylink_section) {
        set_error_buf_v(program_inst->error_buf, program_inst->error_buf_size,
                      "%s isn't a valid shared wasm module.\n", aot_module->module_name->str);
        return NULL;
    }

    if (root_module_inst->module_type == Wasm_Module_Bytecode)
        stack_size = ((WASMModuleInstance*)root_module_inst)->default_wasm_stack_size;
    else
        stack_size = ((AOTModuleInstance*)root_module_inst)->default_wasm_stack_size;

    memset(&init_globals, 0, sizeof(DependencyModuleInitGlobals));

    // lazy instantiation
    // the idea is to load all dependencies first (so we can check if all of them exist),
    // but only instantiate the first one, and other modules will be instantiated as needed.

    export_func_count = get_export_count_by_kind(module, EXPORT_KIND_FUNC);

    // allocate init mem space for dependency module
    if (!is_aot) {
        if (!wasm_module->dylink_section->table_alignment)
            wasm_module->dylink_section->table_alignment = 1;

        init_size = wasm_module->dylink_section->table_size + export_func_count;
        init_size += wasm_module->dylink_section->table_alignment - 1;

        // will calcuate it by instance id
        init_globals.table_alignment = wasm_module->dylink_section->table_alignment;

        if (!wasm_module->dylink_section->memory_alignment)
            wasm_module->dylink_section->memory_alignment = 1;

        bh_assert(wasm_module->dylink_section->memory_alignment > 0);
    } else {
        if (!aot_module->dylink_section->table_alignment)
            aot_module->dylink_section->table_alignment = 1;

        init_size = aot_module->dylink_section->table_size + export_func_count;
        init_size += aot_module->dylink_section->table_alignment - 1;

        // will calcuate it by instance id
        init_globals.table_alignment = aot_module->dylink_section->table_alignment;

        if (!aot_module->dylink_section->memory_alignment)
            aot_module->dylink_section->memory_alignment = 1;

        bh_assert(aot_module->dylink_section->memory_alignment > 0);
    }

    bh_assert(init_size <= TABLE_SPACE_SLOT_SIZE);
    init_globals.table_size = init_size;
    init_globals.table_base = 0;

    // assumpt stack_pointer is the first global.
    // if root module is opened by dlopen, global[0] should be __stack_pointer
    // else if root module is a library module, its global[0] is also __stack_pointer
    if (caller_module_inst->module_type == Wasm_Module_Bytecode) {
        //bh_assert(((WASMModuleInstance*)caller_module_inst)->globals[0].is_mutable == true &&
        //        ((WASMModuleInstance*)caller_module_inst)->globals[0].type == VALUE_TYPE_I32);
    } else {
        AOTModule * root_aot_module = (AOTModule*)((AOTModuleInstance*)caller_module_inst)->aot_module.ptr;
        (void)root_aot_module;
        if (root_aot_module->dylink_section)
            bh_assert(root_aot_module->import_globals[0].is_mutable == true &&
            root_aot_module->import_globals[0].type == VALUE_TYPE_I32);
        //else
        //    bh_assert(root_aot_module->globals[0].is_mutable == true &&
        //        root_aot_module->globals[0].type == VALUE_TYPE_I32);
    }

    if (!is_aot)  {
        init_size = wasm_module->dylink_section->memory_size + wasm_module->dylink_section->memory_alignment - 1;

        offset = wasm_runtime_module_malloc((wasm_module_inst_t)root_module_inst,
                                    init_size, &native_addr);
        if (!offset)
            return NULL;

        init_globals.actual_memory_base = offset;

        init_globals.memory_base = (offset + wasm_module->dylink_section->memory_alignment - 1) &
                             (~(wasm_module->dylink_section->memory_alignment - 1));

        new_module_inst = (WASMModuleInstanceCommon*)wasm_instantiate_dependency((WASMModule*)module, program_inst,
                            stack_size, &init_globals);
    } else {
        init_size = aot_module->dylink_section->memory_size + aot_module->dylink_section->memory_alignment - 1;
        offset = wasm_runtime_module_malloc((wasm_module_inst_t)root_module_inst,
                                    init_size, &native_addr);
        if (!offset)
            return NULL;

        init_globals.actual_memory_base = offset;

        init_globals.memory_base = (offset + aot_module->dylink_section->memory_alignment - 1) &
                             (~(aot_module->dylink_section->memory_alignment - 1));

        new_module_inst = (WASMModuleInstanceCommon*)aot_instantiate_dependency((AOTModule*)module, program_inst,
                            stack_size, &init_globals);
    }

    if (!new_module_inst)
        return NULL;

    return new_module_inst;
}

WASMModuleInstanceCommon *
wasm_program_open_dependencies_general(WASMRuntime * runtime,
                                WASMProgramInstance * program_inst,
                                WASMModuleInstanceCommon * caller_module_inst,
                                const char * path)
{
    const ConstStrDescription * key = NULL;
    WASMModuleCommon * new_module = NULL;
    WASMModuleInstanceCommon *root_module_inst = NULL, *new_module_inst = NULL;
    // bh_list loading_modules_list;
    //LoadingModuleElem * elem = NULL;

    root_module_inst = program_inst->root_module_inst;

    // records string into const string pool
    key = wasm_runtime_records_const_string(
                        runtime, path,
                        strlen(path),
                        program_inst->error_buf,
                        program_inst->error_buf_size);
    if (!key) {
        return NULL;
    }

    // check if opened by the current program.
    new_module_inst = dylib_entries_map_find(key, program_inst->global_modules_inst_name_hmap);
    if (new_module_inst) {
        if (new_module_inst->module_type != caller_module_inst->module_type)
            return NULL;

        if (!wasm_runtime_is_shared_module_instance(new_module_inst)) {
            return NULL;
        }

        return new_module_inst;
    }

    // start to load all dependency modules.
    new_module = load_explicit_dependency_module(root_module_inst, key);

    if (!new_module)
        return NULL;

    // currently only supports wasm->wasm, aot->aot
    if (new_module->module_type != caller_module_inst->module_type)
        return NULL;

    if (!wasm_runtime_is_shared_module(new_module)) {
        return NULL;
    }

    // can exit the program lock scope from here.

    new_module_inst = (WASMModuleInstanceCommon*)wasm_program_instantiate_dependencies(
                runtime, program_inst, (WASMModuleInstanceCommon*)caller_module_inst, new_module);
    if (!new_module_inst)
        return NULL;

    if (!dylib_entries_map_insert(key, new_module_inst,
                    program_inst->global_modules_inst_name_hmap))
        return NULL;

    return new_module_inst;
}

uint32
wasm_program_lookup_symbol_from_module(wasm_module_inst_t caller_module,
                                        uint32 inst_id, const char * symbol)
{
    WASMModuleInstanceHead * caller_module_inst_head = (WASMModuleInstanceHead *)caller_module;
    WASMModuleInstanceCommon * callee_module_inst = NULL;
    WASMModuleInstance * wasm_inst = NULL;
    AOTModuleInstance * aot_inst = NULL;
    AOTModule * aot_module = NULL;
    uint32 table_slot = 0, i = 0;
    void * key = NULL;

    if (!caller_module_inst_head->program)
        return 0;

    key = (void*)(uintptr_t)inst_id;

    callee_module_inst = bh_hash_map_find(caller_module_inst_head->program->global_modules_inst_id_hmap, key);
    if (!callee_module_inst)
        return 0;

    if (callee_module_inst->module_type == Wasm_Module_Bytecode) {
        wasm_inst = (WASMModuleInstance *)callee_module_inst;

        for (i = 0; i < wasm_inst->export_func_count; i++) {
            if (!strcmp(wasm_inst->export_functions[i].name, symbol)) {
                table_slot = i;
                break;
            }
        }

        if (i == wasm_inst->export_func_count)
            return 0;

        table_slot += (wasm_inst->inst_id *
                    TABLE_SPACE_SLOT_SIZE) - wasm_inst->export_func_count;
    } else {
        aot_inst = (AOTModuleInstance *)callee_module_inst;
        aot_module = (AOTModule*)aot_inst->aot_module.ptr;

        for (i = 0; i < aot_module->export_func_count; i++) {
            if (!strcmp(((AOTExportFunctionInstance*)aot_inst->export_funcs.ptr)[i].func_name, symbol)) {
                table_slot = i;
                break;
            }
        }

        if (i == aot_module->export_func_count)
            return 0;

        table_slot += (aot_inst->inst_id *
                    TABLE_SPACE_SLOT_SIZE) - aot_module->export_func_count;
    }

    return table_slot;
}

bool
wasm_program_resolve_aot_function(WASMProgramInstance * program,
                            AOTModuleInstance * resolve_module_inst,
                            AOTModuleInstance ** p_callee_module_inst,
                            uint32 import_func_id)
{
    WASMRuntime * runtime = resolve_module_inst->runtime;
    const ConstStrDescription * module_name = NULL, * func_name = NULL;
    AOTModule * module = resolve_module_inst->aot_module.ptr;
    AOTModule * callee_module = NULL;
    AOTImportFunc * import_func = &module->import_funcs[import_func_id];
    AOTFuncType * cur_type = NULL, *cur_func_type = NULL;
    AOTModuleInstance *caller_module_inst = resolve_module_inst, *callee_module_inst = NULL;
    AOTExportFunctionInstance * export_funcs = NULL, *callee_function_inst = NULL;
    uint32 i = 0;
    bool resolve_memop_func = false;

    cur_type = import_func->func_type;

    while(1) {
        resolve_memop_func = false;
        module_name = import_func->module_name;
        func_name = import_func->func_name;

        if (module_name == CONST_STR_POOL_DESC(runtime, WAMR_CSP_env) &&
            (program->config.import_memop_mode != FROM_ROOT ||
            !wasm_runtime_is_memop_symbol(runtime, func_name))) {
            *p_callee_module_inst = caller_module_inst;
            return true;
        }

        if (module_name == CONST_STR_POOL_DESC(runtime, WAMR_CSP_env)) {
            resolve_memop_func = true;
            bh_assert(caller_module_inst != (AOTModuleInstance*)program->root_module_inst);
            callee_module_inst = (AOTModuleInstance*)program->root_module_inst;
        }

        if (!callee_module_inst) {
            module_name = upgrade_module_extension(runtime,
                                module_name, Wasm_Module_AoT, program->error_buf, program->error_buf_size);

            callee_module_inst = (AOTModuleInstance*)wasm_program_get_module_inst_by_name(program, module_name);
        }

        if (!callee_module_inst) {
            callee_module = (AOTModule*)wasm_runtime_get_module_by_name(runtime, module_name);

            if (!callee_module) {
                aot_set_exception_with_id(caller_module_inst, EXEC_CALL_UNLINKED_MODULE);
                return false;
            }

            callee_module_inst = (AOTModuleInstance*)wasm_program_instantiate_dependencies(runtime,
                                    program, (WASMModuleInstanceCommon*)caller_module_inst, (WASMModuleCommon*)callee_module);

            if (!callee_module_inst) {
                aot_set_exception_with_id(caller_module_inst, EXEC_CALL_UNLINKED_MODULE);
                return false;
            }

            if (!bh_hash_map_insert_with_dup(program->global_modules_inst_name_hmap,
                                        (void*)module_name, (void*)callee_module_inst)) {
                aot_set_exception_with_id(caller_module_inst, EXEC_CALL_UNLINKED_MODULE);
                return false;
            }
        }

        if (!resolve_memop_func) {
            if (!caller_module_inst->local_implicit_dependency_modules_name_hmap) {
                caller_module_inst->local_implicit_dependency_modules_name_hmap =
                    bh_hash_map_create(8, false,
                                        (HashFunc)const_str_hash,
                                        (KeyEqualFunc)const_str_equal,
                                        NULL,
                                        NULL);
                if (!caller_module_inst->local_implicit_dependency_modules_name_hmap) {
                    aot_set_exception_with_id(caller_module_inst, EXEC_CALL_UNLINKED_MODULE);
                    return false;
                }

                if (!bh_hash_map_insert(caller_module_inst->local_implicit_dependency_modules_name_hmap,
                    (void*)module_name, (void*)callee_module_inst)) {
                    aot_set_exception_with_id(caller_module_inst, EXEC_CALL_UNLINKED_MODULE);
                    return false;
                }

                callee_module_inst->imp_ref_cnt ++;
            } else {
                if (!bh_hash_map_find(caller_module_inst->local_implicit_dependency_modules_name_hmap,
                    (void*)module_name)) {
                    if (!bh_hash_map_insert(caller_module_inst->local_implicit_dependency_modules_name_hmap,
                        (void*)module_name, (void*)callee_module_inst)) {
                        aot_set_exception_with_id(caller_module_inst, EXEC_CALL_UNLINKED_MODULE);
                        return false;
                    }

                    callee_module_inst->imp_ref_cnt ++;
                }
            }
        }

        //printf("module %s, exp ref = %d, imp ref = %d\n",
        //        module_name->str,
        //        callee_module_inst->exp_ref_cnt,
        //        callee_module_inst->imp_ref_cnt);

        export_funcs = (AOTExportFunctionInstance*)callee_module_inst->export_funcs.ptr;
        if (!callee_module)
            callee_module = (AOTModule*)callee_module_inst->aot_module.ptr;

        for (i = 0; i < callee_module->export_func_count; i++) {
            if (!strcmp(export_funcs[i].func_name, import_func->func_name->str)) {
                break;
            }
        }

        if (i == callee_module->export_func_count) {
            aot_set_exception_with_id(caller_module_inst, EXCE_CALL_UNLINKED_IMPORT_FUNC);
            return false;
        }

        callee_function_inst = &export_funcs[i];

        if (callee_function_inst->is_import_func)
            cur_func_type = callee_function_inst->u.func_import->func_type;
        else
            cur_func_type = callee_function_inst->u.func.func_type;
        if (!wasm_type_equal(cur_type, cur_func_type)) {
            aot_set_exception_with_id(caller_module_inst, EXCE_INVALID_FUNCTION_TYPE_INDEX);
            return false;
        }

        if (!callee_function_inst->is_import_func)
            break;

        if (callee_function_inst->u.func_import->func_ptr_linked)
            break;

        caller_module_inst = callee_module_inst;
        import_func = callee_function_inst->u.func_import;
    }

    ((void**)resolve_module_inst->func_ptrs.ptr)[import_func_id] = callee_function_inst->u.func.func_ptr;
    *p_callee_module_inst = callee_module_inst;

    return true;
}

// TODO: the resolve algorithm need to improved to update the resolving result for other intermediate modules.
static bool
wasm_program_resolve_wasm_function(WASMProgramInstance * program,
                            WASMModuleInstance * caller_module_inst,
                            WASMFunctionInstance * import_func)
{
    WASMModule * callee_module = NULL;
    const ConstStrDescription * module_name = NULL, *func_name = NULL;
    WASMModuleInstance * callee_module_inst = NULL;
    WASMFunctionInstance * callee_function_inst = NULL;
    WASMRuntime * runtime = caller_module_inst->runtime;
    WASMType *cur_type = NULL, *cur_func_type = NULL;
    bool resolve_memop_func = false;

    uint32 i = 0;
    if (!import_func->is_import_func)
        return true;

    callee_function_inst = import_func;
    cur_type = callee_function_inst->u.func_import->func_type;

    while (1) {
        resolve_memop_func = false;
        module_name = callee_function_inst->u.func_import->module_name;
        func_name = callee_function_inst->u.func_import->field_name;
        if (module_name == CONST_STR_POOL_DESC(runtime, WAMR_CSP_env) &&
            (program->config.import_memop_mode != FROM_ROOT ||
            !wasm_runtime_is_memop_symbol(runtime, func_name))) {
            break;
        }

        if (module_name == CONST_STR_POOL_DESC(runtime, WAMR_CSP_env)) {
            resolve_memop_func = true;
            bh_assert(caller_module_inst != (WASMModuleInstance*)program->root_module_inst);
            callee_module_inst = (WASMModuleInstance*)program->root_module_inst;
        }

        if (!callee_module_inst)
            callee_module_inst = (WASMModuleInstance*)wasm_program_get_module_inst_by_name(program, module_name);

        if (!callee_module_inst) {
            callee_module = (WASMModule*)wasm_runtime_get_module_by_name(runtime, module_name);
            if (!callee_module)
                return false;

            callee_module_inst = (WASMModuleInstance*)wasm_program_instantiate_dependencies(runtime,
                                            program, (WASMModuleInstanceCommon*)caller_module_inst, (WASMModuleCommon*)callee_module);

            if (!callee_module_inst)
                return false;

            if (!bh_hash_map_insert_with_dup(program->global_modules_inst_name_hmap,
                                        (void*)module_name, (void*)callee_module_inst))
                return false;
        }

        if (!resolve_memop_func) {
            if (!caller_module_inst->local_implicit_dependency_modules_name_hmap) {
                caller_module_inst->local_implicit_dependency_modules_name_hmap =
                    bh_hash_map_create(8, false,
                                        (HashFunc)const_str_hash,
                                        (KeyEqualFunc)const_str_equal,
                                        NULL,
                                        NULL);
                if (!caller_module_inst->local_implicit_dependency_modules_name_hmap)
                    return false;

                if (!bh_hash_map_insert(caller_module_inst->local_implicit_dependency_modules_name_hmap,
                    (void*)module_name, (void*)callee_module_inst))
                    return false;

                callee_module_inst->imp_ref_cnt ++;
            } else {
                if (!bh_hash_map_find(caller_module_inst->local_implicit_dependency_modules_name_hmap,
                    (void*)module_name)) {
                    if (!bh_hash_map_insert(caller_module_inst->local_implicit_dependency_modules_name_hmap,
                        (void*)module_name, (void*)callee_module_inst))
                        return false;

                    callee_module_inst->imp_ref_cnt ++;
                }
            }
        }

        //printf("module %s, exp ref = %d, imp ref = %d\n",
        //    callee_module_inst->module->module_name->str,
        //    callee_module_inst->exp_ref_cnt,
        //    callee_module_inst->imp_ref_cnt);
#if 0
        if (program->config.root_is_AS_module) {
            if (func_name == CONST_STR_POOL_DESC(runtime, WAMR_CSP_malloc))
                func_name = CONST_STR_POOL_DESC(runtime, WAMR_CSP___alloc);
            else if (func_name == CONST_STR_POOL_DESC(runtime, WAMR_CSP_free))
                func_name = CONST_STR_POOL_DESC(runtime, WAMR_CSP___free);
            else if (func_name == CONST_STR_POOL_DESC(runtime, WAMR_CSP_realloc))
                func_name = CONST_STR_POOL_DESC(runtime, WAMR_CSP___realloc);
        }
#endif
        for (i = 0; i < callee_module_inst->export_func_count; i++) {
            if (!strcmp(callee_module_inst->export_functions[i].name, func_name->str)) {
                break;
            }
        }

        if (i == callee_module_inst->export_func_count) {
            return false;
        }

        callee_function_inst = callee_module_inst->export_functions[i].function;

        if (callee_function_inst->is_import_func)
            cur_func_type = callee_function_inst->u.func_import->func_type;
        else
            cur_func_type = callee_function_inst->u.func->func_type;
        if (!wasm_type_equal(cur_type, cur_func_type)) {
            return false;
        }

        if (!callee_function_inst->is_import_func)
            break;

        if (callee_function_inst->u.func_import->func_ptr_linked)
            break;

        caller_module_inst = callee_module_inst;
    }

    import_func->import_module_inst = callee_module_inst;
    import_func->import_func_inst = callee_function_inst;

    return true;
}

bool
wasm_program_resolve_op_call(WASMProgramInstance * program,
                            WASMModuleInstance * caller_module_inst,
                            WASMModuleInstance ** p_callee_module_inst,
                            WASMFunctionInstance ** p_call_func)
{
    WASMModuleInstance * callee_module_inst = NULL;
    WASMFunctionInstance * call_func = *p_call_func;

    // lazy link, resolve the import function.
    if (program &&
        //caller_module_inst != (WASMModuleInstance*)program->root_module_inst &&
        call_func->is_import_func &&
        !call_func->u.func_import->func_ptr_linked) {

        if (!call_func->import_module_inst &&
            !call_func->import_func_inst) {
                if (program->config.binding_mode != LAZY_BINDING) {
                    wasm_set_exception(caller_module_inst, "uninitialized import function.");
                    return false;
                }

            if (!wasm_program_resolve_wasm_function(program, caller_module_inst, call_func)) {
                char buf[128];
                snprintf(buf, sizeof(buf),
                        "failed to call unlinked import function (%s, %s)",
                        (char*)call_func->u.func_import->module_name->str,
                        (char*)call_func->u.func_import->field_name->str);
                wasm_set_exception(caller_module_inst, buf);
                return false;
            }
        }

        callee_module_inst = call_func->import_module_inst;
        call_func = call_func->import_func_inst;

        *p_callee_module_inst = callee_module_inst;
        *p_call_func = call_func;
    }

    return  true;
}

uint32
wasm_program_alloc_table_space_by_size(uint32 inst_id,
                            uint32 needed_size)
{
    uint32 table_space_start = 0;

    bh_assert(needed_size <= TABLE_SPACE_SLOT_SIZE);
    table_space_start = ((inst_id - 1)<< TABLE_SPACE_BITS_LEN);

    return table_space_start;
}

uint32
wasm_program_alloc_table_space_by_table(WASMModuleInstance * module_inst,
                        WASMTableInstance * table)
{
    uint32 needed_size = table->max_size;
    uint32 inst_id = module_inst->inst_id;

    return wasm_program_alloc_table_space_by_size(inst_id, needed_size);
}

#if WASM_ENABLE_LIBC_BUILTIN != 0
uint32
get_libc_builtin_export_apis(NativeSymbol **p_libc_builtin_apis);

#define BUILTIN_FUNC_ALLOC_STEP 4
// create an internal libc module, but not create func instance until link GOT.func global
static int
wasm_program_link_internal_builtin_libc_func(WASMProgramInstance * program, const ConstStrDescription * func_name)
{
    WASMRuntime * runtime = program->runtime;
    WASMFunctionInstance ** functions = (WASMFunctionInstance**)program->builtin_libc_funcs, *linked_func = NULL;
    WASMFunctionImport * func_import = NULL;
    NativeSymbol * native_symbols = NULL;
    uint32 i = 0, func_id = 0, func_offset = 0;
    int32 ret_id = -1;
    uint32 libc_funcs_count = get_libc_builtin_export_apis(&native_symbols);

    (void)(libc_funcs_count);

    if (!functions) {
        functions = wasm_runtime_malloc(sizeof(WASMFunctionInstance *) * BUILTIN_FUNC_ALLOC_STEP);
        if (!functions)
            return -1;

        program->builtin_libc_funcs = (void**)functions;
        program->builtin_libc_funcs_size = BUILTIN_FUNC_ALLOC_STEP;
    }

    for (i = 0; i < program->builtin_libc_funcs_count; i ++) {
        if (func_name == functions[i]->u.func_import->field_name)
            break;
    }

    if (i < program->builtin_libc_funcs_count) {
        return i;
    }

    func_id = wasm_runtime_get_syssymbol_id(runtime, func_name);
    if (func_id < WAMR_CSP_iprintf ||
            func_id > WAMR_CSP_dlclose)
        return -1;

    func_import = wasm_runtime_malloc(sizeof(WASMFunctionImport));
    if (!func_import)
        return -1;

    linked_func = wasm_runtime_malloc(sizeof(WASMFunctionInstance));
    if (!linked_func) {
        wasm_runtime_free(func_import);
        return -1;
    }

    memset(func_import, 0, sizeof(WASMFunctionImport));
    memset(linked_func, 0, sizeof(WASMFunctionInstance));

    func_offset = func_id - WAMR_CSP_iprintf;

    bh_assert(func_name == native_symbols[func_offset].u.symbol);
    func_import->module_name = CONST_STR_POOL_DESC(runtime, WAMR_CSP_env);
    func_import->field_name = func_name;
    func_import->signature = native_symbols[func_offset].signature;
    func_import->func_type = NULL;
    func_import->attachment = native_symbols[func_offset].attachment;
    func_import->call_conv_raw = false;
    func_import->call_conv_wasm_c_api = false;
    func_import->wasm_c_api_with_env = false;
    func_import->func_ptr_linked = native_symbols[func_offset].func_ptr;

    linked_func->is_import_func = true;
    linked_func->func_type = NULL;
    linked_func->local_count = 0;
    linked_func->local_cell_num = 0;
#if WASM_ENABLE_FAST_INTERP != 0
    linked_func->const_cell_num = 0;
#endif
    linked_func->local_offsets = 0;
    linked_func->local_types = NULL;
    linked_func->u.func_import = func_import;

    if ((program->builtin_libc_funcs_count + 1) > program->builtin_libc_funcs_size) {
        functions = wasm_runtime_realloc(functions,
                            sizeof(WASMFunctionInstance *) * (program->builtin_libc_funcs_size + BUILTIN_FUNC_ALLOC_STEP));
        if (!functions) {
            wasm_runtime_free(func_import);
            wasm_runtime_free(linked_func);
            return -1;
        }

        program->builtin_libc_funcs_size += BUILTIN_FUNC_ALLOC_STEP;
        program->builtin_libc_funcs = (void**)functions;
    }

    functions[program->builtin_libc_funcs_count] = linked_func;

    ret_id = program->builtin_libc_funcs_count;
    program->builtin_libc_funcs_count ++;
    return ret_id;
}

#if WASM_ENABLE_AOT != 0
static int
wasm_program_link_aot_internal_builtin_libc_func(WASMProgramInstance * program, const ConstStrDescription * func_name)
{
    WASMRuntime * runtime = program->runtime;
    AOTExportFunctionInstance ** functions = (AOTExportFunctionInstance **)program->builtin_libc_funcs, *linked_func = NULL;
    AOTImportFunc * func_import = NULL;
    NativeSymbol * native_symbols = NULL;
    uint32 i = 0, func_id = 0, func_offset = 0;
    int32 ret_id = -1;
    uint32 libc_funcs_count = get_libc_builtin_export_apis(&native_symbols);

    (void)(libc_funcs_count);

    if (!functions) {
        functions = wasm_runtime_malloc(sizeof(AOTExportFunctionInstance *) * BUILTIN_FUNC_ALLOC_STEP);
        if (!functions)
            return -1;

        program->builtin_libc_funcs = (void**)functions;
        program->builtin_libc_funcs_size = BUILTIN_FUNC_ALLOC_STEP;
    }

    for (i = 0; i < program->builtin_libc_funcs_count; i ++) {
        if (func_name == functions[i]->u.func_import->func_name)
            break;
    }

    if (i < program->builtin_libc_funcs_count) {
        return i;
    }

    func_id = wasm_runtime_get_syssymbol_id(runtime, func_name);
    if (func_id < WAMR_CSP_iprintf ||
            func_id > WAMR_CSP_dlclose)
        return -1;

    func_import = wasm_runtime_malloc(sizeof(AOTImportFunc));
    if (!func_import)
        return -1;

    linked_func = wasm_runtime_malloc(sizeof(AOTExportFunctionInstance));
    if (!linked_func) {
        wasm_runtime_free(func_import);
        return -1;
    }

    memset(func_import, 0, sizeof(AOTImportFunc));
    memset(linked_func, 0, sizeof(AOTExportFunctionInstance));

    func_offset = func_id - WAMR_CSP_iprintf;

    bh_assert(func_name == native_symbols[func_offset].u.symbol);
    func_import->module_name = CONST_STR_POOL_DESC(runtime, WAMR_CSP_env);
    func_import->func_name = func_name;
    func_import->signature = native_symbols[func_offset].signature;
    func_import->func_type = NULL;
    func_import->func_type_index = 0;
    func_import->attachment = native_symbols[func_offset].attachment;
    func_import->call_conv_raw = false;
    func_import->call_conv_wasm_c_api = false;
    func_import->wasm_c_api_with_env = false;
    func_import->func_ptr_linked = native_symbols[func_offset].func_ptr;

    linked_func->is_import_func = true;
    linked_func->u.func_import = func_import;
    linked_func->func_index = 0;

    if ((program->builtin_libc_funcs_count + 1) > program->builtin_libc_funcs_size) {
        functions = wasm_runtime_realloc(functions,
                            sizeof(WASMFunctionInstance *) * (program->builtin_libc_funcs_size + BUILTIN_FUNC_ALLOC_STEP));
        if (!functions) {
            wasm_runtime_free(func_import);
            wasm_runtime_free(linked_func);
            return -1;
        }

        program->builtin_libc_funcs_size += BUILTIN_FUNC_ALLOC_STEP;
        program->builtin_libc_funcs = (void**)functions;
    }

    functions[program->builtin_libc_funcs_count] = linked_func;

    ret_id = program->builtin_libc_funcs_count;
    program->builtin_libc_funcs_count ++;
    return ret_id;
}
#endif

#undef BUILTIN_FUNC_ALLOC_STEP

static void
wasm_program_destroy_internal_libc_module(WASMProgramInstance * program)
{
    if (!program->builtin_libc_funcs)
        return;

    for(uint32 i = 0; i < program->builtin_libc_funcs_count; i++) {
        if (!program->builtin_libc_funcs[i])
            continue;

        //WASMFunctionImport * func_import = program->builtin_libc_funcs[i]->u.func_import;
        //if (func_import)
        //    wasm_runtime_free(func_import);

        wasm_runtime_free(program->builtin_libc_funcs[i]);
    }
    wasm_runtime_free(program->builtin_libc_funcs);
}
#endif


bool
wasm_program_link_sp_wasm_globals(WASMProgramInstance * program,
                                WASMGlobalInstance * global,
                                const ConstStrDescription * field_name)
{
    WASMRuntime * runtime = program->runtime;
    WASMModuleInstance * root_module_inst = (WASMModuleInstance *)program->root_module_inst;
    WASMGlobalInstance * export_global = NULL;
    (void)runtime;

    for (uint32 i = 0; i < root_module_inst->export_glob_count; i++) {
        if (!program->config.root_is_AS_module) {
            if (!strcmp(root_module_inst->export_globals[i].name, field_name->str)) {
                export_global = root_module_inst->export_globals[i].global;
            }
        } else {
            if (!strcmp(root_module_inst->export_globals[i].name, CONST_STR_POOL_STR(runtime, WAMR_CSP_var_user_stack_pointer))) {
                export_global = root_module_inst->export_globals[i].global;
            }
        }

        if (!export_global)
            continue;

        if (export_global->type != global->type ||
            export_global->is_mutable != global->is_mutable)
            return false;

        // global->data_offset = root_module_inst->globals->data_offset;
        if (export_global->is_mutable)
            global->data = export_global->data;
        else {
            global->data = (uint8*)&root_module_inst->init_globals.stack_pointer;
        }

        return true;
    }

    return false;
}

#if WASM_ENABLE_AOT != 0
bool
wasm_program_link_sp_aot_globals(WASMProgramInstance * program,
                                uint8 * p_global_data,
                                const ConstStrDescription * global_name)
{
    WASMRuntime * runtime = program->runtime;
    AOTModuleInstance * root_module_inst = (AOTModuleInstance *)program->root_module_inst;
    AOTModule * root_module = (AOTModule *)root_module_inst->aot_module.ptr;
    uint32 global_id = 0, global_offset = 0;
    uint8 * p_data = NULL;

    if (root_module->export_sp_global_id >= root_module->export_count)
        return false;

    if (!program->config.root_is_AS_module) {
        if (strcmp(root_module->exports[root_module->export_sp_global_id].name, global_name->str)) {
            return false;
        }
    } else {
        if (strcmp(root_module->exports[root_module->export_sp_global_id].name, CONST_STR_POOL_STR(runtime, WAMR_CSP_var_user_stack_pointer))) {
            return false;
        }
    }

    global_id = root_module->exports[root_module->export_sp_global_id].index;

    global_offset = root_module->globals[global_id].data_offset;

    p_data = (uint8*)root_module_inst->global_data.ptr + global_offset;

#if __WORDSIZE == 32
    if (root_module->dylink_section)
        *(uint32*)p_global_data = *(uint32*)p_data;
    else
        *(uint32*)p_global_data = (uintptr_t)p_data;
#else
    if (root_module->dylink_section)
        *(uint64*)p_global_data = *(uint64*)p_data;
    else
        *(uint64*)p_global_data = (uintptr_t)p_data;
#endif

    return true;
}

bool
wasm_program_link_aot_got_globals(WASMProgramInstance * program,
                                AOTModuleInstance * module_inst,
                                uint8 * p_global_data,
                                const ConstStrDescription * field_name)
{
    WASMRuntime * runtime = program->runtime;
    AOTModuleInstance * root_module_inst = NULL;
    AOTModule * root_module = NULL;
    bool is_sys_symbol = wasm_runtime_is_system_symbol(runtime, field_name);
    bool is_memop_symbol = wasm_runtime_is_memop_symbol(runtime, field_name);
    int32 import_func_id = -1;
    uint32 i = 0;

    if (is_sys_symbol &&
        (!is_memop_symbol ||
        program->config.import_memop_mode == FROM_BUILTIN_LIBC)) {
#if WASM_ENABLE_LIBC_BUILTIN != 0
        import_func_id = wasm_program_link_aot_internal_builtin_libc_func(program, field_name);
        if (import_func_id < 0) {
            return false;
        }

        *(uint32*)p_global_data = import_func_id + TABLE_SPACE_FOR_BUILTIN_LIBC;
        return true;
#else
        return false;
#endif
    }

    if (is_memop_symbol) {
        bh_assert(program->config.import_memop_mode == FROM_ROOT);
        root_module_inst = (AOTModuleInstance*)program->root_module_inst;
        root_module = (AOTModule*)root_module_inst->aot_module.ptr;

        AOTExportFunctionInstance * export_funcs = (AOTExportFunctionInstance *)root_module_inst->export_funcs.ptr;

        for (i = 0; i < root_module->export_func_count; i++) {
            if (!strcmp(export_funcs[i].func_name, field_name->str)) {
                import_func_id = (int32)i;
                break;
            }
        }

        if (import_func_id == -1)
            return false;

        import_func_id += TABLE_SPACE_SLOT_SIZE - root_module->export_func_count;
        *(uint32*)p_global_data = import_func_id;
        return true;
    }

    // not sys symbol
    // other module's export
    return false;
}
#endif


bool
wasm_program_link_got_globals(WASMProgramInstance * program,
                                WASMModuleInstance * module_inst,
                                WASMGlobalInstance * global,
                                const ConstStrDescription * field_name)
{
    WASMRuntime * runtime = program->runtime;
//    WASMModule * wasm_module = module_inst->module;
//    uint32 import_func_count = wasm_module->import_function_count;
    int32 import_func_id = -1;
    uint32 i = 0;
    WASMModuleInstance * root_module_inst = NULL;
    bool is_sys_symbol = wasm_runtime_is_system_symbol(runtime, field_name);
    bool is_memop_symbol = wasm_runtime_is_memop_symbol(runtime, field_name);

    if (is_sys_symbol &&
        (!is_memop_symbol ||
        program->config.import_memop_mode == FROM_BUILTIN_LIBC)) {
#if WASM_ENABLE_LIBC_BUILTIN != 0
        import_func_id = wasm_program_link_internal_builtin_libc_func(program, field_name);
        if (import_func_id < 0) {
            return false;
        }

        global->initial_value.u32 = import_func_id + TABLE_SPACE_FOR_BUILTIN_LIBC;
        return true;
#else
        return false;
#endif
    }

    if (is_memop_symbol) {
        bh_assert(program->config.import_memop_mode == FROM_ROOT);
        root_module_inst = (WASMModuleInstance*)program->root_module_inst;
#if 0
        if (program->config.root_is_AS_module) {
            if (field_name == CONST_STR_POOL_DESC(runtime, WAMR_CSP_malloc))
                field_name = CONST_STR_POOL_DESC(runtime, WAMR_CSP___alloc);
            else if (field_name == CONST_STR_POOL_DESC(runtime, WAMR_CSP_free))
                field_name = CONST_STR_POOL_DESC(runtime, WAMR_CSP___free);
            else if (field_name == CONST_STR_POOL_DESC(runtime, WAMR_CSP_realloc))
                field_name = CONST_STR_POOL_DESC(runtime, WAMR_CSP___realloc);
        }
#endif
        for (i = 0; i < root_module_inst->export_func_count; i++) {
            if (!strcmp(root_module_inst->export_functions[i].name, field_name->str)) {
                import_func_id = (int32)i;
                break;
            }
        }

        if (import_func_id == -1)
            return false;

        import_func_id += TABLE_SPACE_SLOT_SIZE - root_module_inst->export_func_count;
        global->initial_value.u32 = import_func_id;
        return true;
    }

    // not sys symbol
    // other module's export
    return false;
}

bool
check_symbol_signature(const WASMType *type, const char *signature);

bool
wasm_program_resolve_op_call_indirect(WASMProgramInstance * program,
                                    WASMModuleInstance * module_inst,
                                    uint32 tbl_idx,
                                    int32 tbl_slot_id,
                                    WASMType * func_type,
                                    WASMModuleInstance ** p_callee_module_inst,
                                    WASMFunctionInstance ** p_call_func)
{
    uint32 inst_idx = 0, fidx = 0, export_func_id = 0;
    WASMModuleInstance * callee_module_inst = NULL;
    WASMFunctionInstance * call_func = NULL;
    WASMTableInstance * tbl_inst = NULL;
    WASMType * cur_func_type = NULL;
    int32 origin_slot_id = 0;
    bool is_local_func = true;

    if (tbl_slot_id < 0) {
        wasm_set_exception(module_inst, "undefined element");
        return false;
    }

    inst_idx = (tbl_slot_id >> TABLE_SPACE_BITS_LEN) + 1;
    origin_slot_id = tbl_slot_id;

    if (program) {
        // set --enable-dlopen
        if (inst_idx == module_inst->inst_id) { // call self-module
            callee_module_inst = module_inst;

            tbl_slot_id -= (inst_idx - 1) * TABLE_SPACE_SLOT_SIZE;
            tbl_inst = wasm_get_table_inst(callee_module_inst, tbl_idx);
        } else if (inst_idx == BUILTIN_LIBC_INST_ID) { // call libc function pointer
            is_local_func = false;
            fidx = tbl_slot_id - (inst_idx - 1) * TABLE_SPACE_SLOT_SIZE;
            if (fidx >= program->builtin_libc_funcs_count) {
                wasm_set_exception(module_inst, "undefined element");
                return false;
            }

            call_func = program->builtin_libc_funcs[fidx];
            if (!call_func) {
                wasm_set_exception(module_inst, "undefined element");
                return false;
            }

            if (call_func->u.func_import->func_type) {
                if (!wasm_type_equal(func_type, call_func->u.func_import->func_type)) {
                    wasm_set_exception(module_inst, "indirect call type mismatch");
                    return false;
                }
            } else {
                if (!check_symbol_signature(func_type, call_func->u.func_import->signature))
                    return false;

                call_func->u.func_import->func_type = func_type;
                call_func->func_type = func_type;
            }

            callee_module_inst = module_inst;
            *p_callee_module_inst = callee_module_inst;
            *p_call_func = call_func;

            wasm_program_cache_resolve_result(program, origin_slot_id, call_func, callee_module_inst);
            return true;
        } else if (inst_idx > 1) { // call dependency module
            // call export function by non root module
            callee_module_inst = (WASMModuleInstance*)wasm_program_get_module_inst_by_id(program, inst_idx);
            if (!callee_module_inst) {
                wasm_set_exception(module_inst, "undefined element");
                return false;
            }
            bh_assert(callee_module_inst->module->dylink_section);

            tbl_slot_id -= (inst_idx - 1) * TABLE_SPACE_SLOT_SIZE;
            if (program->config.use_tbl_as_cache) {
                //if (tbl_slot_id >= (int32)callee_module_inst->module->dylink_section->table_size) {
                tbl_slot_id -= TABLE_SPACE_SLOT_SIZE - callee_module_inst->default_table->max_size;

                if ((uint32)tbl_slot_id < (callee_module_inst->default_table->max_size -
                            callee_module_inst->export_func_count)) {
                    wasm_set_exception(module_inst, "wrong export function id");
                    return false;
                }
                //}

                tbl_inst = wasm_get_table_inst(callee_module_inst, tbl_idx);
            } else {
                is_local_func = false;
                tbl_slot_id = callee_module_inst->export_func_count - (TABLE_SPACE_SLOT_SIZE - tbl_slot_id);
            }
        } else {
            // call root module
            if (!tbl_slot_id) {
                wasm_set_exception(module_inst, "uninitialized element");
                return false;
            }

            callee_module_inst = (WASMModuleInstance*)program->root_module_inst;

            if (callee_module_inst->module->dylink_section &&
                program->config.use_tbl_as_cache) {
                tbl_inst = wasm_get_table_inst(callee_module_inst, tbl_idx);

                bh_assert(program->config.import_memop_mode != FROM_ROOT);
                if (tbl_slot_id > ((int32)callee_module_inst->module->dylink_section->table_size)) {
                    tbl_slot_id -= TABLE_SPACE_SLOT_SIZE - callee_module_inst->default_table->max_size;
                }
            } else if (program->config.import_memop_mode == FROM_ROOT ||
                        !program->config.use_tbl_as_cache) {
                is_local_func = false;
                tbl_slot_id = callee_module_inst->export_func_count - (TABLE_SPACE_SLOT_SIZE - tbl_slot_id);
            } else {
                wasm_set_exception(module_inst, "root module is not shared module");
                return false;
            }
        }
    } else {
        callee_module_inst = module_inst;
        tbl_inst = wasm_get_table_inst(callee_module_inst, tbl_idx);
    }

    if (!tbl_inst) { // func not in table
        if (!callee_module_inst->export_functions[tbl_slot_id].function) {
            wasm_set_exception(module_inst, "uninitialized element");
            return false;
        }

        fidx = (callee_module_inst->export_functions[tbl_slot_id].function -
            callee_module_inst->functions);
    } else {
        if (tbl_slot_id < 0 ||
            (tbl_slot_id >= (int32)tbl_inst->cur_size
            )) {
            wasm_set_exception(module_inst, "undefined element");
            return false;
        }

        fidx = ((uint32*)tbl_inst->base_addr)[tbl_slot_id];
    }

    if (fidx == (uint32)-1) {
        if (!program) {
            wasm_set_exception(module_inst, "uninitialized element");
            return false;
        }

        if (inst_idx < 1) {
            wasm_set_exception(module_inst, "uninitialized element");
            return false;
        }

        if (callee_module_inst->module->dylink_section) {
            if (tbl_slot_id < (int32)(tbl_inst->cur_size - callee_module_inst->export_func_count)) {
            wasm_set_exception(module_inst, "uninitialized element");
            return false;
            }
        } else if (tbl_slot_id < (int32)tbl_inst->cur_size) {
            wasm_set_exception(module_inst, "uninitialized element");
            return false;
        }

        if (program->config.binding_mode != LAZY_BINDING) {
            wasm_set_exception(module_inst, "uninitialized element");
            return false;
        }

        export_func_id = tbl_slot_id - (callee_module_inst->default_table->max_size -
                    callee_module_inst->export_func_count);

        if (!callee_module_inst->export_functions[export_func_id].function) {
            wasm_set_exception(module_inst, "uninitialized element");
            return false;
        }

        // lazy link
        fidx = (callee_module_inst->export_functions[export_func_id].function -
                        callee_module_inst->functions);
        ((uint32*)tbl_inst->base_addr)[tbl_slot_id] = fidx;
    }

    if (fidx >= callee_module_inst->function_count) {
        wasm_set_exception(module_inst, "unknown function");
        return false;
    }

    /* always call module own functions */
    call_func = callee_module_inst->functions + fidx;
    cur_func_type = call_func->func_type;

    if (!wasm_type_equal(func_type, cur_func_type)) {
        wasm_set_exception(module_inst, "indirect call type mismatch");
        return false;
    }

    *p_callee_module_inst = callee_module_inst;
    *p_call_func = call_func;

    if (!is_local_func) {
        wasm_program_cache_resolve_result(program, origin_slot_id, call_func, callee_module_inst);
    }
    return true;
}

#if WASM_ENABLE_AOT != 0
bool
wasm_program_resolve_aot_op_call_indirect(WASMExecEnv *exec_env,
                                    WASMProgramInstance * program,
                                    AOTModuleInstance * module_inst,
                                    uint32 tbl_idx,
                                    int32 table_elem_idx,
                                    uint32 expected_type_idx,
                                    AOTFuncType * expected_func_type,
                                    AOTModuleInstance ** p_callee_module_inst,
                                    AOTModule ** p_callee_module,
                                    uint32 * p_call_func_index,
                                    AOTImportFunc ** p_import_func)
{
    AOTModuleInstance * callee_module_inst = NULL;
    AOTModule* callee_module = NULL;
    AOTModule *aot_module = (AOTModule*)module_inst->aot_module.ptr;
    AOTExportFunctionInstance * export_funcs = NULL, *call_func = NULL;
    AOTFuncType * func_type = NULL;
    AOTTableInstance *tbl_inst = NULL;
    uint32 inst_id = 0, export_func_id, local_table_elem_cnt;
    uint32 func_idx = 0, func_type_idx;
    void **func_ptrs = NULL, *func_ptr = NULL;
    AOTImportFunc *import_func = NULL;
    uint32 *func_type_indexes = NULL;
    char buf[96];
    int32 origin_elem_idx = table_elem_idx;

    if (table_elem_idx < 0) {
        aot_set_exception_with_id(module_inst, EXCE_UNDEFINED_ELEMENT);
        return false;
    }

    inst_id = (table_elem_idx >> TABLE_SPACE_BITS_LEN) + 1;
    if (program) {
        if (inst_id == module_inst->inst_id) {
            callee_module_inst = module_inst;
            callee_module = (AOTModule*)callee_module_inst->aot_module.ptr;

            table_elem_idx -= (inst_id - 1) * TABLE_SPACE_SLOT_SIZE;
            tbl_inst = aot_get_table_inst(callee_module_inst, tbl_idx);
        } else if (inst_id == BUILTIN_LIBC_INST_ID) { // call libc function pointer
            //is_intable_func = false;

            func_idx = table_elem_idx - (inst_id - 1) * TABLE_SPACE_SLOT_SIZE;
            if (func_idx >= program->builtin_libc_funcs_count) {
                aot_set_exception_with_id(module_inst, EXCE_UNDEFINED_ELEMENT);
                return false;
            }

            call_func = program->builtin_libc_funcs[func_idx];
            if (!call_func) {
                aot_set_exception_with_id(module_inst, EXCE_UNDEFINED_ELEMENT);
                return false;
            }

            if (call_func->u.func_import->func_type) {
                if (!wasm_type_equal(expected_func_type, call_func->u.func_import->func_type)) {
                    aot_set_exception(module_inst, "indirect call type mismatch");
                    return false;
                }
            } else {
                if (!check_symbol_signature(expected_func_type, call_func->u.func_import->signature))
                    return false;

                call_func->u.func_import->func_type = expected_func_type;
            }

            callee_module_inst = module_inst;
            *p_callee_module_inst = callee_module_inst;
            *p_call_func_index = 0;
            *p_import_func = call_func->u.func_import;

            wasm_program_cache_resolve_result(program, origin_elem_idx, (*p_import_func)->func_ptr_linked, callee_module_inst);

            return true;
        } else if (inst_id > 1) {
            callee_module_inst = (AOTModuleInstance*)wasm_program_get_module_inst_by_id(program, inst_id);
            if (!callee_module_inst) {
                aot_set_exception_with_id(module_inst, EXCE_UNDEFINED_ELEMENT);
                return false;
            }

            callee_module = (AOTModule*)callee_module_inst->aot_module.ptr;

            table_elem_idx -= (inst_id - 1) * TABLE_SPACE_SLOT_SIZE;

            if (program->config.use_tbl_as_cache) {
                tbl_inst = aot_get_table_inst(callee_module_inst, tbl_idx);
                bh_assert(tbl_inst);

                local_table_elem_cnt = (uint32)callee_module->dylink_section->table_size;
                if (wasm_program_is_root_module((WASMModuleInstanceCommon*)callee_module_inst))
                    local_table_elem_cnt ++;

                if ((uint32)table_elem_idx >= local_table_elem_cnt) {
                    table_elem_idx -= TABLE_SPACE_SLOT_SIZE - tbl_inst->max_size;

                    if ((uint32)table_elem_idx < (tbl_inst->max_size -
                            callee_module->export_func_count)) {
                        aot_set_exception_with_id(module_inst, EXCE_UNDEFINED_ELEMENT);
                        return false;
                    }
                }
            } else {
                table_elem_idx = callee_module->export_func_count - (TABLE_SPACE_SLOT_SIZE - table_elem_idx);
            }
        } else {
            if (!table_elem_idx) {
                aot_set_exception_with_id(module_inst, EXCE_UNINITIALIZED_ELEMENT);
                return false;
            }

            callee_module_inst = (AOTModuleInstance*)program->root_module_inst;
            callee_module = (AOTModule*)callee_module_inst->aot_module.ptr;
            if (callee_module->dylink_section &&
                program->config.use_tbl_as_cache) {
                tbl_inst = aot_get_table_inst(callee_module_inst, tbl_idx);
                if ((uint32)table_elem_idx > callee_module->dylink_section->table_size) {
                    table_elem_idx -= TABLE_SPACE_SLOT_SIZE - tbl_inst->max_size;
                }
            } else if (program->config.import_memop_mode == FROM_ROOT ||
                        !program->config.use_tbl_as_cache) {
                table_elem_idx = callee_module->export_func_count - (TABLE_SPACE_SLOT_SIZE - table_elem_idx);
            } else {
                aot_set_exception(module_inst, "root module is not shared module");
                return false;
            }
        }
    } else {
        callee_module_inst = module_inst;
        callee_module = aot_module;

        /* this function is called from native code, so exec_env->handle and
        exec_env->native_stack_boundary must have been set, we don't set
        it again */

        if ((uint8*)&callee_module_inst < exec_env->native_stack_boundary) {
            aot_set_exception_with_id(module_inst, EXCE_NATIVE_STACK_OVERFLOW);
            return false;
        }

        tbl_inst = aot_get_table_inst(callee_module_inst, tbl_idx);
        bh_assert(tbl_inst);

        if ((uint32)table_elem_idx >= tbl_inst->cur_size) {
            aot_set_exception_with_id(callee_module_inst, EXCE_UNDEFINED_ELEMENT);
            return false;
        }
    }

    if (!tbl_inst) {
        export_funcs = (AOTExportFunctionInstance*)callee_module_inst->export_funcs.ptr;

        func_idx = export_funcs[table_elem_idx].func_index;
    } else {
        if (table_elem_idx < 0 ||
            table_elem_idx >= (int32)tbl_inst->cur_size) {
            aot_set_exception_with_id(callee_module_inst, EXCE_UNDEFINED_ELEMENT);
            return false;
        }

        bh_assert( ((uint32*)tbl_inst->data + table_elem_idx) < (uint32*)callee_module_inst->global_table_data_end.ptr);
        func_idx = ((uint32*)tbl_inst->data)[table_elem_idx];
    }

    if (func_idx == (uint32)-1) {
        if (!program) {
            aot_set_exception_with_id(module_inst, EXCE_UNINITIALIZED_ELEMENT);
            return false;
        }

        if (inst_id < 1) {
            aot_set_exception_with_id(module_inst, EXCE_UNINITIALIZED_ELEMENT);
            return false;
        }

        if (callee_module->dylink_section) {
              if ((uint32)table_elem_idx < (tbl_inst->cur_size - callee_module->export_func_count)) {
                aot_set_exception_with_id(module_inst, EXCE_UNINITIALIZED_ELEMENT);
                return false;
              }
        } else if ((uint32)table_elem_idx < tbl_inst->cur_size) {
            aot_set_exception_with_id(module_inst, EXCE_UNINITIALIZED_ELEMENT);
            return false;
        }

        if (program->config.binding_mode != LAZY_BINDING) {
            aot_set_exception_with_id(module_inst, EXCE_UNINITIALIZED_ELEMENT);
            return false;
        }

        export_func_id = table_elem_idx - (tbl_inst->max_size -
                        callee_module->export_func_count);

        export_funcs = (AOTExportFunctionInstance*)callee_module_inst->export_funcs.ptr;

        if (export_func_id >= callee_module->export_func_count) {
            aot_set_exception_with_id(module_inst, EXCE_UNDEFINED_ELEMENT);
            return false;
        }

        // lazy link
        func_idx = export_funcs[export_func_id].func_index;
        ((uint32*)tbl_inst->data)[table_elem_idx] = func_idx;
    }

    // aot_module = (AOTModule*)callee_module_inst->aot_module.ptr;
    func_ptrs = (void**)callee_module_inst->func_ptrs.ptr;
    func_type_indexes = (uint32*)callee_module_inst->func_type_indexes.ptr;

    func_type_idx = func_type_indexes[func_idx];
    func_type = callee_module->func_types[func_type_idx];

    if (!(func_ptr = func_ptrs[func_idx])) {
        bh_assert(func_idx < callee_module->import_func_count);
        import_func = callee_module->import_funcs + func_idx;
        snprintf(buf, sizeof(buf),
                 "failed to call unlinked import function (%s, %s)",
                 import_func->module_name->str, import_func->func_name->str);
        aot_set_exception(module_inst, buf);
        return false;
    }

    if (module_inst->inst_id == callee_module_inst->inst_id && expected_type_idx != func_type_idx) {
        aot_set_exception_with_id(module_inst, EXCE_INVALID_FUNCTION_TYPE_INDEX);
        return false;
    } else if (module_inst->inst_id != callee_module_inst->inst_id) {
        if (!wasm_type_equal(expected_func_type, func_type)) {
            aot_set_exception_with_id(module_inst, EXCE_INVALID_FUNCTION_TYPE_INDEX);
            return false;
        }
    }

    *p_callee_module_inst = callee_module_inst;
    *p_callee_module = callee_module;
    *p_call_func_index = func_idx;
    *p_import_func = NULL;

    wasm_program_cache_resolve_result(program, origin_elem_idx,
        ((void**)(*p_callee_module_inst)->func_ptrs.ptr)[func_idx],
        callee_module_inst);

    return true;
}

#if WASM_ENABLE_LIBC_BUILTIN != 0
uint32
wasm_program_get_ctype_tolower_mem(WASMModuleInstanceCommon * module_inst)
{
    WASMProgramInstance * program = ((WASMModuleInstanceHead*)module_inst)->program;

    if (!program)
        return 0;

    return program->ctype_tolower_loc_space;
}

void
wasm_program_set_ctype_tolower_mem(WASMModuleInstanceCommon * module_inst, uint32 addr)
{
    WASMProgramInstance * program = ((WASMModuleInstanceHead*)module_inst)->program;

    if (!program)
        return;

    program->ctype_tolower_loc_space = addr;
}
#endif

#endif

#endif
