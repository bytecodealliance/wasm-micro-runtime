/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"
#include "bh_common.h"
#include "bh_assert.h"
#include "bh_log.h"
#include "wasm_runtime_common.h"
#include "wasm_memory.h"
#if WASM_ENABLE_INTERP != 0
#include "../interpreter/wasm_runtime.h"
#endif
#if WASM_ENABLE_AOT != 0
#include "../aot/aot_runtime.h"
#endif
#if WASM_ENABLE_THREAD_MGR != 0
#include "../libraries/thread-mgr/thread_manager.h"
#endif
#if WASM_ENABLE_SHARED_MEMORY != 0
#include "wasm_shared_memory.h"
#endif

#if WASM_ENABLE_MULTI_MODULE != 0
/*
 * a safety insurance to prevent
 * circular depencies leading a stack overflow
 * try break early
 */
typedef struct LoadingModule {
    bh_list_link l;
    /* point to a string pool */
    const char *module_name;
} LoadingModule;

static bh_list loading_module_list_head;
static bh_list *const loading_module_list = &loading_module_list_head;
static korp_mutex loading_module_list_lock;

/*
 * a list about all exported functions, globals, memories, tables of every
 * fully loaded module
 */
static bh_list registered_module_list_head;
static bh_list *const registered_module_list = &registered_module_list_head;
static korp_mutex registered_module_list_lock;
static void
wasm_runtime_destroy_registered_module_list();
#endif /* WASM_ENABLE_MULTI_MODULE */

static void
set_error_buf(char *error_buf, uint32 error_buf_size, const char *string)
{
    if (error_buf != NULL)
        snprintf(error_buf, error_buf_size, "%s", string);
}

static void *
runtime_malloc(uint64 size, WASMModuleInstanceCommon *module_inst,
               char *error_buf, uint32 error_buf_size)
{
    void *mem;

    if (size >= UINT32_MAX
        || !(mem = wasm_runtime_malloc((uint32)size))) {
        if (module_inst != NULL) {
            wasm_runtime_set_exception(module_inst,
                                       "allocate memory failed");
        }
        else if (error_buf != NULL) {
            set_error_buf(error_buf, error_buf_size,
                          "allocate memory failed");
        }
        return NULL;
    }

    memset(mem, 0, (uint32)size);
    return mem;
}

static bool
wasm_runtime_env_init()
{
    if (bh_platform_init() != 0)
        return false;

    if (wasm_native_init() == false) {
        goto fail1;
    }

#if WASM_ENABLE_MULTI_MODULE
    if (BHT_OK != os_mutex_init(&registered_module_list_lock)) {
        goto fail2;
    }

    if (BHT_OK != os_mutex_init(&loading_module_list_lock)) {
        goto fail3;
    }
#endif

#if WASM_ENABLE_SHARED_MEMORY
    if (!wasm_shared_memory_init()) {
        goto fail4;
    }
#endif

#if (WASM_ENABLE_WAMR_COMPILER == 0) && (WASM_ENABLE_THREAD_MGR != 0)
    if (!thread_manager_init()) {
        goto fail5;
    }
#endif

#if WASM_ENABLE_AOT != 0
#ifdef OS_ENABLE_HW_BOUND_CHECK
    if (!aot_signal_init()) {
        goto fail6;
    }
#endif
#endif

    return true;

#if WASM_ENABLE_AOT != 0
#ifdef OS_ENABLE_HW_BOUND_CHECK
fail6:
#endif
#endif
#if (WASM_ENABLE_WAMR_COMPILER == 0) && (WASM_ENABLE_THREAD_MGR != 0)
    thread_manager_destroy();
fail5:
#endif
#if WASM_ENABLE_SHARED_MEMORY
    wasm_shared_memory_destroy();
fail4:
#endif
#if WASM_ENABLE_MULTI_MODULE
    os_mutex_destroy(&loading_module_list_lock);
fail3:
    os_mutex_destroy(&registered_module_list_lock);
fail2:
#endif
    wasm_native_destroy();
fail1:
    bh_platform_destroy();

    return false;
}

static bool
wasm_runtime_exec_env_check(WASMExecEnv *exec_env)
{
    return exec_env
           && exec_env->module_inst
           && exec_env->wasm_stack_size > 0
           && exec_env->wasm_stack.s.top_boundary ==
                exec_env->wasm_stack.s.bottom + exec_env->wasm_stack_size
           && exec_env->wasm_stack.s.top <= exec_env->wasm_stack.s.top_boundary;
}

bool
wasm_runtime_init()
{
    if (!wasm_runtime_memory_init(Alloc_With_System_Allocator, NULL))
        return false;

    if (!wasm_runtime_env_init()) {
        wasm_runtime_memory_destroy();
        return false;
    }

    return true;
}

void
wasm_runtime_destroy()
{
#if WASM_ENABLE_AOT != 0
#ifdef OS_ENABLE_HW_BOUND_CHECK
    aot_signal_destroy();
#endif
#endif

    /* runtime env destroy */
#if WASM_ENABLE_MULTI_MODULE
    wasm_runtime_destroy_loading_module_list();
    os_mutex_destroy(&loading_module_list_lock);

    wasm_runtime_destroy_registered_module_list();
    os_mutex_destroy(&registered_module_list_lock);
#endif

#if WASM_ENABLE_SHARED_MEMORY
    wasm_shared_memory_destroy();
#endif

#if (WASM_ENABLE_WAMR_COMPILER == 0) && (WASM_ENABLE_THREAD_MGR != 0)
    thread_manager_destroy();
#endif

    wasm_native_destroy();
    bh_platform_destroy();

    wasm_runtime_memory_destroy();
}

bool
wasm_runtime_full_init(RuntimeInitArgs *init_args)
{
    if (!wasm_runtime_memory_init(init_args->mem_alloc_type,
                                  &init_args->mem_alloc_option))
        return false;

    if (!wasm_runtime_env_init()) {
        wasm_runtime_memory_destroy();
        return false;
    }

    if (init_args->n_native_symbols > 0
        && !wasm_runtime_register_natives(init_args->native_module_name,
                                          init_args->native_symbols,
                                          init_args->n_native_symbols)) {
        wasm_runtime_destroy();
        return false;
    }

#if WASM_ENABLE_THREAD_MGR != 0
    wasm_cluster_set_max_thread_num(init_args->max_thread_num);
#endif

    return true;
}

PackageType
get_package_type(const uint8 *buf, uint32 size)
{
    if (buf && size >= 4) {
        if (buf[0] == '\0' && buf[1] == 'a' && buf[2] == 's' && buf[3] == 'm')
            return Wasm_Module_Bytecode;
        if (buf[0] == '\0' && buf[1] == 'a' && buf[2] == 'o' && buf[3] == 't')
            return Wasm_Module_AoT;
    }
    return Package_Type_Unknown;
}

#if WASM_ENABLE_MULTI_MODULE != 0
static module_reader reader;
static module_destroyer destroyer;
void
wasm_runtime_set_module_reader(const module_reader reader_cb,
                               const module_destroyer destroyer_cb)
{
    reader = reader_cb;
    destroyer = destroyer_cb;
}

module_reader
wasm_runtime_get_module_reader()
{
    return reader;
}

module_destroyer
wasm_runtime_get_module_destroyer()
{
    return destroyer;
}

static WASMRegisteredModule *
wasm_runtime_find_module_registered_by_reference(WASMModuleCommon *module)
{
    WASMRegisteredModule *reg_module = NULL;

    os_mutex_lock(&registered_module_list_lock);
    reg_module = bh_list_first_elem(registered_module_list);
    while (reg_module && module != reg_module->module) {
        reg_module = bh_list_elem_next(reg_module);
    }
    os_mutex_unlock(&registered_module_list_lock);

    return reg_module;
}

bool
wasm_runtime_register_module_internal(const char *module_name,
                                      WASMModuleCommon *module,
                                      uint8 *orig_file_buf,
                                      uint32 orig_file_buf_size,
                                      char *error_buf,
                                      uint32_t error_buf_size)
{
    WASMRegisteredModule *node = NULL;

    node = wasm_runtime_find_module_registered_by_reference(module);
    if (node) { /* module has been registered */
        if (node->module_name) { /* module has name */
           if (!module_name || strcmp(node->module_name, module_name)) {
               /* module has different name */
               LOG_DEBUG("module(%p) has been registered with name %s",
                         module, node->module_name);
               set_error_buf(error_buf, error_buf_size,
                             "Register module failed: "
                             "failed to rename the module");
               return false;
           }
           else {
               /* module has the same name */
               LOG_DEBUG("module(%p) has been registered with the same name %s",
                         module, node->module_name);
               return true;
           }
        }
        else {
            /* module has empyt name, reset it */
            node->module_name = module_name;
            return true;
        }
    }

    /* module hasn't been registered */
    node = runtime_malloc(sizeof(WASMRegisteredModule), NULL, NULL, 0);
    if (!node) {
        LOG_DEBUG("malloc WASMRegisteredModule failed. SZ=%d",
                  sizeof(WASMRegisteredModule));
        return false;
    }

    /* share the string and the module */
    node->module_name = module_name;
    node->module = module;
    node->orig_file_buf = orig_file_buf;
    node->orig_file_buf_size = orig_file_buf_size;

    os_mutex_lock(&registered_module_list_lock);
    bh_list_status ret = bh_list_insert(registered_module_list, node);
    bh_assert(BH_LIST_SUCCESS == ret);
    (void)ret;
    os_mutex_unlock(&registered_module_list_lock);
    return true;
}

bool
wasm_runtime_register_module(const char *module_name, WASMModuleCommon *module,
                             char *error_buf, uint32_t error_buf_size)
{
    if (!error_buf || !error_buf_size) {
        LOG_ERROR("error buffer is required");
        return false;
    }

    if (!module_name || !module) {
        LOG_DEBUG("module_name and module are required");
        set_error_buf(error_buf, error_buf_size,
                      "Register module failed: "
                      "module_name and module are required");
        return false;
    }

    if (wasm_runtime_is_built_in_module(module_name)) {
        LOG_DEBUG("%s is a built-in module name", module_name);
        set_error_buf(error_buf, error_buf_size,
                      "Register module failed: "
                      "can not register as a built-in module");
        return false;
    }

    return wasm_runtime_register_module_internal(
                            module_name, module, NULL, 0,
                            error_buf, error_buf_size);
}

void
wasm_runtime_unregister_module(const WASMModuleCommon *module)
{
    WASMRegisteredModule *registered_module = NULL;

    os_mutex_lock(&registered_module_list_lock);
    registered_module = bh_list_first_elem(registered_module_list);
    while (registered_module && module != registered_module->module) {
        registered_module = bh_list_elem_next(registered_module);
    }

    /* it does not matter if it is not exist. after all, it is gone */
    if (registered_module) {
        bh_list_remove(registered_module_list, registered_module);
        wasm_runtime_free(registered_module);
    }
    os_mutex_unlock(&registered_module_list_lock);
}

WASMModuleCommon *
wasm_runtime_find_module_registered(const char *module_name)
{
    WASMRegisteredModule *module = NULL, *module_next;

    os_mutex_lock(&registered_module_list_lock);
    module = bh_list_first_elem(registered_module_list);
    while (module) {
        module_next = bh_list_elem_next(module);
        if (module->module_name
            && !strcmp(module_name, module->module_name)) {
            break;
        }
        module = module_next;
    }
    os_mutex_unlock(&registered_module_list_lock);

    return module ? module->module : NULL;
}

bool
wasm_runtime_is_module_registered(const char *module_name)
{
    return NULL != wasm_runtime_find_module_registered(module_name);
}

/*
 * simply destroy all
 */
static void
wasm_runtime_destroy_registered_module_list()
{
    WASMRegisteredModule *reg_module = NULL;

    os_mutex_lock(&registered_module_list_lock);
    reg_module = bh_list_first_elem(registered_module_list);
    while (reg_module) {
        WASMRegisteredModule *next_reg_module = bh_list_elem_next(reg_module);

        bh_list_remove(registered_module_list, reg_module);

        /* now, it is time to release every module in the runtime */
#if WASM_ENABLE_INTERP != 0
        if (reg_module->module->module_type == Wasm_Module_Bytecode)
            wasm_unload((WASMModule *)reg_module->module);
#endif
#if WASM_ENABLE_AOT != 0
        if (reg_module->module->module_type == Wasm_Module_AoT)
            aot_unload((AOTModule *)reg_module->module);
#endif

        /* destroy the file buffer */
        if (destroyer && reg_module->orig_file_buf) {
            destroyer(reg_module->orig_file_buf,
                      reg_module->orig_file_buf_size);
            reg_module->orig_file_buf = NULL;
            reg_module->orig_file_buf_size = 0;
        }

        wasm_runtime_free(reg_module);
        reg_module = next_reg_module;
    }
    os_mutex_unlock(&registered_module_list_lock);
}

bool
wasm_runtime_add_loading_module(const char *module_name,
                                char *error_buf, uint32 error_buf_size)
{
    LOG_DEBUG("add %s into a loading list", module_name);
    LoadingModule *loadingModule =
            runtime_malloc(sizeof(LoadingModule), NULL,
                           error_buf, error_buf_size);

    if (!loadingModule) {
        return false;
    }

    /* share the incoming string */
    loadingModule->module_name = module_name;

    os_mutex_lock(&loading_module_list_lock);
    bh_list_status ret = bh_list_insert(loading_module_list, loadingModule);
    bh_assert(BH_LIST_SUCCESS == ret);
    (void)ret;
    os_mutex_unlock(&loading_module_list_lock);
    return true;
}

void
wasm_runtime_delete_loading_module(const char *module_name)
{
    LOG_DEBUG("delete %s from a loading list", module_name);

    LoadingModule *module = NULL;

    os_mutex_lock(&loading_module_list_lock);
    module = bh_list_first_elem(loading_module_list);
    while (module && strcmp(module->module_name, module_name)) {
        module = bh_list_elem_next(module);
    }

    /* it does not matter if it is not exist. after all, it is gone */
    if (module) {
        bh_list_remove(loading_module_list, module);
        wasm_runtime_free(module);
    }
    os_mutex_unlock(&loading_module_list_lock);
}

bool
wasm_runtime_is_loading_module(const char *module_name)
{
    LOG_DEBUG("find %s in a loading list", module_name);

    LoadingModule *module = NULL;

    os_mutex_lock(&loading_module_list_lock);
    module = bh_list_first_elem(loading_module_list);
    while (module && strcmp(module_name, module->module_name)) {
        module = bh_list_elem_next(module);
    }
    os_mutex_unlock(&loading_module_list_lock);

    return module != NULL;
}

void
wasm_runtime_destroy_loading_module_list()
{
    LoadingModule *module = NULL;

    os_mutex_lock(&loading_module_list_lock);
    module = bh_list_first_elem(loading_module_list);
    while (module) {
        LoadingModule *next_module = bh_list_elem_next(module);

        bh_list_remove(loading_module_list, module);
        /*
         * will not free the module_name since it is
         * shared one of the const string pool
         */
        wasm_runtime_free(module);

        module = next_module;
    }

    os_mutex_unlock(&loading_module_list_lock);
}
#endif /* WASM_ENABLE_MULTI_MODULE */

bool
wasm_runtime_is_built_in_module(const char *module_name)
{
    return (!strcmp("env", module_name)
            || !strcmp("wasi_unstable", module_name)
            || !strcmp("wasi_snapshot_preview1", module_name)
            || !strcmp("spectest", module_name)
            || !strcmp("", module_name));
}

#if WASM_ENABLE_THREAD_MGR != 0
bool
wasm_exec_env_set_aux_stack(WASMExecEnv *exec_env,
                            uint32 start_offset, uint32 size)
{
    WASMModuleInstanceCommon *module_inst
        = wasm_exec_env_get_module_inst(exec_env);
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        return wasm_set_aux_stack(exec_env, start_offset, size);
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT) {
        return aot_set_aux_stack(exec_env, start_offset, size);
    }
#endif
    return false;
}

bool
wasm_exec_env_get_aux_stack(WASMExecEnv *exec_env,
                            uint32 *start_offset, uint32 *size)
{
    WASMModuleInstanceCommon *module_inst
        = wasm_exec_env_get_module_inst(exec_env);
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        return wasm_get_aux_stack(exec_env, start_offset, size);
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT) {
        return aot_get_aux_stack(exec_env, start_offset, size);
    }
#endif
    return false;
}

void
wasm_runtime_set_max_thread_num(uint32 num)
{
    wasm_cluster_set_max_thread_num(num);
}
#endif /* end of WASM_ENABLE_THREAD_MGR */

static WASMModuleCommon *
register_module_with_null_name(WASMModuleCommon *module_common,
                               char *error_buf, uint32 error_buf_size)
{
#if WASM_ENABLE_MULTI_MODULE != 0
    if (module_common) {
        if (!wasm_runtime_register_module_internal(NULL, module_common,
                                                   NULL, 0,
                                                   error_buf,
                                                   error_buf_size)) {
            wasm_runtime_unload(module_common);
            return NULL;
        }
        return module_common;
    }
    else
        return NULL;
#else
    return module_common;
#endif
}

WASMModuleCommon *
wasm_runtime_load(const uint8 *buf, uint32 size,
                  char *error_buf, uint32 error_buf_size)
{
    WASMModuleCommon *module_common = NULL;

    if (get_package_type(buf, size) == Wasm_Module_Bytecode) {
#if WASM_ENABLE_AOT != 0 && WASM_ENABLE_JIT != 0
        AOTModule *aot_module;
        WASMModule *module = wasm_load(buf, size, error_buf, error_buf_size);
        if (!module)
            return NULL;

        if (!(aot_module = aot_convert_wasm_module(module,
                                                   error_buf, error_buf_size))) {
            wasm_unload(module);
            return NULL;
        }

        module_common = (WASMModuleCommon*)aot_module;
        return register_module_with_null_name(module_common,
                                              error_buf, error_buf_size);
#elif WASM_ENABLE_INTERP != 0
        module_common = (WASMModuleCommon*)
               wasm_load(buf, size, error_buf, error_buf_size);
        return register_module_with_null_name(module_common,
                                              error_buf, error_buf_size);
#endif
    }
    else if (get_package_type(buf, size) == Wasm_Module_AoT) {
#if WASM_ENABLE_AOT != 0
        module_common = (WASMModuleCommon*)
               aot_load_from_aot_file(buf, size, error_buf, error_buf_size);
        return register_module_with_null_name(module_common,
                                              error_buf, error_buf_size);
#endif
    }

    if (size < 4)
        set_error_buf(error_buf, error_buf_size,
                      "WASM module load failed: unexpected end");
    else
       set_error_buf(error_buf, error_buf_size,
                     "WASM module load failed: magic header not detected");
    return NULL;
}

WASMModuleCommon *
wasm_runtime_load_from_sections(WASMSection *section_list, bool is_aot,
                                char *error_buf, uint32_t error_buf_size)
{
    WASMModuleCommon *module_common;

#if WASM_ENABLE_INTERP != 0
    if (!is_aot) {
        module_common = (WASMModuleCommon*)
               wasm_load_from_sections(section_list,
                                       error_buf, error_buf_size);
        return register_module_with_null_name(module_common,
                                              error_buf, error_buf_size);
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (is_aot) {
        module_common = (WASMModuleCommon*)
               aot_load_from_sections(section_list,
                                      error_buf, error_buf_size);
        return register_module_with_null_name(module_common,
                                              error_buf, error_buf_size);
    }
#endif

    set_error_buf(error_buf, error_buf_size,
                  "WASM module load failed: invalid section list type");
    return NULL;
}

void
wasm_runtime_unload(WASMModuleCommon *module)
{
#if WASM_ENABLE_MULTI_MODULE != 0
    /**
     * since we will unload and free all module when runtime_destroy()
     * we don't want users to unwillingly disrupt it
     */
    return;
#endif

#if WASM_ENABLE_INTERP != 0
    if (module->module_type == Wasm_Module_Bytecode) {
        wasm_unload((WASMModule*)module);
        return;
    }
#endif

#if WASM_ENABLE_AOT != 0
    if (module->module_type == Wasm_Module_AoT) {
        aot_unload((AOTModule*)module);
        return;
    }
#endif
}

WASMModuleInstanceCommon *
wasm_runtime_instantiate_internal(WASMModuleCommon *module, bool is_sub_inst,
                                  uint32 stack_size, uint32 heap_size,
                                  char *error_buf, uint32 error_buf_size)
{
#if WASM_ENABLE_INTERP != 0
    if (module->module_type == Wasm_Module_Bytecode)
        return (WASMModuleInstanceCommon*)
               wasm_instantiate((WASMModule*)module, is_sub_inst,
                                stack_size, heap_size,
                                error_buf, error_buf_size);
#endif
#if WASM_ENABLE_AOT != 0
    if (module->module_type == Wasm_Module_AoT)
        return (WASMModuleInstanceCommon*)
               aot_instantiate((AOTModule*)module, is_sub_inst,
                               stack_size, heap_size,
                               error_buf, error_buf_size);
#endif
    set_error_buf(error_buf, error_buf_size,
                  "Instantiate module failed, invalid module type");
    return NULL;
}

WASMModuleInstanceCommon *
wasm_runtime_instantiate(WASMModuleCommon *module,
                         uint32 stack_size, uint32 heap_size,
                         char *error_buf, uint32 error_buf_size)
{
    return wasm_runtime_instantiate_internal(module, false,
                                             stack_size, heap_size,
                                             error_buf, error_buf_size);
}

void
wasm_runtime_deinstantiate_internal(WASMModuleInstanceCommon *module_inst,
                                    bool is_sub_inst)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        wasm_deinstantiate((WASMModuleInstance*)module_inst, is_sub_inst);
        return;
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT) {
        aot_deinstantiate((AOTModuleInstance*)module_inst, is_sub_inst);
        return;
    }
#endif
}

void
wasm_runtime_deinstantiate(WASMModuleInstanceCommon *module_inst)
{
    return wasm_runtime_deinstantiate_internal(module_inst, false);
}

WASMExecEnv *
wasm_runtime_create_exec_env(WASMModuleInstanceCommon *module_inst,
                             uint32 stack_size)
{
    return wasm_exec_env_create(module_inst, stack_size);
}

void
wasm_runtime_destroy_exec_env(WASMExecEnv *exec_env)
{
    wasm_exec_env_destroy(exec_env);
}

#if (WASM_ENABLE_MEMORY_PROFILING != 0) || (WASM_ENABLE_MEMORY_TRACING != 0)
void
wasm_runtime_dump_module_mem_consumption(const WASMModuleCommon *module)
{
    WASMModuleMemConsumption mem_conspn = { 0 };

#if WASM_ENABLE_INTERP != 0
    if (module->module_type == Wasm_Module_Bytecode) {
        wasm_get_module_mem_consumption((WASMModule*)module, &mem_conspn);
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (module->module_type == Wasm_Module_AoT) {
        aot_get_module_mem_consumption((AOTModule*)module, &mem_conspn);
    }
#endif

    os_printf("WASM module memory consumption, total size: %u\n",
              mem_conspn.total_size);
    os_printf("    module struct size: %u\n", mem_conspn.module_struct_size);
    os_printf("    types size: %u\n", mem_conspn.types_size);
    os_printf("    imports size: %u\n", mem_conspn.imports_size);
    os_printf("    funcs size: %u\n", mem_conspn.functions_size);
    os_printf("    tables size: %u\n", mem_conspn.tables_size);
    os_printf("    memories size: %u\n", mem_conspn.memories_size);
    os_printf("    globals size: %u\n", mem_conspn.globals_size);
    os_printf("    exports size: %u\n", mem_conspn.exports_size);
    os_printf("    table segs size: %u\n", mem_conspn.table_segs_size);
    os_printf("    data segs size: %u\n", mem_conspn.data_segs_size);
    os_printf("    const strings size: %u\n", mem_conspn.const_strs_size);
#if WASM_ENABLE_AOT != 0
    os_printf("    aot code size: %u\n", mem_conspn.aot_code_size);
#endif
}

void
wasm_runtime_dump_module_inst_mem_consumption(const WASMModuleInstanceCommon
                                              *module_inst)
{
    WASMModuleInstMemConsumption mem_conspn = { 0 };

#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        wasm_get_module_inst_mem_consumption((WASMModuleInstance*)module_inst,
                                             &mem_conspn);
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT) {
        aot_get_module_inst_mem_consumption((AOTModuleInstance*)module_inst,
                                            &mem_conspn);
    }
#endif

    os_printf("WASM module inst memory consumption, total size: %u\n",
              mem_conspn.total_size);
    os_printf("    module inst struct size: %u\n",
              mem_conspn.module_inst_struct_size);
    os_printf("    memories size: %u\n", mem_conspn.memories_size);
    os_printf("        app heap size: %u\n", mem_conspn.app_heap_size);
    os_printf("    tables size: %u\n", mem_conspn.tables_size);
    os_printf("    functions size: %u\n", mem_conspn.functions_size);
    os_printf("    globals size: %u\n", mem_conspn.globals_size);
    os_printf("    exports size: %u\n", mem_conspn.exports_size);
}

void
wasm_runtime_dump_exec_env_mem_consumption(const WASMExecEnv *exec_env)
{
    uint32 total_size = offsetof(WASMExecEnv, wasm_stack.s.bottom)
                        + exec_env->wasm_stack_size;

    os_printf("Exec env memory consumption, total size: %u\n", total_size);
    os_printf("    exec env struct size: %u\n",
              offsetof(WASMExecEnv, wasm_stack.s.bottom));
#if WASM_ENABLE_INTERP != 0 && WASM_ENABLE_FAST_INTERP == 0
    os_printf("        block addr cache size: %u\n",
              sizeof(exec_env->block_addr_cache));
#endif
    os_printf("    stack size: %u\n", exec_env->wasm_stack_size);
}

uint32
gc_get_heap_highmark_size(void *heap);

void
wasm_runtime_dump_mem_consumption(WASMExecEnv *exec_env)
{
    WASMModuleInstMemConsumption module_inst_mem_consps;
    WASMModuleMemConsumption module_mem_consps;
    WASMModuleInstanceCommon *module_inst_common;
    WASMModuleCommon *module_common = NULL;
    void *heap_handle = NULL;
    uint32 total_size = 0, app_heap_peak_size = 0;
    uint32 max_aux_stack_used = -1;

    module_inst_common = exec_env->module_inst;
#if WASM_ENABLE_INTERP != 0
    if (module_inst_common->module_type == Wasm_Module_Bytecode) {
        WASMModuleInstance *wasm_module_inst =
                    (WASMModuleInstance*)module_inst_common;
        WASMModule *wasm_module = wasm_module_inst->module;
        module_common = (WASMModuleCommon*)wasm_module;
        if (wasm_module_inst->memories) {
            heap_handle = wasm_module_inst->memories[0]->heap_handle;
        }
        wasm_get_module_inst_mem_consumption
                    (wasm_module_inst, &module_inst_mem_consps);
        wasm_get_module_mem_consumption
                    (wasm_module, &module_mem_consps);
        if (wasm_module_inst->module->aux_stack_top_global_index != (uint32)-1)
            max_aux_stack_used = wasm_module_inst->max_aux_stack_used;
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst_common->module_type == Wasm_Module_AoT) {
        AOTModuleInstance *aot_module_inst =
                    (AOTModuleInstance*)module_inst_common;
        AOTModule *aot_module =
                    (AOTModule*)aot_module_inst->aot_module.ptr;
        module_common = (WASMModuleCommon*)aot_module;
        if (aot_module_inst->memories.ptr) {
            AOTMemoryInstance **memories =
               (AOTMemoryInstance **)aot_module_inst->memories.ptr;
            heap_handle = memories[0]->heap_handle.ptr;
        }
        aot_get_module_inst_mem_consumption
                    (aot_module_inst, &module_inst_mem_consps);
        aot_get_module_mem_consumption
                    (aot_module, &module_mem_consps);
    }
#endif

    bh_assert(module_common != NULL);

    if (heap_handle) {
        app_heap_peak_size = gc_get_heap_highmark_size(heap_handle);
    }

    total_size = offsetof(WASMExecEnv, wasm_stack.s.bottom)
                 + exec_env->wasm_stack_size
                 + module_mem_consps.total_size
                 + module_inst_mem_consps.total_size;

    os_printf("\nMemory consumption summary (bytes):\n");
    wasm_runtime_dump_module_mem_consumption(module_common);
    wasm_runtime_dump_module_inst_mem_consumption(module_inst_common);
    wasm_runtime_dump_exec_env_mem_consumption(exec_env);
    os_printf("\nTotal memory consumption of module, module inst and "
              "exec env: %u\n", total_size);
    os_printf("Total interpreter stack used: %u\n",
              exec_env->max_wasm_stack_used);

    if (max_aux_stack_used != (uint32)-1)
        os_printf("Total auxiliary stack used: %u\n", max_aux_stack_used);
    else
        os_printf("Total aux stack used: no enough info to profile\n");

    os_printf("Total app heap used: %u\n", app_heap_peak_size);
}
#endif /* end of (WASM_ENABLE_MEMORY_PROFILING != 0)
                 || (WASM_ENABLE_MEMORY_TRACING != 0) */

#if WASM_ENABLE_PERF_PROFILING != 0
void
wasm_runtime_dump_perf_profiling(WASMModuleInstanceCommon *module_inst)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        wasm_dump_perf_profiling((WASMModuleInstance*)module_inst);
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT) {
        aot_dump_perf_profiling((AOTModuleInstance*)module_inst);
    }
#endif
}
#endif

WASMModuleInstanceCommon *
wasm_runtime_get_module_inst(WASMExecEnv *exec_env)
{
    return wasm_exec_env_get_module_inst(exec_env);
}

void *
wasm_runtime_get_function_attachment(WASMExecEnv *exec_env)
{
    return exec_env->attachment;
}

void
wasm_runtime_set_user_data(WASMExecEnv *exec_env, void *user_data)
{
    exec_env->user_data = user_data;
}

void *
wasm_runtime_get_user_data(WASMExecEnv *exec_env)
{
    return exec_env->user_data;
}

WASMFunctionInstanceCommon *
wasm_runtime_lookup_function(WASMModuleInstanceCommon * const module_inst,
                             const char *name,
                             const char *signature)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode)
        return (WASMFunctionInstanceCommon*)
               wasm_lookup_function((const WASMModuleInstance*)module_inst,
                                    name, signature);
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT)
        return (WASMFunctionInstanceCommon*)
               aot_lookup_function((const AOTModuleInstance*)module_inst,
                                   name, signature);
#endif
    return NULL;
}

bool
wasm_runtime_call_wasm(WASMExecEnv *exec_env,
                       WASMFunctionInstanceCommon *function,
                       uint32 argc, uint32 argv[])
{
    if (!wasm_runtime_exec_env_check(exec_env)) {
        LOG_ERROR("Invalid exec env stack info.");
        return false;
    }

    /* set thread handle and stack boundary */
    wasm_exec_env_set_thread_info(exec_env);

#if WASM_ENABLE_INTERP != 0
    if (exec_env->module_inst->module_type == Wasm_Module_Bytecode)
        return wasm_call_function(exec_env,
                                  (WASMFunctionInstance*)function,
                                  argc, argv);
#endif
#if WASM_ENABLE_AOT != 0
    if (exec_env->module_inst->module_type == Wasm_Module_AoT)
        return aot_call_function(exec_env,
                                 (AOTFunctionInstance*)function,
                                 argc, argv);
#endif
    return false;
}

static uint32
parse_args_to_uint32_array(WASMType *type,
                           uint32 num_args, wasm_val_t *args,
                           uint32 *out_argv)
{
    uint32 i, p;

    for (i = 0, p = 0; i < num_args; i++) {
        switch (args[i].kind) {
            case WASM_I32:
                out_argv[p++] = args[i].of.i32;
                break;
            case WASM_I64:
            {
                union { uint64 val; uint32 parts[2]; } u;
                u.val = args[i].of.i64;
                out_argv[p++] = u.parts[0];
                out_argv[p++] = u.parts[1];
                break;
            }
            case WASM_F32:
            {
                union { float32 val; uint32 part; } u;
                u.val = args[i].of.f32;
                out_argv[p++] = u.part;
                break;
            }
            case WASM_F64:
            {
                union { float64 val; uint32 parts[2]; } u;
                u.val = args[i].of.f64;
                out_argv[p++] = u.parts[0];
                out_argv[p++] = u.parts[1];
                break;
            }
            default:
                bh_assert(0);
                break;
        }
    }
    return p;
}

static uint32
parse_uint32_array_to_results(WASMType *type,
                              uint32 argc, uint32 *argv,
                              wasm_val_t *out_results)
{
    uint32 i, p;

    for (i = 0, p = 0; i < type->result_count; i++) {
        switch (type->types[type->param_count + i]) {
            case VALUE_TYPE_I32:
                out_results[i].kind = WASM_I32;
                out_results[i].of.i32 = (int32)argv[p++];
                break;
            case VALUE_TYPE_I64:
            {
                union { uint64 val; uint32 parts[2]; } u;
                u.parts[0] = argv[p++];
                u.parts[1] = argv[p++];
                out_results[i].kind = WASM_I64;
                out_results[i].of.i64 = u.val;
                break;
            }
            case VALUE_TYPE_F32:
            {
                union { float32 val; uint32 part; } u;
                u.part = argv[p++];
                out_results[i].kind = WASM_F32;
                out_results[i].of.f32 = u.val;
                break;
            }
            case VALUE_TYPE_F64:
            {
                union { float64 val; uint32 parts[2]; } u;
                u.parts[0] = argv[p++];
                u.parts[1] = argv[p++];
                out_results[i].kind = WASM_F64;
                out_results[i].of.f64 = u.val;
                break;
            }
            default:
                bh_assert(0);
                break;
        }
    }
    bh_assert(argc == p);
    return type->result_count;
}

bool
wasm_runtime_call_wasm_a(WASMExecEnv *exec_env,
                         WASMFunctionInstanceCommon *function,
                         uint32 num_results, wasm_val_t results[],
                         uint32 num_args, wasm_val_t args[])
{
    uint32 argc, *argv, ret_num, cell_num, total_size;
    bool ret = false;
    WASMType *type = NULL;

#if WASM_ENABLE_INTERP != 0
    if (exec_env->module_inst->module_type == Wasm_Module_Bytecode) {
        WASMFunctionInstance *wasm_func = (WASMFunctionInstance*)function;
        type = wasm_func->u.func->func_type;
        argc = wasm_func->param_cell_num;
        cell_num = argc > wasm_func->ret_cell_num ?
                   argc : wasm_func->ret_cell_num;
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (exec_env->module_inst->module_type == Wasm_Module_AoT) {
        type = ((AOTFunctionInstance*)function)->u.func.func_type;
        argc = type->param_cell_num;
        cell_num = argc > type->ret_cell_num ?
                   argc : type->ret_cell_num;
    }
#endif
    if (!type) {
        LOG_ERROR("Function type get failed, WAMR Interpreter and AOT must be enabled at least one.");
        goto fail1;
    }

    if (num_results != type->result_count) {
        LOG_ERROR("The result value number does not match the function declaration.");
        goto fail1;
    }

    if (num_args != type->param_count) {
        LOG_ERROR("The argument value number does not match the function declaration.");
        goto fail1;
    }

    total_size = sizeof(uint32) * (uint64)(cell_num > 2 ? cell_num : 2);
    if (!(argv = runtime_malloc((uint32)total_size, exec_env->module_inst, NULL, 0))) {
        wasm_runtime_set_exception(exec_env->module_inst, "allocate memory failed");
        goto fail1;
    }

    argc = parse_args_to_uint32_array(type, num_args, args, argv);
    if (!(ret = wasm_runtime_call_wasm(exec_env, function, argc, argv)))
        goto fail2;

    ret_num = parse_uint32_array_to_results(type, type->ret_cell_num, argv, results);
    bh_assert(ret_num == num_results);
    (void)ret_num;

fail2:
    wasm_runtime_free(argv);
fail1:
    return ret;
}

bool
wasm_runtime_call_wasm_v(WASMExecEnv *exec_env,
                         WASMFunctionInstanceCommon *function,
                         uint32 num_results, wasm_val_t results[],
                         uint32 num_args, ...)
{
    wasm_val_t *args = NULL;
    WASMType *type = NULL;
    bool ret = false;
    uint32 i = 0;
    va_list vargs;

#if WASM_ENABLE_INTERP != 0
    if (exec_env->module_inst->module_type == Wasm_Module_Bytecode) {
        WASMFunctionInstance *wasm_func = (WASMFunctionInstance*)function;
        type = wasm_func->u.func->func_type;
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (exec_env->module_inst->module_type == Wasm_Module_AoT) {
        type = ((AOTFunctionInstance*)function)->u.func.func_type;
    }
#endif
    if (!type) {
        LOG_ERROR("Function type get failed, WAMR Interpreter and AOT must be enabled at least one.");
        goto fail1;
    }

    if (num_args != type->param_count) {
        LOG_ERROR("The argument value number does not match the function declaration.");
        goto fail1;
    }
    if (!(args = runtime_malloc(sizeof(wasm_val_t) * num_args, NULL, NULL, 0))) {
        wasm_runtime_set_exception(exec_env->module_inst, "allocate memory failed");
        goto fail1;
    }

    va_start(vargs, num_args);
    for (i = 0; i < num_args; i++) {
        switch (type->types[i]) {
            case VALUE_TYPE_I32:
                args[i].kind = WASM_I32;
                args[i].of.i32 = va_arg(vargs, uint32);
                break;
            case VALUE_TYPE_I64:
                args[i].kind = WASM_I64;
                args[i].of.i64 = va_arg(vargs, uint64);
                break;
            case VALUE_TYPE_F32:
                args[i].kind = WASM_F32;
                args[i].of.f32 = (float32)va_arg(vargs, float64);
                break;
            case VALUE_TYPE_F64:
                args[i].kind = WASM_F64;
                args[i].of.f64 = va_arg(vargs, float64);;
                break;
            default:
                bh_assert(0);
                break;
        }
    }
    va_end(vargs);
    ret = wasm_runtime_call_wasm_a(exec_env, function, num_results, results, num_args, args);
    wasm_runtime_free(args);

fail1:
    return ret;
}

bool
wasm_runtime_create_exec_env_and_call_wasm(WASMModuleInstanceCommon *module_inst,
                                           WASMFunctionInstanceCommon *function,
                                           uint32 argc, uint32 argv[])
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode)
        return wasm_create_exec_env_and_call_function(
                                    (WASMModuleInstance*)module_inst,
                                    (WASMFunctionInstance*)function,
                                    argc, argv);
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT)
        return aot_create_exec_env_and_call_function(
                                    (AOTModuleInstance*)module_inst,
                                    (AOTFunctionInstance*)function,
                                    argc, argv);
#endif
    return false;
}

void
wasm_runtime_set_exception(WASMModuleInstanceCommon *module_inst,
                           const char *exception)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        wasm_set_exception((WASMModuleInstance*)module_inst, exception);
        return;
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT) {
        aot_set_exception((AOTModuleInstance*)module_inst, exception);
        return;
    }
#endif
}

const char*
wasm_runtime_get_exception(WASMModuleInstanceCommon *module_inst)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        return wasm_get_exception((WASMModuleInstance*)module_inst);
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT) {
        return aot_get_exception((AOTModuleInstance*)module_inst);
    }
#endif
    return NULL;
}

void
wasm_runtime_clear_exception(WASMModuleInstanceCommon *module_inst)
{
    wasm_runtime_set_exception(module_inst, NULL);
}

void
wasm_runtime_set_custom_data(WASMModuleInstanceCommon *module_inst,
                             void *custom_data)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        ((WASMModuleInstance*)module_inst)->custom_data = custom_data;
        return;
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT) {
        ((AOTModuleInstance*)module_inst)->custom_data.ptr = custom_data;
        return;
    }
#endif
}

void*
wasm_runtime_get_custom_data(WASMModuleInstanceCommon *module_inst)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode)
        return ((WASMModuleInstance*)module_inst)->custom_data;
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT)
        return ((AOTModuleInstance*)module_inst)->custom_data.ptr;
#endif
    return NULL;
}

uint32
wasm_runtime_module_malloc(WASMModuleInstanceCommon *module_inst, uint32 size,
                           void **p_native_addr)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode)
        return wasm_module_malloc((WASMModuleInstance*)module_inst, size,
                                  p_native_addr);
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT)
        return aot_module_malloc((AOTModuleInstance*)module_inst, size,
                                 p_native_addr);
#endif
    return 0;
}

void
wasm_runtime_module_free(WASMModuleInstanceCommon *module_inst, uint32 ptr)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        wasm_module_free((WASMModuleInstance*)module_inst, ptr);
        return;
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT) {
        aot_module_free((AOTModuleInstance*)module_inst, ptr);
        return;
    }
#endif
}

uint32
wasm_runtime_module_dup_data(WASMModuleInstanceCommon *module_inst,
                             const char *src, uint32 size)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        return wasm_module_dup_data((WASMModuleInstance*)module_inst, src, size);
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT) {
        return aot_module_dup_data((AOTModuleInstance*)module_inst, src, size);
    }
#endif
    return 0;
}

bool
wasm_runtime_validate_app_addr(WASMModuleInstanceCommon *module_inst,
                               uint32 app_offset, uint32 size)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode)
        return wasm_validate_app_addr((WASMModuleInstance*)module_inst,
                                      app_offset, size);
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT)
        return aot_validate_app_addr((AOTModuleInstance*)module_inst,
                                     app_offset, size);
#endif
    return false;
}

bool
wasm_runtime_validate_app_str_addr(WASMModuleInstanceCommon *module_inst,
                                   uint32 app_str_offset)
{
    uint32 app_end_offset;
    char *str, *str_end;

    if (!wasm_runtime_get_app_addr_range(module_inst, app_str_offset,
                                         NULL, &app_end_offset))
        goto fail;

    str = wasm_runtime_addr_app_to_native(module_inst, app_str_offset);
    str_end = str + (app_end_offset - app_str_offset);
    while (str < str_end && *str != '\0')
        str++;
    if (str == str_end)
        goto fail;
    return true;

fail:
    wasm_runtime_set_exception(module_inst, "out of bounds memory access");
    return false;
}

bool
wasm_runtime_validate_native_addr(WASMModuleInstanceCommon *module_inst,
                                  void *native_ptr, uint32 size)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode)
        return wasm_validate_native_addr((WASMModuleInstance*)module_inst,
                                         native_ptr, size);
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT)
        return aot_validate_native_addr((AOTModuleInstance*)module_inst,
                                        native_ptr, size);
#endif
    return false;
}

void *
wasm_runtime_addr_app_to_native(WASMModuleInstanceCommon *module_inst,
                                uint32 app_offset)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode)
        return wasm_addr_app_to_native((WASMModuleInstance*)module_inst,
                                       app_offset);
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT)
        return aot_addr_app_to_native((AOTModuleInstance*)module_inst,
                                      app_offset);
#endif
    return NULL;
}

uint32
wasm_runtime_addr_native_to_app(WASMModuleInstanceCommon *module_inst,
                                void *native_ptr)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode)
        return wasm_addr_native_to_app((WASMModuleInstance*)module_inst,
                                       native_ptr);
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT)
        return aot_addr_native_to_app((AOTModuleInstance*)module_inst,
                                      native_ptr);
#endif
    return 0;
}

bool
wasm_runtime_get_app_addr_range(WASMModuleInstanceCommon *module_inst,
                                uint32 app_offset,
                                uint32 *p_app_start_offset,
                                uint32 *p_app_end_offset)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode)
        return wasm_get_app_addr_range((WASMModuleInstance*)module_inst,
                                      app_offset, p_app_start_offset,
                                      p_app_end_offset);
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT)
        return aot_get_app_addr_range((AOTModuleInstance*)module_inst,
                                      app_offset, p_app_start_offset,
                                      p_app_end_offset);
#endif
    return false;
}

bool
wasm_runtime_get_native_addr_range(WASMModuleInstanceCommon *module_inst,
                                   uint8 *native_ptr,
                                   uint8 **p_native_start_addr,
                                   uint8 **p_native_end_addr)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode)
        return wasm_get_native_addr_range((WASMModuleInstance*)module_inst,
                                          native_ptr, p_native_start_addr,
                                          p_native_end_addr);
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT)
        return aot_get_native_addr_range((AOTModuleInstance*)module_inst,
                                         native_ptr, p_native_start_addr,
                                         p_native_end_addr);
#endif
    return false;
}

uint32
wasm_runtime_get_temp_ret(WASMModuleInstanceCommon *module_inst)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode)
        return ((WASMModuleInstance*)module_inst)->temp_ret;
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT)
        return ((AOTModuleInstance*)module_inst)->temp_ret;
#endif
    return 0;
}

void
wasm_runtime_set_temp_ret(WASMModuleInstanceCommon *module_inst,
                          uint32 temp_ret)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        ((WASMModuleInstance*)module_inst)->temp_ret = temp_ret;
        return;
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT) {
       ((AOTModuleInstance*)module_inst)->temp_ret = temp_ret;
       return;
    }
#endif
}

uint32
wasm_runtime_get_llvm_stack(WASMModuleInstanceCommon *module_inst)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode)
        return ((WASMModuleInstance*)module_inst)->llvm_stack;
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT)
        return ((AOTModuleInstance*)module_inst)->llvm_stack;
#endif
    return 0;
}

void
wasm_runtime_set_llvm_stack(WASMModuleInstanceCommon *module_inst,
                            uint32 llvm_stack)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        ((WASMModuleInstance*)module_inst)->llvm_stack = llvm_stack;
        return;
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT) {
       ((AOTModuleInstance*)module_inst)->llvm_stack = llvm_stack;
       return;
    }
#endif
}

bool
wasm_runtime_enlarge_memory(WASMModuleInstanceCommon *module,
                            uint32 inc_page_count)
{
#if WASM_ENABLE_INTERP != 0
    if (module->module_type == Wasm_Module_Bytecode)
        return wasm_enlarge_memory((WASMModuleInstance*)module,
                                   inc_page_count);
#endif
#if WASM_ENABLE_AOT != 0
    if (module->module_type == Wasm_Module_AoT)
        return aot_enlarge_memory((AOTModuleInstance*)module,
                                  inc_page_count);
#endif
    return false;
}

#if WASM_ENABLE_LIBC_WASI != 0
void
wasm_runtime_set_wasi_args(WASMModuleCommon *module,
                           const char *dir_list[], uint32 dir_count,
                           const char *map_dir_list[], uint32 map_dir_count,
                           const char *env_list[], uint32 env_count,
                           char *argv[], int argc)
{
    WASIArguments *wasi_args = NULL;

#if WASM_ENABLE_INTERP != 0 || WASM_ENABLE_JIT != 0
    if (module->module_type == Wasm_Module_Bytecode)
        wasi_args = &((WASMModule*)module)->wasi_args;
#endif
#if WASM_ENABLE_AOT != 0
    if (module->module_type == Wasm_Module_AoT)
        wasi_args = &((AOTModule*)module)->wasi_args;
#endif

    if (wasi_args) {
        wasi_args->dir_list = dir_list;
        wasi_args->dir_count = dir_count;
        wasi_args->map_dir_list = map_dir_list;
        wasi_args->map_dir_count = map_dir_count;
        wasi_args->env = env_list;
        wasi_args->env_count = env_count;
        wasi_args->argv = argv;
        wasi_args->argc = (uint32)argc;
    }
}

#if WASM_ENABLE_UVWASI == 0
bool
wasm_runtime_init_wasi(WASMModuleInstanceCommon *module_inst,
                       const char *dir_list[], uint32 dir_count,
                       const char *map_dir_list[], uint32 map_dir_count,
                       const char *env[], uint32 env_count,
                       char *argv[], uint32 argc,
                       char *error_buf, uint32 error_buf_size)
{
    WASIContext *wasi_ctx;
    char *argv_buf = NULL;
    char **argv_list = NULL;
    char *env_buf = NULL;
    char **env_list = NULL;
    uint64 argv_buf_size = 0, env_buf_size = 0, total_size;
    uint32 argv_buf_offset = 0, env_buf_offset = 0;
    struct fd_table *curfds = NULL;
    struct fd_prestats *prestats = NULL;
    struct argv_environ_values *argv_environ = NULL;
    bool fd_table_inited = false, fd_prestats_inited = false;
    bool argv_environ_inited = false;
    __wasi_fd_t wasm_fd = 3;
    int32 raw_fd;
    char *path, resolved_path[PATH_MAX];
    uint32 i;

    if (!(wasi_ctx = runtime_malloc(sizeof(WASIContext), NULL,
                                    error_buf, error_buf_size))) {
        return false;
    }

    wasm_runtime_set_wasi_ctx(module_inst, wasi_ctx);

#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode
        && !((WASMModuleInstance*)module_inst)->default_memory)
        return true;
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT
        && !((AOTModuleInstance*)module_inst)->
                global_table_data.memory_instances[0].memory_data.ptr)
        return true;
#endif

    /* process argv[0], trip the path and suffix, only keep the program name */
    for (i = 0; i < argc; i++)
        argv_buf_size += strlen(argv[i]) + 1;

    total_size = sizeof(char *) * (uint64)argc;
    if (total_size >= UINT32_MAX
        || (total_size > 0 &&
            !(argv_list = wasm_runtime_malloc((uint32)total_size)))
        || argv_buf_size >= UINT32_MAX
        || (argv_buf_size > 0 &&
            !(argv_buf = wasm_runtime_malloc((uint32)argv_buf_size)))) {
        set_error_buf(error_buf, error_buf_size,
                      "Init wasi environment failed: allocate memory failed");
        goto fail;
    }

    for (i = 0; i < argc; i++) {
        argv_list[i] = argv_buf + argv_buf_offset;
        bh_strcpy_s(argv_buf + argv_buf_offset,
                    (uint32)argv_buf_size - argv_buf_offset, argv[i]);
        argv_buf_offset += (uint32)(strlen(argv[i]) + 1);
    }

    for (i = 0; i < env_count; i++)
        env_buf_size += strlen(env[i]) + 1;

    total_size = sizeof(char *) * (uint64)env_count;
    if (total_size >= UINT32_MAX
        || (total_size > 0
            && !(env_list = wasm_runtime_malloc((uint32)total_size)))
        || env_buf_size >= UINT32_MAX
        || (env_buf_size > 0
            && !(env_buf = wasm_runtime_malloc((uint32)env_buf_size)))) {
        set_error_buf(error_buf, error_buf_size,
                      "Init wasi environment failed: allocate memory failed");
        goto fail;
    }

    for (i = 0; i < env_count; i++) {
        env_list[i] = env_buf + env_buf_offset;
        bh_strcpy_s(env_buf + env_buf_offset,
                    (uint32)env_buf_size - env_buf_offset, env[i]);
        env_buf_offset += (uint32)(strlen(env[i]) + 1);
    }

    if (!(curfds = wasm_runtime_malloc(sizeof(struct fd_table)))
        || !(prestats = wasm_runtime_malloc(sizeof(struct fd_prestats)))
        || !(argv_environ =
                wasm_runtime_malloc(sizeof(struct argv_environ_values)))) {
        set_error_buf(error_buf, error_buf_size,
                      "Init wasi environment failed: allocate memory failed");
        goto fail;
    }

    if (!fd_table_init(curfds)) {
        set_error_buf(error_buf, error_buf_size,
                      "Init wasi environment failed: "
                      "init fd table failed");
        goto fail;
    }
    fd_table_inited = true;

    if (!fd_prestats_init(prestats)) {
        set_error_buf(error_buf, error_buf_size,
                      "Init wasi environment failed: "
                      "init fd prestats failed");
        goto fail;
    }
    fd_prestats_inited = true;

    if (!argv_environ_init(argv_environ,
                           argv_buf, argv_buf_size,
                           argv_list, argc,
                           env_buf, env_buf_size,
                           env_list, env_count)) {
        set_error_buf(error_buf, error_buf_size,
                      "Init wasi environment failed: "
                      "init argument environment failed");
        goto fail;
    }
    argv_environ_inited = true;

    /* Prepopulate curfds with stdin, stdout, and stderr file descriptors. */
    if (!fd_table_insert_existing(curfds, 0, 0)
        || !fd_table_insert_existing(curfds, 1, 1)
        || !fd_table_insert_existing(curfds, 2, 2)) {
        set_error_buf(error_buf, error_buf_size,
                      "Init wasi environment failed: init fd table failed");
        goto fail;
    }

    wasm_fd = 3;
    for (i = 0; i < dir_count; i++, wasm_fd++) {
        path = realpath(dir_list[i], resolved_path);
        if (!path) {
            if (error_buf)
                snprintf(error_buf, error_buf_size,
                         "error while pre-opening directory %s: %d\n",
                         dir_list[i], errno);
            goto fail;
        }

        raw_fd = open(path, O_RDONLY | O_DIRECTORY, 0);
        if (raw_fd == -1) {
            if (error_buf)
                snprintf(error_buf, error_buf_size,
                         "error while pre-opening directory %s: %d\n",
                         dir_list[i], errno);
            goto fail;
        }

        fd_table_insert_existing(curfds, wasm_fd, raw_fd);
        fd_prestats_insert(prestats, dir_list[i], wasm_fd);
    }

    wasi_ctx->curfds = curfds;
    wasi_ctx->prestats = prestats;
    wasi_ctx->argv_environ = argv_environ;
    wasi_ctx->argv_buf = argv_buf;
    wasi_ctx->argv_list = argv_list;
    wasi_ctx->env_buf = env_buf;
    wasi_ctx->env_list = env_list;

    return true;

fail:
    if (argv_environ_inited)
        argv_environ_destroy(argv_environ);
    if (fd_prestats_inited)
        fd_prestats_destroy(prestats);
    if (fd_table_inited)
        fd_table_destroy(curfds);
    if (curfds)
        wasm_runtime_free(curfds);
    if (prestats)
        wasm_runtime_free(prestats);
    if (argv_environ)
        wasm_runtime_free(argv_environ);
    if (argv_buf)
        wasm_runtime_free(argv_buf);
    if (argv_list)
        wasm_runtime_free(argv_list);
    if (env_buf)
        wasm_runtime_free(env_buf);
    if (env_list)
        wasm_runtime_free(env_list);
    return false;
}
#else /* else of WASM_ENABLE_UVWASI == 0 */
static void *
wasm_uvwasi_malloc(size_t size, void *mem_user_data)
{
    return runtime_malloc(size, NULL, NULL, 0);
    (void)mem_user_data;
}

static void
wasm_uvwasi_free(void *ptr, void *mem_user_data)
{
    if (ptr)
        wasm_runtime_free(ptr);
    (void)mem_user_data;
}

static void *
wasm_uvwasi_calloc(size_t nmemb, size_t size,
                   void *mem_user_data)
{
    uint64 total_size = (uint64)nmemb * size;
    return runtime_malloc(total_size, NULL, NULL, 0);
    (void)mem_user_data;
}

static void *
wasm_uvwasi_realloc(void *ptr, size_t size,
                    void *mem_user_data)
{
    if (size >= UINT32_MAX) {
        return NULL;
    }
    return wasm_runtime_realloc(ptr, (uint32)size);
}

static uvwasi_mem_t uvwasi_allocator = {
    .mem_user_data = 0,
    .malloc = wasm_uvwasi_malloc,
    .free = wasm_uvwasi_free,
    .calloc = wasm_uvwasi_calloc,
    .realloc = wasm_uvwasi_realloc
};

bool
wasm_runtime_init_wasi(WASMModuleInstanceCommon *module_inst,
                       const char *dir_list[], uint32 dir_count,
                       const char *map_dir_list[], uint32 map_dir_count,
                       const char *env[], uint32 env_count,
                       char *argv[], uint32 argc,
                       char *error_buf, uint32 error_buf_size)
{
    uvwasi_t *uvwasi = NULL;
    uvwasi_options_t init_options;
    const char **envp = NULL;
    uint64 total_size;
    uint32 i;
    bool ret = false;

    uvwasi = runtime_malloc(sizeof(uvwasi_t), module_inst,
                            error_buf, error_buf_size);
    if (!uvwasi)
        return false;

    /* Setup the initialization options */
    uvwasi_options_init(&init_options);
    init_options.allocator = &uvwasi_allocator;
    init_options.argc = argc;
    init_options.argv = (const char **)argv;

    if (dir_count > 0) {
        init_options.preopenc = dir_count;

        total_size = sizeof(uvwasi_preopen_t) * (uint64)init_options.preopenc;
        init_options.preopens =
            (uvwasi_preopen_t *)runtime_malloc(total_size, module_inst,
                                               error_buf, error_buf_size);
        if (init_options.preopens == NULL)
            goto fail;

        for (i = 0; i < init_options.preopenc; i++) {
            init_options.preopens[i].real_path = dir_list[i];
            init_options.preopens[i].mapped_path =
                    (i < map_dir_count) ? map_dir_list[i] : dir_list[i];
        }
    }

    if (env_count > 0) {
        total_size = sizeof(char *) * (uint64)(env_count + 1);
        envp = runtime_malloc(total_size, module_inst,
                              error_buf, error_buf_size);
        if (envp == NULL)
            goto fail;

        for (i = 0; i < env_count; i++) {
            envp[i] = env[i];
        }
        envp[env_count] = NULL;
        init_options.envp = envp;
    }

    if (UVWASI_ESUCCESS != uvwasi_init(uvwasi, &init_options)) {
        set_error_buf(error_buf, error_buf_size, "uvwasi init failed");
        goto fail;
    }

    wasm_runtime_set_wasi_ctx(module_inst, uvwasi);

    ret = true;

fail:
    if (envp)
        wasm_runtime_free(envp);

    if (init_options.preopens)
        wasm_runtime_free(init_options.preopens);

    if (!ret && uvwasi)
        wasm_runtime_free(uvwasi);

    return ret;
}
#endif /* end of WASM_ENABLE_UVWASI */

bool
wasm_runtime_is_wasi_mode(WASMModuleInstanceCommon *module_inst)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode
        && ((WASMModuleInstance*)module_inst)->module->is_wasi_module)
        return true;
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT
        && ((AOTModule*)((AOTModuleInstance*)module_inst)->aot_module.ptr)
           ->is_wasi_module)
        return true;
#endif
    return false;
}

WASMFunctionInstanceCommon *
wasm_runtime_lookup_wasi_start_function(WASMModuleInstanceCommon *module_inst)
{
    uint32 i;

#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        WASMModuleInstance *wasm_inst = (WASMModuleInstance*)module_inst;
        WASMFunctionInstance *func;
        for (i = 0; i < wasm_inst->export_func_count; i++) {
            if (!strcmp(wasm_inst->export_functions[i].name, "_start")) {
                func = wasm_inst->export_functions[i].function;
                if (func->u.func->func_type->param_count != 0
                    || func->u.func->func_type->result_count != 0) {
                    LOG_ERROR("Lookup wasi _start function failed: "
                              "invalid function type.\n");
                    return NULL;
                }
                return (WASMFunctionInstanceCommon*)func;
            }
        }
        return NULL;
    }
#endif

#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT) {
        AOTModuleInstance *aot_inst = (AOTModuleInstance*)module_inst;
        AOTFunctionInstance *export_funcs = (AOTFunctionInstance *)
                                            aot_inst->export_funcs.ptr;
        for (i = 0; i < aot_inst->export_func_count; i++) {
            if (!strcmp(export_funcs[i].func_name, "_start")) {
                AOTFuncType *func_type = export_funcs[i].u.func.func_type;
                if (func_type->param_count != 0
                    || func_type->result_count != 0) {
                    LOG_ERROR("Lookup wasi _start function failed: "
                              "invalid function type.\n");
                    return NULL;
                }
                return (WASMFunctionInstanceCommon*)&export_funcs[i];
            }
        }
        return NULL;
    }
#endif /* end of WASM_ENABLE_AOT */

    return NULL;
}

#if WASM_ENABLE_UVWASI == 0
void
wasm_runtime_destroy_wasi(WASMModuleInstanceCommon *module_inst)
{
    WASIContext *wasi_ctx = wasm_runtime_get_wasi_ctx(module_inst);

    if (wasi_ctx) {
        if (wasi_ctx->argv_environ) {
            argv_environ_destroy(wasi_ctx->argv_environ);
            wasm_runtime_free(wasi_ctx->argv_environ);
        }
        if (wasi_ctx->curfds) {
            fd_table_destroy(wasi_ctx->curfds);
            wasm_runtime_free(wasi_ctx->curfds);
        }
        if (wasi_ctx->prestats) {
            fd_prestats_destroy(wasi_ctx->prestats);
            wasm_runtime_free(wasi_ctx->prestats);
        }
        if (wasi_ctx->argv_buf)
            wasm_runtime_free(wasi_ctx->argv_buf);
        if (wasi_ctx->argv_list)
            wasm_runtime_free(wasi_ctx->argv_list);
        if (wasi_ctx->env_buf)
            wasm_runtime_free(wasi_ctx->env_buf);
        if (wasi_ctx->env_list)
            wasm_runtime_free(wasi_ctx->env_list);
        wasm_runtime_free(wasi_ctx);
    }
}
#else
void
wasm_runtime_destroy_wasi(WASMModuleInstanceCommon *module_inst)
{
    WASIContext *wasi_ctx = wasm_runtime_get_wasi_ctx(module_inst);

    if (wasi_ctx) {
        uvwasi_destroy(wasi_ctx);
        wasm_runtime_free(wasi_ctx);
    }
}
#endif

WASIContext *
wasm_runtime_get_wasi_ctx(WASMModuleInstanceCommon *module_inst)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode)
        return ((WASMModuleInstance*)module_inst)->wasi_ctx;
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT)
        return ((AOTModuleInstance*)module_inst)->wasi_ctx.ptr;
#endif
    return NULL;
}

void
wasm_runtime_set_wasi_ctx(WASMModuleInstanceCommon *module_inst,
                          WASIContext *wasi_ctx)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode)
        ((WASMModuleInstance*)module_inst)->wasi_ctx = wasi_ctx;
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT)
        ((AOTModuleInstance*)module_inst)->wasi_ctx.ptr = wasi_ctx;
#endif
}
#endif /* end of WASM_ENABLE_LIBC_WASI */

WASMModuleCommon*
wasm_exec_env_get_module(WASMExecEnv *exec_env)
{
    WASMModuleInstanceCommon *module_inst =
        wasm_runtime_get_module_inst(exec_env);
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode)
        return (WASMModuleCommon*)
            ((WASMModuleInstance*)module_inst)->module;
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT)
        return (WASMModuleCommon*)
            ((AOTModuleInstance*)module_inst)->aot_module.ptr;
#endif
    return NULL;
}

/**
 * Implementation of wasm_application_execute_main()
 */

static WASMFunctionInstanceCommon*
resolve_function(const WASMModuleInstanceCommon *module_inst,
                 const char *name);

static bool
check_main_func_type(const WASMType *type)
{
    if (!(type->param_count == 0 || type->param_count == 2)
        ||type->result_count > 1) {
        LOG_ERROR("WASM execute application failed: invalid main function type.\n");
        return false;
    }

    if (type->param_count == 2
        && !(type->types[0] == VALUE_TYPE_I32
        && type->types[1] == VALUE_TYPE_I32)) {
        LOG_ERROR("WASM execute application failed: invalid main function type.\n");
        return false;
    }

    if (type->result_count
        && type->types[type->param_count] != VALUE_TYPE_I32) {
        LOG_ERROR("WASM execute application failed: invalid main function type.\n");
        return false;
    }

    return true;
}

bool
wasm_application_execute_main(WASMModuleInstanceCommon *module_inst,
                              int32 argc, char *argv[])
{
    WASMFunctionInstanceCommon *func;
    WASMType *func_type = NULL;
    uint32 argc1 = 0, argv1[2] = { 0 };
    uint32 total_argv_size = 0;
    uint64 total_size;
    uint32 argv_buf_offset = 0;
    int32 i;
    char *argv_buf, *p, *p_end;
    uint32 *argv_offsets;
    bool ret;

#if WASM_ENABLE_LIBC_WASI != 0
    if (wasm_runtime_is_wasi_mode(module_inst)) {
        /* In wasi mode, we should call function named "_start"
           which initializes the wasi envrionment and then calls
           the actual main function. Directly call main function
           may cause exception thrown. */
        if ((func = wasm_runtime_lookup_wasi_start_function(module_inst)))
            return wasm_runtime_create_exec_env_and_call_wasm(
                                            module_inst, func, 0, NULL);
        /* if no start function is found, we execute
           the main function as normal */
    }
#endif /* end of WASM_ENABLE_LIBC_WASI */

    if (!(func = resolve_function(module_inst, "main"))
        && !(func = resolve_function(module_inst, "__main_argc_argv"))
        && !(func = resolve_function(module_inst, "_main"))) {
        wasm_runtime_set_exception(module_inst,
                                   "lookup main function failed");
        return false;
    }

#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        if (((WASMFunctionInstance*)func)->is_import_func) {
            wasm_runtime_set_exception(module_inst,
                                       "lookup main function failed");
            return false;
        }
        func_type = ((WASMFunctionInstance*)func)->u.func->func_type;
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT)
        func_type = ((AOTFunctionInstance*)func)->u.func.func_type;
#endif

    if (!func_type) {
        LOG_ERROR("invalid module instance type");
        return false;
    }

    if (!check_main_func_type(func_type)) {
        wasm_runtime_set_exception(module_inst,
                                   "invalid function type of main function");
        return false;
    }

    if (func_type->param_count) {
        for (i = 0; i < argc; i++)
            total_argv_size += (uint32)(strlen(argv[i]) + 1);
        total_argv_size = align_uint(total_argv_size, 4);

        total_size = (uint64)total_argv_size + sizeof(int32) * (uint64)argc;

        if (total_size >= UINT32_MAX
            || !(argv_buf_offset =
                    wasm_runtime_module_malloc(module_inst, (uint32)total_size,
                                               (void**)&argv_buf))) {
            wasm_runtime_set_exception(module_inst,
                                       "allocate memory failed");
            return false;
        }

        p = argv_buf;
        argv_offsets = (uint32*)(p + total_argv_size);
        p_end = p + total_size;

        for (i = 0; i < argc; i++) {
            bh_memcpy_s(p, (uint32)(p_end - p), argv[i], (uint32)(strlen(argv[i]) + 1));
            argv_offsets[i] = argv_buf_offset + (uint32)(p - argv_buf);
            p += strlen(argv[i]) + 1;
        }

        argc1 = 2;
        argv1[0] = (uint32)argc;
        argv1[1] = (uint32)wasm_runtime_addr_native_to_app(module_inst, argv_offsets);
    }

    ret = wasm_runtime_create_exec_env_and_call_wasm(module_inst, func,
                                                     argc1, argv1);
    if (argv_buf_offset)
        wasm_runtime_module_free(module_inst, argv_buf_offset);
    return ret;
}

#if WASM_ENABLE_MULTI_MODULE != 0
static WASMModuleInstance *
get_sub_module_inst(const WASMModuleInstance *parent_module_inst,
                    const char *sub_module_name)
{
    WASMSubModInstNode *node =
      bh_list_first_elem(parent_module_inst->sub_module_inst_list);

    while (node && strcmp(node->module_name, sub_module_name)) {
        node = bh_list_elem_next(node);
    }
    return node ? node->module_inst : NULL;
}

static bool
parse_function_name(char *orig_function_name, char **p_module_name,
                    char **p_function_name)
{
    if (orig_function_name[0] != '$') {
        *p_module_name = NULL;
        *p_function_name = orig_function_name;
        return true;
    }

    /**
     * $module_name$function_name\0
     *  ===>
     * module_name\0function_name\0
     *  ===>
     * module_name
     * function_name
     */
    char *p1 = orig_function_name;
    char *p2 = strchr(p1 + 1, '$');
    if (!p2) {
        LOG_DEBUG("can not parse the incoming function name");
        return false;
    }

    *p_module_name = p1 + 1;
    *p2 = '\0';
    *p_function_name = p2 + 1;
    return strlen(*p_module_name) && strlen(*p_function_name);
}
#endif

/**
 * Implementation of wasm_application_execute_func()
 */

static WASMFunctionInstanceCommon*
resolve_function(const WASMModuleInstanceCommon *module_inst,
                 const char *name)
{
    uint32 i = 0;
    WASMFunctionInstanceCommon *ret = NULL;
#if WASM_ENABLE_MULTI_MODULE != 0
    WASMModuleInstance *sub_module_inst = NULL;
    char *orig_name = NULL;
    char *sub_module_name = NULL;
    char *function_name = NULL;
    uint32 length = strlen(name) + 1;

    orig_name = runtime_malloc(sizeof(char) * length, NULL, NULL, 0);
    if (!orig_name) {
        return NULL;
    }

    strncpy(orig_name, name, length);

    if (!parse_function_name(orig_name, &sub_module_name, &function_name)) {
        goto LEAVE;
    }

    LOG_DEBUG("%s -> %s and %s", name, sub_module_name, function_name);

    if (sub_module_name) {
        sub_module_inst = get_sub_module_inst(
          (WASMModuleInstance *)module_inst, sub_module_name);
        if (!sub_module_inst) {
            LOG_DEBUG("can not find a sub module named %s", sub_module_name);
            goto LEAVE;
        }
    }
#else
    const char *function_name = name;
#endif

#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        WASMModuleInstance *wasm_inst = (WASMModuleInstance*)module_inst;

#if WASM_ENABLE_MULTI_MODULE != 0
        wasm_inst = sub_module_inst ? sub_module_inst : wasm_inst;
#endif /* WASM_ENABLE_MULTI_MODULE */

        for (i = 0; i < wasm_inst->export_func_count; i++) {
           if (!strcmp(wasm_inst->export_functions[i].name, function_name)) {
                ret = wasm_inst->export_functions[i].function;
                break;
           }
        }
    }
#endif /* WASM_ENABLE_INTERP */

#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT) {
        AOTModuleInstance *aot_inst = (AOTModuleInstance*)module_inst;
        AOTFunctionInstance *export_funcs = (AOTFunctionInstance *)
                                            aot_inst->export_funcs.ptr;
        for (i = 0; i < aot_inst->export_func_count; i++) {
            if (!strcmp(export_funcs[i].func_name, function_name)) {
                ret = &export_funcs[i];
                break;
            }
        }
    }
#endif

#if WASM_ENABLE_MULTI_MODULE != 0
LEAVE:
    wasm_runtime_free(orig_name);
#endif
    return ret;
}

union ieee754_float {
    float f;

    /* This is the IEEE 754 single-precision format.  */
    union {
        struct {
            unsigned int negative:1;
            unsigned int exponent:8;
            unsigned int mantissa:23;
        } ieee_big_endian;
        struct {
            unsigned int mantissa:23;
            unsigned int exponent:8;
            unsigned int negative:1;
        } ieee_little_endian;
    } ieee;
};

union ieee754_double {
    double d;

    /* This is the IEEE 754 double-precision format.  */
    union {
        struct {
            unsigned int negative:1;
            unsigned int exponent:11;
            /* Together these comprise the mantissa.  */
            unsigned int mantissa0:20;
            unsigned int mantissa1:32;
        } ieee_big_endian;

        struct {
            /* Together these comprise the mantissa.  */
            unsigned int mantissa1:32;
            unsigned int mantissa0:20;
            unsigned int exponent:11;
            unsigned int negative:1;
        } ieee_little_endian;
    } ieee;
};

static union {
    int a;
    char b;
} __ue = { .a = 1 };

#define is_little_endian() (__ue.b == 1)

bool
wasm_application_execute_func(WASMModuleInstanceCommon *module_inst,
                              const char *name, int32 argc, char *argv[])
{
    WASMFunctionInstanceCommon *func;
    WASMType *type = NULL;
    uint32 argc1, *argv1 = NULL, cell_num = 0, j, k = 0;
    int32 i, p;
    uint64 total_size;
    const char *exception;
    char buf[128];

    bh_assert(argc >= 0);
    LOG_DEBUG("call a function \"%s\" with %d arguments", name, argc);
    func = resolve_function(module_inst, name);

    if (!func) {
        snprintf(buf, sizeof(buf), "lookup function %s failed", name);
        wasm_runtime_set_exception(module_inst, buf);
        goto fail;
    }

#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        WASMFunctionInstance *wasm_func = (WASMFunctionInstance*)func;
        if (wasm_func->is_import_func
#if WASM_ENABLE_MULTI_MODULE != 0
            && !wasm_func->import_func_inst
#endif
        ) {
            snprintf(buf, sizeof(buf), "lookup function %s failed", name);
            wasm_runtime_set_exception(module_inst, buf);
            goto fail;
        }
        type = wasm_func->is_import_func ? wasm_func->u.func_import->func_type
                                         : wasm_func->u.func->func_type;
        argc1 = wasm_func->param_cell_num;
        cell_num = argc1 > wasm_func->ret_cell_num ?
                   argc1 : wasm_func->ret_cell_num;
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT) {
        type = ((AOTFunctionInstance*)func)->u.func.func_type;
        argc1 = type->param_cell_num;
        cell_num = argc1 > type->ret_cell_num ?
                   argc1 : type->ret_cell_num;
    }
#endif

    if (!type) {
        LOG_ERROR("invalid module instance type");
        return false;
    }

    if (type->param_count != (uint32)argc) {
        wasm_runtime_set_exception(module_inst,
                                   "invalid input argument count");
        goto fail;
    }

    total_size = sizeof(uint32) * (uint64)(cell_num > 2 ? cell_num : 2);
    if ((!(argv1 = runtime_malloc((uint32)total_size, module_inst,
                                  NULL, 0)))) {
        goto fail;
    }

    /* Parse arguments */
    for (i = 0, p = 0; i < argc; i++) {
        char *endptr = NULL;
        bh_assert(argv[i] != NULL);
        if (argv[i][0] == '\0') {
            snprintf(buf, sizeof(buf), "invalid input argument %d", i);
            wasm_runtime_set_exception(module_inst, buf);
            goto fail;
        }
        switch (type->types[i]) {
            case VALUE_TYPE_I32:
                argv1[p++] = (uint32)strtoul(argv[i], &endptr, 0);
                break;
            case VALUE_TYPE_I64:
            {
                union { uint64 val; uint32 parts[2]; } u;
                u.val = strtoull(argv[i], &endptr, 0);
                argv1[p++] = u.parts[0];
                argv1[p++] = u.parts[1];
                break;
            }
            case VALUE_TYPE_F32:
            {
                float32 f32 = strtof(argv[i], &endptr);
                if (isnan(f32)) {
                    if (argv[i][0] == '-') {
                        union ieee754_float u;
                        u.f = f32;
                        if (is_little_endian())
                            u.ieee.ieee_little_endian.negative = 1;
                        else
                            u.ieee.ieee_big_endian.negative = 1;
                        memcpy(&f32, &u.f, sizeof(float));
                    }
                    if (endptr[0] == ':') {
                        uint32 sig;
                        union ieee754_float u;
                        sig = (uint32)strtoul(endptr + 1, &endptr, 0);
                        u.f = f32;
                        if (is_little_endian())
                            u.ieee.ieee_little_endian.mantissa = sig;
                        else
                            u.ieee.ieee_big_endian.mantissa = sig;
                        memcpy(&f32, &u.f, sizeof(float));
                    }
                }
                memcpy(&argv1[p++], &f32, sizeof(float));
                break;
            }
            case VALUE_TYPE_F64:
            {
                union { float64 val; uint32 parts[2]; } u;
                u.val = strtod(argv[i], &endptr);
                if (isnan(u.val)) {
                    if (argv[i][0] == '-') {
                        union ieee754_double ud;
                        ud.d = u.val;
                        if (is_little_endian())
                            ud.ieee.ieee_little_endian.negative = 1;
                        else
                            ud.ieee.ieee_big_endian.negative = 1;
                        memcpy(&u.val, &ud.d, sizeof(double));
                    }
                    if (endptr[0] == ':') {
                        uint64 sig;
                        union ieee754_double ud;
                        sig = strtoull(endptr + 1, &endptr, 0);
                        ud.d = u.val;
                        if (is_little_endian()) {
                            ud.ieee.ieee_little_endian.mantissa0 = sig >> 32;
                            ud.ieee.ieee_little_endian.mantissa1 = (uint32)sig;
                        }
                        else {
                            ud.ieee.ieee_big_endian.mantissa0 = sig >> 32;
                            ud.ieee.ieee_big_endian.mantissa1 = (uint32)sig;
                        }
                        memcpy(&u.val, &ud.d, sizeof(double));
                    }
                }
                argv1[p++] = u.parts[0];
                argv1[p++] = u.parts[1];
                break;
            }
#if WASM_ENABLE_SIMD != 0
            case VALUE_TYPE_V128:
            {
                /* it likes 0x123\0x234 or 123\234 */
                /* retrive first i64 */
                *(uint64*)(argv1 + p) = strtoull(argv[i], &endptr, 0);
                /* skip \ */
                endptr++;
                /* retrive second i64 */
                *(uint64*)(argv1 + p + 2) = strtoull(endptr, &endptr, 0);
                p += 4;
                break;
            }
#endif /* WASM_ENABLE_SIMD != 0 */
            default:
                bh_assert(0);
                break;
        }
        if (endptr && *endptr != '\0' && *endptr != '_') {
            snprintf(buf, sizeof(buf), "invalid input argument %d: %s",
                     i, argv[i]);
            wasm_runtime_set_exception(module_inst, buf);
            goto fail;
        }
    }
    bh_assert(p == (int32)argc1);

    wasm_runtime_set_exception(module_inst, NULL);
    if (!wasm_runtime_create_exec_env_and_call_wasm(module_inst, func,
                                                    argc1, argv1)) {
        goto fail;
    }

    /* print return value */
    for (j = 0; j < type->result_count; j++) {
        switch (type->types[type->param_count + j]) {
            case VALUE_TYPE_I32:
            {
                os_printf("0x%x:i32", argv1[k]);
                k++;
                break;
            }
            case VALUE_TYPE_I64:
            {
                union { uint64 val; uint32 parts[2]; } u;
                u.parts[0] = argv1[k];
                u.parts[1] = argv1[k + 1];
                k += 2;
#ifdef PRIx64
                os_printf("0x%"PRIx64":i64", u.val);
#else
                char buf[16];
                if (sizeof(long) == 4)
                    snprintf(buf, sizeof(buf), "%s", "0x%llx:i64");
                else
                    snprintf(buf, sizeof(buf), "%s", "0x%lx:i64");
                os_printf(buf, u.val);
#endif
                break;
            }
            case VALUE_TYPE_F32:
                os_printf("%.7g:f32", *(float32*)(argv1 + k));
                k++;
                break;
            case VALUE_TYPE_F64:
            {
                union { float64 val; uint32 parts[2]; } u;
                u.parts[0] = argv1[k];
                u.parts[1] = argv1[k + 1];
                k += 2;
                os_printf("%.7g:f64", u.val);
                break;
            }
#if WASM_ENABLE_SIMD != 0
            case VALUE_TYPE_V128:
            {
                uint64 *v = (uint64*)(argv1 + k);
#if defined(PRIx64)
                os_printf("<0x%016"PRIx64" 0x%016"PRIx64">:v128", *v, *(v + 1));
#else
                if (4 == sizeof(long)) {
                    os_printf("<0x%016llx 0x%016llx>:v128", *v, *(v + 1));
                }
                else {
                    os_printf("<0x%016lx 0x%016lx>:v128", *v, *(v + 1));
                }
#endif /* PRIx64 */
                k += 4;
                break;
            }
#endif /*  WASM_ENABLE_SIMD != 0 */
            default:
                bh_assert(0);
                break;
        }
        if (j < (uint32)(type->result_count - 1))
            os_printf(",");
    }
    os_printf("\n");

    wasm_runtime_free(argv1);
    return true;

fail:
    if (argv1)
        wasm_runtime_free(argv1);

    exception = wasm_runtime_get_exception(module_inst);
    bh_assert(exception);
    os_printf("%s\n", exception);
    return false;
}

bool
wasm_runtime_register_natives(const char *module_name,
                              NativeSymbol *native_symbols,
                              uint32 n_native_symbols)
{
    return wasm_native_register_natives(module_name,
                                        native_symbols, n_native_symbols);
}

bool
wasm_runtime_register_natives_raw(const char *module_name,
                                  NativeSymbol *native_symbols,
                                  uint32 n_native_symbols)
{
    return wasm_native_register_natives_raw(module_name,
                                            native_symbols, n_native_symbols);
}

bool
wasm_runtime_invoke_native_raw(WASMExecEnv *exec_env, void *func_ptr,
                               const WASMType *func_type, const char *signature,
                               void *attachment,
                               uint32 *argv, uint32 argc, uint32 *argv_ret)
{
    WASMModuleInstanceCommon *module = wasm_runtime_get_module_inst(exec_env);
    typedef void (*NativeRawFuncPtr)(WASMExecEnv*, uint64*);
    NativeRawFuncPtr invokeNativeRaw = (NativeRawFuncPtr)func_ptr;
    uint64 argv_buf[16] = { 0 }, *argv1 = argv_buf, *argv_dst, size;
    uint32 *argv_src = argv, i, argc1, ptr_len;
    uint32 arg_i32;
    bool ret = false;

    argc1 = func_type->param_count;
    if (argc1 > sizeof(argv_buf) / sizeof(uint64)) {
        size = sizeof(uint64) * (uint64)argc1;
        if (!(argv1 = runtime_malloc((uint32)size, exec_env->module_inst,
                                     NULL, 0))) {
            return false;
        }
    }

    argv_dst = argv1;

    /* Traverse secondly to fill in each argument */
    for (i = 0; i < func_type->param_count; i++, argv_dst++) {
        switch (func_type->types[i]) {
            case VALUE_TYPE_I32:
            {
                *(uint32*)argv_dst = arg_i32 = *argv_src++;
                if (signature) {
                    if (signature[i + 1] == '*') {
                        /* param is a pointer */
                        if (signature[i + 2] == '~')
                            /* pointer with length followed */
                            ptr_len = *argv_src;
                        else
                            /* pointer without length followed */
                            ptr_len = 1;

                        if (!wasm_runtime_validate_app_addr(module, arg_i32, ptr_len))
                            goto fail;

                        *(uintptr_t*)argv_dst = (uintptr_t)
                                      wasm_runtime_addr_app_to_native(module, arg_i32);
                    }
                    else if (signature[i + 1] == '$') {
                        /* param is a string */
                        if (!wasm_runtime_validate_app_str_addr(module, arg_i32))
                            goto fail;

                        *(uintptr_t*)argv_dst = (uintptr_t)
                                      wasm_runtime_addr_app_to_native(module, arg_i32);
                    }
                }
                break;
            }
            case VALUE_TYPE_I64:
            case VALUE_TYPE_F64:
                bh_memcpy_s(argv_dst, sizeof(uint64), argv_src, sizeof(uint32) * 2);
                argv_src += 2;
                break;
            case VALUE_TYPE_F32:
                *(float32*)argv_dst = *(float32*)argv_src++;
                break;
            default:
                bh_assert(0);
                break;
        }
    }

    exec_env->attachment = attachment;
    invokeNativeRaw(exec_env, argv1);
    exec_env->attachment = NULL;

    if (func_type->result_count > 0) {
        switch (func_type->types[func_type->param_count]) {
            case VALUE_TYPE_I32:
                argv_ret[0] = *(uint32*)argv1;
                break;
            case VALUE_TYPE_F32:
                *(float32*)argv_ret = *(float32*)argv1;
                break;
            case VALUE_TYPE_I64:
            case VALUE_TYPE_F64:
                bh_memcpy_s(argv_ret, sizeof(uint32) * 2, argv1, sizeof(uint64));
                break;
            default:
                bh_assert(0);
                break;
        }
    }

    ret = !wasm_runtime_get_exception(module) ? true : false;

fail:
    if (argv1 != argv_buf)
        wasm_runtime_free(argv1);
     return ret;
}

/**
 * Implementation of wasm_runtime_invoke_native()
 */

#define PUT_I64_TO_ADDR(addr, value) do {       \
    union { int64 val; uint32 parts[2]; } u;    \
    u.val = (value);                            \
    (addr)[0] = u.parts[0];                     \
    (addr)[1] = u.parts[1];                     \
  } while (0)

#define PUT_F64_TO_ADDR(addr, value) do {       \
    union { float64 val; uint32 parts[2]; } u;  \
    u.val = (value);                            \
    (addr)[0] = u.parts[0];                     \
    (addr)[1] = u.parts[1];                     \
  } while (0)

/* The invoke native implementation on ARM platform with VFP co-processor */
#if defined(BUILD_TARGET_ARM_VFP) \
    || defined(BUILD_TARGET_THUMB_VFP) \
    || defined(BUILD_TARGET_RISCV32_ILP32D) \
    || defined(BUILD_TARGET_RISCV32_ILP32)
typedef void (*GenericFunctionPointer)();
int64 invokeNative(GenericFunctionPointer f, uint32 *args, uint32 n_stacks);

typedef float64 (*Float64FuncPtr)(GenericFunctionPointer, uint32*, uint32);
typedef float32 (*Float32FuncPtr)(GenericFunctionPointer, uint32*, uint32);
typedef int64 (*Int64FuncPtr)(GenericFunctionPointer, uint32*,uint32);
typedef int32 (*Int32FuncPtr)(GenericFunctionPointer, uint32*, uint32);
typedef void (*VoidFuncPtr)(GenericFunctionPointer, uint32*, uint32);

static Float64FuncPtr invokeNative_Float64 = (Float64FuncPtr)(uintptr_t)invokeNative;
static Float32FuncPtr invokeNative_Float32 = (Float32FuncPtr)(uintptr_t)invokeNative;
static Int64FuncPtr invokeNative_Int64 = (Int64FuncPtr)(uintptr_t)invokeNative;
static Int32FuncPtr invokeNative_Int32 = (Int32FuncPtr)(uintptr_t)invokeNative;
static VoidFuncPtr invokeNative_Void = (VoidFuncPtr)(uintptr_t)invokeNative;

#if !defined(BUILD_TARGET_RISCV32_ILP32D) \
    && !defined(BUILD_TARGET_RISCV32_ILP32)
#define MAX_REG_INTS   4
#define MAX_REG_FLOATS 16
#else
#define MAX_REG_INTS   8
#define MAX_REG_FLOATS 8
#endif

bool
wasm_runtime_invoke_native(WASMExecEnv *exec_env, void *func_ptr,
                           const WASMType *func_type, const char *signature,
                           void *attachment,
                           uint32 *argv, uint32 argc, uint32 *argv_ret)
{
    WASMModuleInstanceCommon *module = wasm_runtime_get_module_inst(exec_env);
    /* argv buf layout: int args(fix cnt) + float args(fix cnt) + stack args */
    uint32 argv_buf[32], *argv1 = argv_buf, *ints, *stacks, size;
    uint32 *argv_src = argv, i, argc1, n_ints = 0, n_stacks = 0;
    uint32 arg_i32, ptr_len;
    uint32 result_count = func_type->result_count;
    uint32 ext_ret_count = result_count > 1 ? result_count - 1 : 0;
    bool ret = false;
#if !defined(BUILD_TARGET_RISCV32_ILP32)
    uint32 *fps;
    int n_fps = 0;
#else
#define fps ints
#define n_fps n_ints
#endif

    n_ints++; /* exec env */

    /* Traverse firstly to calculate stack args count */
    for (i = 0; i < func_type->param_count; i++) {
        switch (func_type->types[i]) {
            case VALUE_TYPE_I32:
                if (n_ints < MAX_REG_INTS)
                    n_ints++;
                else
                    n_stacks++;
                break;
            case VALUE_TYPE_I64:
                if (n_ints < MAX_REG_INTS - 1) {
#if !defined(BUILD_TARGET_RISCV32_ILP32) && !defined(BUILD_TARGET_RISCV32_ILP32D)
                    /* 64-bit data must be 8 bytes aligned in arm */
                    if (n_ints & 1)
                        n_ints++;
#endif
                    n_ints += 2;
                }
#if defined(BUILD_TARGET_RISCV32_ILP32) || defined(BUILD_TARGET_RISCV32_ILP32D)
                /* part in register, part in stack */
                else if (n_ints == MAX_REG_INTS - 1) {
                    n_ints++;
                    n_stacks++;
                }
#endif
                else {
                    /* 64-bit data in stack must be 8 bytes aligned
                       in arm and riscv32 */
                    if (n_stacks & 1)
                        n_stacks++;
                    n_stacks += 2;
                }
                break;
#if !defined(BUILD_TARGET_RISCV32_ILP32D)
            case VALUE_TYPE_F32:
                if (n_fps < MAX_REG_FLOATS)
                    n_fps++;
                else
                    n_stacks++;
                break;
            case VALUE_TYPE_F64:
                if (n_fps < MAX_REG_FLOATS - 1) {
#if !defined(BUILD_TARGET_RISCV32_ILP32)
                    /* 64-bit data must be 8 bytes aligned in arm */
                    if (n_fps & 1)
                        n_fps++;
#endif
                    n_fps += 2;
                }
#if defined(BUILD_TARGET_RISCV32_ILP32)
                else if (n_fps == MAX_REG_FLOATS - 1) {
                    n_fps++;
                    n_stacks++;
                }
#endif
                else {
                    /* 64-bit data must be 8 bytes aligned in arm */
                    if (n_stacks & 1)
                        n_stacks++;
                    n_stacks += 2;
                }
                break;
#else /* BUILD_TARGET_RISCV32_ILP32D */
            case VALUE_TYPE_F32:
            case VALUE_TYPE_F64:
                if (n_fps < MAX_REG_FLOATS) {
                    n_fps++;
                }
                else if (func_type->types[i] == VALUE_TYPE_F32
                         && n_ints < MAX_REG_INTS) {
                    /* use int reg firstly if available */
                    n_ints++;
                }
                else if (func_type->types[i] == VALUE_TYPE_F64
                         && n_ints < MAX_REG_INTS - 1) {
                    /* use int regs firstly if available */
                    if (n_ints & 1)
                        n_ints++;
                    ints += 2;
                }
                else {
                    /* 64-bit data in stack must be 8 bytes aligned in riscv32 */
                    if (n_stacks & 1)
                        n_stacks++;
                    n_stacks += 2;
                }
                break;
#endif /* BUILD_TARGET_RISCV32_ILP32D */
            default:
                bh_assert(0);
                break;
        }
    }

    for (i = 0; i < ext_ret_count; i++) {
        if (n_ints < MAX_REG_INTS)
            n_ints++;
        else
            n_stacks++;
    }

#if !defined(BUILD_TARGET_RISCV32_ILP32) && !defined(BUILD_TARGET_RISCV32_ILP32D)
    argc1 = MAX_REG_INTS + MAX_REG_FLOATS + n_stacks;
#elif defined(BUILD_TARGET_RISCV32_ILP32)
    argc1 = MAX_REG_INTS + n_stacks;
#else
    argc1 = MAX_REG_INTS + MAX_REG_FLOATS * 2 + n_stacks;
#endif

    if (argc1 > sizeof(argv_buf) / sizeof(uint32)) {
        size = sizeof(uint32) * (uint32)argc1;
        if (!(argv1 = runtime_malloc((uint32)size, exec_env->module_inst,
                                     NULL, 0))) {
            return false;
        }
    }

    ints = argv1;
#if !defined(BUILD_TARGET_RISCV32_ILP32) && !defined(BUILD_TARGET_RISCV32_ILP32D)
    fps = ints + MAX_REG_INTS;
    stacks = fps + MAX_REG_FLOATS;
#elif defined(BUILD_TARGET_RISCV32_ILP32)
    stacks = ints + MAX_REG_INTS;
#else
    fps = ints + MAX_REG_INTS;
    stacks = fps + MAX_REG_FLOATS * 2;
#endif

    n_ints = 0;
    n_fps = 0;
    n_stacks = 0;
    ints[n_ints++] = (uint32)(uintptr_t)exec_env;

    /* Traverse secondly to fill in each argument */
    for (i = 0; i < func_type->param_count; i++) {
        switch (func_type->types[i]) {
            case VALUE_TYPE_I32:
            {
                arg_i32 = *argv_src++;

                if (signature) {
                    if (signature[i + 1] == '*') {
                        /* param is a pointer */
                        if (signature[i + 2] == '~')
                            /* pointer with length followed */
                            ptr_len = *argv_src;
                        else
                            /* pointer without length followed */
                            ptr_len = 1;

                        if (!wasm_runtime_validate_app_addr(module, arg_i32, ptr_len))
                            goto fail;

                        arg_i32 = (uintptr_t)
                                  wasm_runtime_addr_app_to_native(module, arg_i32);
                    }
                    else if (signature[i + 1] == '$') {
                        /* param is a string */
                        if (!wasm_runtime_validate_app_str_addr(module, arg_i32))
                            goto fail;

                        arg_i32 = (uintptr_t)
                                  wasm_runtime_addr_app_to_native(module, arg_i32);
                    }
                }

                if (n_ints < MAX_REG_INTS)
                    ints[n_ints++] = arg_i32;
                else
                    stacks[n_stacks++] = arg_i32;
                break;
            }
            case VALUE_TYPE_I64:
            {
                if (n_ints < MAX_REG_INTS - 1) {
#if !defined(BUILD_TARGET_RISCV32_ILP32) && !defined(BUILD_TARGET_RISCV32_ILP32D)
                    /* 64-bit data must be 8 bytes aligned in arm */
                    if (n_ints & 1)
                        n_ints++;
#endif
                    *(uint64*)&ints[n_ints] = *(uint64*)argv_src;
                    n_ints += 2;
                    argv_src += 2;
                }
#if defined(BUILD_TARGET_RISCV32_ILP32) || defined(BUILD_TARGET_RISCV32_ILP32D)
                else if (n_ints == MAX_REG_INTS - 1) {
                    ints[n_ints++] = *argv_src++;
                    stacks[n_stacks++] = *argv_src++;
                }
#endif
                else {
                    /* 64-bit data in stack must be 8 bytes aligned
                       in arm and riscv32 */
                    if (n_stacks & 1)
                        n_stacks++;
                    *(uint64*)&stacks[n_stacks] = *(uint64*)argv_src;
                    n_stacks += 2;
                    argv_src += 2;
                }
                break;
            }
#if !defined(BUILD_TARGET_RISCV32_ILP32D)
            case VALUE_TYPE_F32:
            {
                if (n_fps < MAX_REG_FLOATS)
                    *(float32*)&fps[n_fps++] = *(float32*)argv_src++;
                else
                    *(float32*)&stacks[n_stacks++] = *(float32*)argv_src++;
                break;
            }
            case VALUE_TYPE_F64:
            {
                if (n_fps < MAX_REG_FLOATS - 1) {
#if !defined(BUILD_TARGET_RISCV32_ILP32)
                    /* 64-bit data must be 8 bytes aligned in arm */
                    if (n_fps & 1)
                        n_fps++;
#endif
                    *(float64*)&fps[n_fps] = *(float64*)argv_src;
                    n_fps += 2;
                    argv_src += 2;
                }
#if defined(BUILD_TARGET_RISCV32_ILP32)
                else if (n_fps == MAX_REG_FLOATS - 1) {
                    fps[n_fps++] = *argv_src++;
                    stacks[n_stacks++] = *argv_src++;
                }
#endif
                else {
                    /* 64-bit data must be 8 bytes aligned in arm */
                    if (n_stacks & 1)
                        n_stacks++;
                    *(float64*)&stacks[n_stacks] = *(float64*)argv_src;
                    n_stacks += 2;
                    argv_src += 2;
                }
                break;
            }
#else /* BUILD_TARGET_RISCV32_ILP32D */
            case VALUE_TYPE_F32:
            case VALUE_TYPE_F64:
            {
                if (n_fps < MAX_REG_FLOATS) {
                    if (func_type->types[i] == VALUE_TYPE_F32) {
                        *(float32*)&fps[n_fps * 2] = *(float32*)argv_src++;
                        /* NaN boxing, the upper bits of a valid NaN-boxed
                          value must be all 1s. */
                        fps[n_fps * 2 + 1] = 0xFFFFFFFF;
                    }
                    else {
                        *(float64*)&fps[n_fps * 2] = *(float64*)argv_src;
                        argv_src += 2;
                    }
                    n_fps++;
                }
                else if (func_type->types[i] == VALUE_TYPE_F32
                         && n_ints < MAX_REG_INTS) {
                    /* use int reg firstly if available */
                    *(float32*)&ints[n_ints++] = *(float32*)argv_src++;
                }
                else if (func_type->types[i] == VALUE_TYPE_F64
                         && n_ints < MAX_REG_INTS - 1) {
                    /* use int regs firstly if available */
                    if (n_ints & 1)
                        n_ints++;
                    *(float64*)&ints[n_ints] = *(float64*)argv_src;
                    n_ints += 2;
                    argv_src += 2;
                }
                else {
                    /* 64-bit data in stack must be 8 bytes aligned in riscv32 */
                    if (n_stacks & 1)
                        n_stacks++;
                    if (func_type->types[i] == VALUE_TYPE_F32) {
                        *(float32*)&stacks[n_stacks] = *(float32*)argv_src++;
                        /* NaN boxing, the upper bits of a valid NaN-boxed
                          value must be all 1s. */
                        stacks[n_stacks + 1] = 0xFFFFFFFF;
                    }
                    else {
                        *(float64*)&stacks[n_stacks] = *(float64*)argv_src;
                        argv_src += 2;
                    }
                    n_stacks += 2;
                }
                break;
            }
#endif /* BUILD_TARGET_RISCV32_ILP32D */
            default:
                bh_assert(0);
                break;
        }
    }

    /* Save extra result values' address to argv1 */
    for (i = 0; i < ext_ret_count; i++) {
        if (n_ints < MAX_REG_INTS)
            ints[n_ints++] = *(uint32*)argv_src++;
        else
            stacks[n_stacks++] = *(uint32*)argv_src++;
    }

    exec_env->attachment = attachment;
    if (func_type->result_count == 0) {
        invokeNative_Void(func_ptr, argv1, n_stacks);
    }
    else {
        switch (func_type->types[func_type->param_count]) {
            case VALUE_TYPE_I32:
                argv_ret[0] = (uint32)invokeNative_Int32(func_ptr, argv1, n_stacks);
                break;
            case VALUE_TYPE_I64:
                PUT_I64_TO_ADDR(argv_ret, invokeNative_Int64(func_ptr, argv1, n_stacks));
                break;
            case VALUE_TYPE_F32:
                *(float32*)argv_ret = invokeNative_Float32(func_ptr, argv1, n_stacks);
                break;
            case VALUE_TYPE_F64:
                PUT_F64_TO_ADDR(argv_ret, invokeNative_Float64(func_ptr, argv1, n_stacks));
                break;
            default:
                bh_assert(0);
                break;
        }
    }
    exec_env->attachment = NULL;

    ret = !wasm_runtime_get_exception(module) ? true : false;

fail:
    if (argv1 != argv_buf)
        wasm_runtime_free(argv1);
    return ret;
}
#endif /* end of defined(BUILD_TARGET_ARM_VFP)
          || defined(BUILD_TARGET_THUMB_VFP) \
          || defined(BUILD_TARGET_RISCV32_ILP32D)
          || defined(BUILD_TARGET_RISCV32_ILP32) */

#if defined(BUILD_TARGET_X86_32) \
    || defined(BUILD_TARGET_ARM) \
    || defined(BUILD_TARGET_THUMB) \
    || defined(BUILD_TARGET_MIPS) \
    || defined(BUILD_TARGET_XTENSA)
typedef void (*GenericFunctionPointer)();
int64 invokeNative(GenericFunctionPointer f, uint32 *args, uint32 sz);

typedef float64 (*Float64FuncPtr)(GenericFunctionPointer f, uint32*, uint32);
typedef float32 (*Float32FuncPtr)(GenericFunctionPointer f, uint32*, uint32);
typedef int64 (*Int64FuncPtr)(GenericFunctionPointer f, uint32*, uint32);
typedef int32 (*Int32FuncPtr)(GenericFunctionPointer f, uint32*, uint32);
typedef void (*VoidFuncPtr)(GenericFunctionPointer f, uint32*, uint32);

static Int64FuncPtr invokeNative_Int64 = (Int64FuncPtr)invokeNative;
static Int32FuncPtr invokeNative_Int32 = (Int32FuncPtr)invokeNative;
static Float64FuncPtr invokeNative_Float64 = (Float64FuncPtr)invokeNative;
static Float32FuncPtr invokeNative_Float32 = (Float32FuncPtr)invokeNative;
static VoidFuncPtr invokeNative_Void = (VoidFuncPtr)invokeNative;

static inline void
word_copy(uint32 *dest, uint32 *src, unsigned num)
{
    for (; num > 0; num--)
        *dest++ = *src++;
}

bool
wasm_runtime_invoke_native(WASMExecEnv *exec_env, void *func_ptr,
                           const WASMType *func_type, const char *signature,
                           void *attachment,
                           uint32 *argv, uint32 argc, uint32 *argv_ret)
{
    WASMModuleInstanceCommon *module = wasm_runtime_get_module_inst(exec_env);
    uint32 argv_buf[32], *argv1 = argv_buf, argc1, i, j = 0;
    uint32 arg_i32, ptr_len;
    uint32 result_count = func_type->result_count;
    uint32 ext_ret_count = result_count > 1 ? result_count - 1 : 0;
    uint64 size;
    bool ret = false;

#if defined(BUILD_TARGET_X86_32)
    argc1 = argc + ext_ret_count + 2;
#else
    /* arm/thumb/mips/xtensa, 64-bit data must be 8 bytes aligned,
       so we need to allocate more memory. */
    argc1 = func_type->param_count * 2 + ext_ret_count + 2;
#endif

    if (argc1 > sizeof(argv_buf) / sizeof(uint32)) {
        size = sizeof(uint32) * (uint64)argc1;
        if (!(argv1 = runtime_malloc((uint32)size, exec_env->module_inst,
                                     NULL, 0))) {
            return false;
        }
    }

    for (i = 0; i < sizeof(WASMExecEnv*) / sizeof(uint32); i++)
        argv1[j++] = ((uint32*)&exec_env)[i];

    for (i = 0; i < func_type->param_count; i++) {
        switch (func_type->types[i]) {
            case VALUE_TYPE_I32:
            {
                arg_i32 = *argv++;

                if (signature) {
                    if (signature[i + 1] == '*') {
                        /* param is a pointer */
                        if (signature[i + 2] == '~')
                            /* pointer with length followed */
                            ptr_len = *argv;
                        else
                            /* pointer without length followed */
                            ptr_len = 1;

                        if (!wasm_runtime_validate_app_addr(module, arg_i32, ptr_len))
                            goto fail;

                        arg_i32 = (uintptr_t)
                                  wasm_runtime_addr_app_to_native(module, arg_i32);
                    }
                    else if (signature[i + 1] == '$') {
                        /* param is a string */
                        if (!wasm_runtime_validate_app_str_addr(module, arg_i32))
                            goto fail;

                        arg_i32 = (uintptr_t)
                                  wasm_runtime_addr_app_to_native(module, arg_i32);
                    }
                }

                argv1[j++] = arg_i32;
                break;
            }
            case VALUE_TYPE_I64:
            case VALUE_TYPE_F64:
#if !defined(BUILD_TARGET_X86_32)
                /* 64-bit data must be 8 bytes aligned in arm, thumb, mips
                   and xtensa */
                if (j & 1)
                    j++;
#endif
                argv1[j++] = *argv++;
                argv1[j++] = *argv++;
                break;
            case VALUE_TYPE_F32:
                argv1[j++] = *argv++;
                break;
            default:
                bh_assert(0);
                break;
        }
    }

    /* Save extra result values' address to argv1 */
    word_copy(argv1 + j, argv, ext_ret_count);

    argc1 = j + ext_ret_count;
    exec_env->attachment = attachment;
    if (func_type->result_count == 0) {
        invokeNative_Void(func_ptr, argv1, argc1);
    }
    else {
        switch (func_type->types[func_type->param_count]) {
            case VALUE_TYPE_I32:
                argv_ret[0] = (uint32)invokeNative_Int32(func_ptr, argv1, argc1);
                break;
            case VALUE_TYPE_I64:
                PUT_I64_TO_ADDR(argv_ret, invokeNative_Int64(func_ptr, argv1, argc1));
                break;
            case VALUE_TYPE_F32:
                *(float32*)argv_ret = invokeNative_Float32(func_ptr, argv1, argc1);
                break;
            case VALUE_TYPE_F64:
                PUT_F64_TO_ADDR(argv_ret, invokeNative_Float64(func_ptr, argv1, argc1));
                break;
            default:
                bh_assert(0);
                break;
        }
    }
    exec_env->attachment = NULL;

    ret = !wasm_runtime_get_exception(module) ? true : false;

fail:
    if (argv1 != argv_buf)
        wasm_runtime_free(argv1);
    return ret;
}

#endif /* end of defined(BUILD_TARGET_X86_32) \
                 || defined(BUILD_TARGET_ARM) \
                 || defined(BUILD_TARGET_THUMB) \
                 || defined(BUILD_TARGET_MIPS) \
                 || defined(BUILD_TARGET_XTENSA) */

#if defined(BUILD_TARGET_X86_64) \
   || defined(BUILD_TARGET_AMD_64) \
   || defined(BUILD_TARGET_AARCH64) \
   || defined(BUILD_TARGET_RISCV64_LP64D) \
   || defined(BUILD_TARGET_RISCV64_LP64)

#if WASM_ENABLE_SIMD != 0
#ifdef v128
#undef v128
#endif

typedef long long v128 __attribute__ ((__vector_size__ (16),
                                       __may_alias__, __aligned__ (1)));

#endif /* end of WASM_ENABLE_SIMD != 0 */

typedef void (*GenericFunctionPointer)();
int64 invokeNative(GenericFunctionPointer f, uint64 *args, uint64 n_stacks);

typedef float64 (*Float64FuncPtr)(GenericFunctionPointer, uint64*, uint64);
typedef float32 (*Float32FuncPtr)(GenericFunctionPointer, uint64*, uint64);
typedef int64 (*Int64FuncPtr)(GenericFunctionPointer, uint64*, uint64);
typedef int32 (*Int32FuncPtr)(GenericFunctionPointer, uint64*, uint64);
typedef void (*VoidFuncPtr)(GenericFunctionPointer, uint64*, uint64);

static Float64FuncPtr invokeNative_Float64 = (Float64FuncPtr)(uintptr_t)invokeNative;
static Float32FuncPtr invokeNative_Float32 = (Float32FuncPtr)(uintptr_t)invokeNative;
static Int64FuncPtr invokeNative_Int64 = (Int64FuncPtr)(uintptr_t)invokeNative;
static Int32FuncPtr invokeNative_Int32 = (Int32FuncPtr)(uintptr_t)invokeNative;
static VoidFuncPtr invokeNative_Void = (VoidFuncPtr)(uintptr_t)invokeNative;

#if WASM_ENABLE_SIMD != 0
typedef v128 (*V128FuncPtr)(GenericFunctionPointer, uint64*, uint64);
static V128FuncPtr invokeNative_V128 = (V128FuncPtr)(uintptr_t)invokeNative;
#endif

#if defined(_WIN32) || defined(_WIN32_)
#define MAX_REG_FLOATS  4
#define MAX_REG_INTS  4
#else /* else of defined(_WIN32) || defined(_WIN32_) */
#define MAX_REG_FLOATS  8
#if defined(BUILD_TARGET_AARCH64) \
    || defined(BUILD_TARGET_RISCV64_LP64D) \
    || defined(BUILD_TARGET_RISCV64_LP64)
#define MAX_REG_INTS  8
#else
#define MAX_REG_INTS  6
#endif /* end of defined(BUILD_TARGET_AARCH64) \
          || defined(BUILD_TARGET_RISCV64_LP64D) \
          || defined(BUILD_TARGET_RISCV64_LP64) */
#endif /* end of defined(_WIN32) || defined(_WIN32_) */

bool
wasm_runtime_invoke_native(WASMExecEnv *exec_env, void *func_ptr,
                           const WASMType *func_type, const char *signature,
                           void *attachment,
                           uint32 *argv, uint32 argc, uint32 *argv_ret)
{
    WASMModuleInstanceCommon *module = wasm_runtime_get_module_inst(exec_env);
    uint64 argv_buf[32], *argv1 = argv_buf, *ints, *stacks, size, arg_i64;
    uint32 *argv_src = argv, i, argc1, n_ints = 0, n_stacks = 0;
    uint32 arg_i32, ptr_len;
    uint32 result_count = func_type->result_count;
    uint32 ext_ret_count = result_count > 1 ? result_count - 1 : 0;
    bool ret = false;
#ifndef BUILD_TARGET_RISCV64_LP64
#if WASM_ENABLE_SIMD == 0
    uint64 *fps;
#else
    v128 *fps;
#endif
#else /* else of BUILD_TARGET_RISCV64_LP64 */
#define fps ints
#endif /* end of BUILD_TARGET_RISCV64_LP64 */

#if defined(_WIN32) || defined(_WIN32_) || defined(BUILD_TARGET_RISCV64_LP64)
    /* important difference in calling conventions */
#define n_fps n_ints
#else
    int n_fps = 0;
#endif

#if WASM_ENABLE_SIMD == 0
    argc1 = 1 + MAX_REG_FLOATS + (uint32)func_type->param_count
              + ext_ret_count;
#else
    argc1 = 1 + MAX_REG_FLOATS * 2 + (uint32)func_type->param_count * 2
              + ext_ret_count;
#endif
    if (argc1 > sizeof(argv_buf) / sizeof(uint64)) {
        size = sizeof(uint64) * (uint64)argc1;
        if (!(argv1 = runtime_malloc((uint32)size, exec_env->module_inst,
                                     NULL, 0))) {
            return false;
        }
    }

#ifndef BUILD_TARGET_RISCV64_LP64
#if WASM_ENABLE_SIMD == 0
    fps = argv1;
    ints = fps + MAX_REG_FLOATS;
#else
    fps = (v128 *)argv1;
    ints = (uint64 *)(fps + MAX_REG_FLOATS);
#endif
#else /* else of BUILD_TARGET_RISCV64_LP64 */
    ints = argv1;
#endif /* end of BUILD_TARGET_RISCV64_LP64 */
    stacks = ints + MAX_REG_INTS;

    ints[n_ints++] = (uint64)(uintptr_t)exec_env;

    for (i = 0; i < func_type->param_count; i++) {
        switch (func_type->types[i]) {
            case VALUE_TYPE_I32:
            {
                arg_i32 = *argv_src++;
                arg_i64 = arg_i32;
                if (signature) {
                    if (signature[i + 1] == '*') {
                        /* param is a pointer */
                        if (signature[i + 2] == '~')
                            /* pointer with length followed */
                            ptr_len = *argv_src;
                        else
                            /* pointer without length followed */
                            ptr_len = 1;

                        if (!wasm_runtime_validate_app_addr(module, arg_i32, ptr_len))
                            goto fail;

                        arg_i64 = (uintptr_t)
                                  wasm_runtime_addr_app_to_native(module, arg_i32);
                    }
                    else if (signature[i + 1] == '$') {
                        /* param is a string */
                        if (!wasm_runtime_validate_app_str_addr(module, arg_i32))
                            goto fail;

                        arg_i64 = (uintptr_t)
                                  wasm_runtime_addr_app_to_native(module, arg_i32);
                    }
                }
                if (n_ints < MAX_REG_INTS)
                    ints[n_ints++] = arg_i64;
                else
                    stacks[n_stacks++] = arg_i64;
                break;
            }
            case VALUE_TYPE_I64:
                if (n_ints < MAX_REG_INTS)
                    ints[n_ints++] = *(uint64*)argv_src;
                else
                    stacks[n_stacks++] = *(uint64*)argv_src;
                argv_src += 2;
                break;
            case VALUE_TYPE_F32:
                if (n_fps < MAX_REG_FLOATS) {
                    *(float32*)&fps[n_fps++] = *(float32*)argv_src++;
                }
                else {
                    *(float32*)&stacks[n_stacks++] = *(float32*)argv_src++;
                }
                break;
            case VALUE_TYPE_F64:
                if (n_fps < MAX_REG_FLOATS) {
                    *(float64*)&fps[n_fps++] = *(float64*)argv_src;
                }
                else {
                    *(float64*)&stacks[n_stacks++] = *(float64*)argv_src;
                }
                argv_src += 2;
                break;
#if WASM_ENABLE_SIMD != 0
            case VALUE_TYPE_V128:
                if (n_fps < MAX_REG_FLOATS) {
                    *(v128*)&fps[n_fps++] = *(v128*)argv_src;
                }
                else {
                    *(v128*)&stacks[n_stacks++] = *(v128*)argv_src;
                    n_stacks++;
                }
                argv_src += 4;
                break;
#endif
            default:
                bh_assert(0);
                break;
        }
    }

    /* Save extra result values' address to argv1 */
    for (i = 0; i < ext_ret_count; i++) {
        if (n_ints < MAX_REG_INTS)
            ints[n_ints++] = *(uint64*)argv_src;
        else
            stacks[n_stacks++] = *(uint64*)argv_src;
        argv_src += 2;
    }

    exec_env->attachment = attachment;
    if (result_count == 0) {
        invokeNative_Void(func_ptr, argv1, n_stacks);
    }
    else {
        /* Invoke the native function and get the first result value */
        switch (func_type->types[func_type->param_count]) {
            case VALUE_TYPE_I32:
                argv_ret[0] = (uint32)invokeNative_Int32(func_ptr, argv1, n_stacks);
                break;
            case VALUE_TYPE_I64:
                PUT_I64_TO_ADDR(argv_ret, invokeNative_Int64(func_ptr, argv1, n_stacks));
                break;
            case VALUE_TYPE_F32:
                *(float32*)argv_ret = invokeNative_Float32(func_ptr, argv1, n_stacks);
                break;
            case VALUE_TYPE_F64:
                PUT_F64_TO_ADDR(argv_ret, invokeNative_Float64(func_ptr, argv1, n_stacks));
                break;
#if WASM_ENABLE_SIMD != 0
            case VALUE_TYPE_V128:
                *(v128*)argv_ret = invokeNative_V128(func_ptr, argv1, n_stacks);
                break;
#endif
            default:
                bh_assert(0);
                break;
        }
    }
    exec_env->attachment = NULL;

    ret = !wasm_runtime_get_exception(module) ? true : false;
fail:
    if (argv1 != argv_buf)
        wasm_runtime_free(argv1);

    return ret;
}

#endif /* end of defined(BUILD_TARGET_X86_64) \
                 || defined(BUILD_TARGET_AMD_64) \
                 || defined(BUILD_TARGET_AARCH64) \
                 || defined(BUILD_TARGET_RISCV64_LP64D) \
                 || defined(BUILD_TARGET_RISCV64_LP64) */

bool
wasm_runtime_call_indirect(WASMExecEnv *exec_env,
                           uint32_t element_indices,
                           uint32_t argc, uint32_t argv[])
{
    if (!wasm_runtime_exec_env_check(exec_env)) {
        LOG_ERROR("Invalid exec env stack info.");
        return false;
    }

    /* this function is called from native code, so exec_env->handle and
       exec_env->native_stack_boundary must have been set, we don't set
       it again */

#if WASM_ENABLE_INTERP != 0
    if (exec_env->module_inst->module_type == Wasm_Module_Bytecode)
        return wasm_call_indirect(exec_env, element_indices, argc, argv);
#endif
#if WASM_ENABLE_AOT != 0
    if (exec_env->module_inst->module_type == Wasm_Module_AoT)
        return aot_call_indirect(exec_env, element_indices, argc, argv);
#endif
    return false;
}

static void
exchange_uint32(uint8 *p_data)
{
    uint8 value = *p_data;
    *p_data = *(p_data + 3);
    *(p_data + 3) = value;

    value = *(p_data + 1);
    *(p_data + 1) = *(p_data + 2);
    *(p_data + 2) = value;
}

static void
exchange_uint64(uint8 *p_data)
{
    uint32 value;

    value = *(uint32 *)p_data;
    *(uint32 *)p_data = *(uint32 *)(p_data + 4);
    *(uint32 *)(p_data + 4) = value;
    exchange_uint32(p_data);
    exchange_uint32(p_data + 4);
}

void
wasm_runtime_read_v128(const uint8 *bytes, uint64 *ret1, uint64 *ret2)
{
    uint64 u1, u2;

    bh_memcpy_s(&u1, 8, bytes, 8);
    bh_memcpy_s(&u2, 8, bytes + 8, 8);

    if (!is_little_endian()) {
        exchange_uint64((uint8*)&u1);
        exchange_uint64((uint8*)&u2);
        *ret1 = u2;
        *ret2 = u1;
    }
    else {
        *ret1 = u1;
        *ret2 = u2;
    }
}

#if WASM_ENABLE_THREAD_MGR != 0
typedef struct WASMThreadArg {
    WASMExecEnv *new_exec_env;
    wasm_thread_callback_t callback;
    void *arg;
} WASMThreadArg;

WASMExecEnv *
wasm_runtime_spawn_exec_env(WASMExecEnv *exec_env)
{
    return wasm_cluster_spawn_exec_env(exec_env);
}

void
wasm_runtime_destroy_spawned_exec_env(WASMExecEnv *exec_env)
{
    wasm_cluster_destroy_spawned_exec_env(exec_env);
}

static void*
wasm_runtime_thread_routine(void *arg)
{
    WASMThreadArg *thread_arg = (WASMThreadArg *)arg;
    void *ret;

    bh_assert(thread_arg->new_exec_env);
    ret = thread_arg->callback(thread_arg->new_exec_env, thread_arg->arg);

    wasm_runtime_destroy_spawned_exec_env(thread_arg->new_exec_env);
    wasm_runtime_free(thread_arg);

    os_thread_exit(ret);
    return ret;
}

int32
wasm_runtime_spawn_thread(WASMExecEnv *exec_env, wasm_thread_t *tid,
                          wasm_thread_callback_t callback, void *arg)
{
    WASMExecEnv *new_exec_env = wasm_runtime_spawn_exec_env(exec_env);
    WASMThreadArg *thread_arg;
    int32 ret;

    if (!new_exec_env)
        return -1;

    if (!(thread_arg = wasm_runtime_malloc(sizeof(WASMThreadArg)))) {
        wasm_runtime_destroy_spawned_exec_env(new_exec_env);
        return -1;
    }

    thread_arg->new_exec_env = new_exec_env;
    thread_arg->callback = callback;
    thread_arg->arg = arg;

    ret = os_thread_create((korp_tid *)tid, wasm_runtime_thread_routine,
                           thread_arg, APP_THREAD_STACK_SIZE_DEFAULT);

    if (ret != 0) {
        wasm_runtime_destroy_spawned_exec_env(new_exec_env);
        wasm_runtime_free(thread_arg);
    }

    return ret;
}

int32
wasm_runtime_join_thread(wasm_thread_t tid, void **retval)
{
    return os_thread_join((korp_tid)tid, retval);
}

#endif

#if WASM_ENABLE_DUMP_CALL_STACK != 0
void
wasm_runtime_dump_call_stack(WASMExecEnv *exec_env)
{
    WASMModuleInstanceCommon *module_inst
        = wasm_exec_env_get_module_inst(exec_env);
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        wasm_interp_dump_call_stack(exec_env);
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT) {
        aot_dump_call_stack(exec_env);
    }
#endif
}
#endif /* end of WASM_ENABLE_DUMP_CALL_STACK */
