/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WASM_C_API_INTERNAL_H
#define _WASM_C_API_INTERNAL_H

#include "wasm_c_api.h"
#include "wasm_runtime_common.h"

#ifndef own
#define own
#endif

/* Vectors */
/* we will malloc resource for the vector's data field */
/* we will release resource of data */
/* caller needs to take care resource for the vector itself */
#define DEFAULT_VECTOR_INIT_LENGTH (64)

WASM_DECLARE_VEC(store, *)
WASM_DECLARE_VEC(module, *)
WASM_DECLARE_VEC(instance, *)

/* Runtime Environment */
typedef enum runtime_mode_e {
    INTERP_MODE = 0,
    JIT_MODE,
    AOT_MODE
} runtime_mode_e;

struct wasm_engine_t {
    /* support one store for now */
    wasm_store_vec_t *stores;
    /* Interpreter by deault */
    runtime_mode_e mode;
};

struct wasm_store_t {
    wasm_module_vec_t *modules;
    wasm_instance_vec_t *instances;
};

/* Type Representations */
struct wasm_valtype_t {
    wasm_valkind_t kind;
};

struct wasm_functype_t {
    uint32 extern_kind;
    /* gona to new and delete own */
    wasm_valtype_vec_t *params;
    wasm_valtype_vec_t *results;
};

struct wasm_globaltype_t {
    uint32 extern_kind;
    /* gona to new and delete own */
    wasm_valtype_t *val_type;
    wasm_mutability_t mutability;
};

struct wasm_tabletype_t {
    uint32 extern_kind;
    /* always be WASM_FUNCREF */
    wasm_valtype_t *type;
    wasm_limits_t *limits;
};

struct wasm_memorytype_t {
    uint32 extern_kind;
    wasm_limits_t *limits;
};

struct wasm_externtype_t {
    uint32 extern_kind;
    uint8 data[1];
};

struct wasm_import_type_t {
    uint32 extern_kind;
    wasm_name_t *module_name;
    wasm_name_t *name;
};

struct wasm_export_type_t {
    uint32 extern_kind;
    wasm_name_t *module_name;
    wasm_name_t *name;
};

/* Runtime Objects */
struct wasm_ref_t {
    uint32 obj;
};

struct wasm_trap_t {
    wasm_byte_vec_t *message;
};

struct wasm_func_t {
    wasm_name_t *module_name;
    wasm_name_t *name;
    uint16 kind;

    wasm_functype_t *func_type;

    bool with_env;
    union {
        wasm_func_callback_t cb;
        struct callback_ext {
            void *env;
            wasm_func_callback_with_env_t cb;
            void (*finalizer)(void *);
        } cb_env;
    } u;
    /*
     * an index in both functions runtime instance lists
     * of interpreter mode and aot mode
     */
    uint16 func_idx_rt;
    WASMModuleInstanceCommon *inst_comm_rt;
};

struct wasm_global_t {
    wasm_name_t *module_name;
    wasm_name_t *name;
    uint16 kind;

    wasm_globaltype_t *type;
    wasm_val_t *init;
    /*
     * an index in both global runtime instance lists
     * of interpreter mode and aot mode
     */
    uint16 global_idx_rt;
    WASMModuleInstanceCommon *inst_comm_rt;
};

struct wasm_memory_t {
    wasm_name_t *module_name;
    wasm_name_t *name;
    uint16 kind;

    wasm_memorytype_t *type;
    /*
     * an index in both memory runtime instance lists
     * of interpreter mode and aot mode
     */
    uint16 memory_idx_rt;
    WASMModuleInstanceCommon *inst_comm_rt;
};

struct wasm_table_t {
    wasm_name_t *module_name;
    wasm_name_t *name;
    uint16 kind;

    wasm_tabletype_t *type;
    /*
     * an index in both table runtime instance lists
     * of interpreter mode and aot mode
     */
    uint16 table_idx_rt;
    WASMModuleInstanceCommon *inst_comm_rt;
};

struct wasm_extern_t {
    wasm_name_t *module_name;
    wasm_name_t *name;
    uint16 kind;
    uint8 data[1];
};

struct wasm_instance_t {
    wasm_extern_vec_t *imports;
    wasm_extern_vec_t *exports;
    WASMModuleInstanceCommon *inst_comm_rt;
};

#endif /* _WASM_C_API_INTERNAL_H */
