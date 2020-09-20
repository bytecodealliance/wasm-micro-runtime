/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_c_api_internal.h"
#include "wasm_memory.h"
#if WASM_ENABLE_INTERP != 0
#include "wasm_runtime.h"
#endif
#if WASM_ENABLE_AOT != 0
#include "aot_runtime.h"
#endif

#define NOT_REACHED() bh_assert(!"should not be reached")

typedef struct wasm_module_ex_t wasm_module_ex_t;

void
wasm_module_delete_internal(wasm_module_t *);

void
wasm_instance_delete_internal(wasm_instance_t *);

static void *
malloc_internal(size_t size)
{
    void *mem = NULL;

    if (size >= UINT32_MAX) {
        return NULL;
    }

    mem = wasm_runtime_malloc((uint32)size);
    if (mem) {
        memset(mem, 0, size);
    }

    return mem;
}

#define FREEIF(p)                                                             \
    if (p) {                                                                  \
        wasm_runtime_free(p);                                                 \
    }

/* Vectors */
#define INIT_VEC(vector_p, func_prefix, size)                                 \
    do {                                                                      \
        vector_p = malloc_internal(sizeof(*(vector_p)));                      \
        if (!vector_p) {                                                      \
            goto failed;                                                      \
        }                                                                     \
        func_prefix##_new_uninitialized(vector_p, size);                      \
        if (!(vector_p)->data) {                                              \
            goto failed;                                                      \
        }                                                                     \
    } while (false)

#define DEINIT_VEC(vector_p, delete_func)                                     \
    if ((vector_p)) {                                                         \
        if ((vector_p)->data) {                                               \
            delete_func(vector_p);                                            \
        }                                                                     \
        wasm_runtime_free(vector_p);                                          \
        vector_p = NULL;                                                      \
    }

#define FREE_VEC_ELEMS(vec, del_func)                                         \
    for (i = 0; i < (vec)->num_elems; ++i) {                                  \
        del_func(*((vec)->data + i));                                         \
        *((vec)->data + i) = NULL;                                            \
    }

static inline void
generic_vec_init_data(Vector *out, size_t num_of_elems, size_t size_of_elem)
{
    if (!bh_vector_init(out, num_of_elems, size_of_elem)) {
        out->data = NULL;
        out->max_elems = 0;
        out->num_elems = 0;
    }
    else {
        memset(out->data, 0, num_of_elems * size_of_elem);
    }
}

void
wasm_byte_vec_new_uninitialized(wasm_byte_vec_t *out, size_t size)
{
    bh_assert(out);
    generic_vec_init_data((Vector *)out, size, sizeof(wasm_byte_t));
}

void
wasm_byte_vec_copy(wasm_byte_vec_t *out, const wasm_byte_vec_t *src)
{
    uint32 len = 0;

    bh_assert(out && src);

    generic_vec_init_data((Vector *)out, src->size, src->size_of_elem);
    if (!out->data) {
        goto failed;
    }

    len = src->size * src->size_of_elem;
    bh_memcpy_s(out->data, len, src->data, len);
    out->num_elems = src->num_elems;
    return;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_byte_vec_delete(out);
}

void
wasm_byte_vec_new(wasm_byte_vec_t *out, size_t size, const wasm_byte_t *data)
{
    size_t size_in_bytes = 0;

    bh_assert(out && data);

    generic_vec_init_data((Vector *)out, size, sizeof(wasm_byte_t));
    if (!out->data) {
        goto failed;
    }

    size_in_bytes = size * sizeof(wasm_byte_t);
    bh_memcpy_s(out->data, size_in_bytes, data, size_in_bytes);
    out->num_elems = size;
    return;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_byte_vec_delete(out);
}

void
wasm_byte_vec_delete(wasm_byte_vec_t *byte_vec)
{
    if (byte_vec && byte_vec->data) {
        bh_vector_destroy((Vector *)byte_vec);
    }
}

/* Runtime Environment */
static void
wasm_engine_delete_internal(wasm_engine_t *engine)
{
    if (engine) {
        DEINIT_VEC(engine->stores, wasm_store_vec_delete);
        wasm_runtime_free(engine);
    }

    wasm_runtime_destroy();
}

static wasm_engine_t *
wasm_engine_new_internal(mem_alloc_type_t type,
                         const MemAllocOption *opts,
                         runtime_mode_e mode)
{
    wasm_engine_t *engine = NULL;
    /* init runtime */
    RuntimeInitArgs init_args = { 0 };
    init_args.mem_alloc_type = type;

    if (type == Alloc_With_Pool) {
        init_args.mem_alloc_option.pool.heap_buf = opts->pool.heap_buf;
        init_args.mem_alloc_option.pool.heap_size = opts->pool.heap_size;
    }
    else if (type == Alloc_With_Allocator) {
        init_args.mem_alloc_option.allocator.malloc_func =
          opts->allocator.malloc_func;
        init_args.mem_alloc_option.allocator.free_func =
          opts->allocator.free_func;
        init_args.mem_alloc_option.allocator.realloc_func =
          opts->allocator.realloc_func;
    }
    else {
        init_args.mem_alloc_option.pool.heap_buf = NULL;
        init_args.mem_alloc_option.pool.heap_size = 0;
    }

    if (!wasm_runtime_full_init(&init_args)) {
        goto failed;
    }

#if BH_DEBUG == 1
    bh_log_set_verbose_level(5);
#else
    bh_log_set_verbose_level(3);
#endif

    /* create wasm_engine_t */
    engine = malloc_internal(sizeof(wasm_engine_t));
    if (!engine) {
        goto failed;
    }

    /* set running mode */
    LOG_WARNING("running under mode %d", mode);
    engine->mode = mode;

    /* create wasm_store_vec_t */
    INIT_VEC(engine->stores, wasm_store_vec, 1);

    return engine;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_engine_delete_internal(engine);
    return NULL;
}

/* global engine instance */
static wasm_engine_t *singleton_engine = NULL;

static inline runtime_mode_e
current_runtime_mode()
{
    bh_assert(singleton_engine);
    return singleton_engine->mode;
}

wasm_engine_t *
wasm_engine_new()
{
    runtime_mode_e mode = INTERP_MODE;
#if WASM_ENABLE_INTERP == 0 && WASM_ENABLE_AOT != 0
    mode = AOT_MODE;
#endif

    if (INTERP_MODE == mode) {
#if WASM_ENABLE_INTERP == 0
        bh_assert(!"does not support INTERP_MODE. Please recompile");
#endif
    }
    else {
#if WASM_ENABLE_AOT == 0
        bh_assert(!"does not support AOT_MODE. Please recompile");
#endif
    }

    if (!singleton_engine) {
        singleton_engine =
          wasm_engine_new_internal(Alloc_With_System_Allocator, NULL, mode);
    }
    return singleton_engine;
}

wasm_engine_t *
wasm_engine_new_with_args(mem_alloc_type_t type,
                          const MemAllocOption *opts,
                          runtime_mode_e mode)
{
    if (!singleton_engine) {
        singleton_engine = wasm_engine_new_internal(type, opts, mode);
    }
    return singleton_engine;
}

void
wasm_engine_delete(wasm_engine_t *engine)
{
    if (engine) {
        wasm_engine_delete_internal(engine);
        singleton_engine = NULL;
    }
}

wasm_store_t *
wasm_store_new(wasm_engine_t *engine)
{
    wasm_store_t *store = NULL;

    bh_assert(engine && singleton_engine == engine);

    /* always return the first store */
    if (bh_vector_size((Vector *)singleton_engine->stores) > 0) {
        /*
         * although it is a copy of the first store, but still point to
         * the wasm_store_t
         */
        if (!bh_vector_get((Vector *)singleton_engine->stores, 0, &store)) {
            goto failed;
        }
        return store;
    }

    store = malloc_internal(sizeof(wasm_store_t));
    if (!store) {
        goto failed;
    }

    /* new a vector, and new its data */
    INIT_VEC(store->modules, wasm_module_vec, DEFAULT_VECTOR_INIT_LENGTH);
    INIT_VEC(store->instances, wasm_instance_vec, DEFAULT_VECTOR_INIT_LENGTH);

    /* append to a store list of engine */
    if (!bh_vector_append((Vector *)singleton_engine->stores, &store)) {
        goto failed;
    }

    return store;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_store_delete(store);
    return NULL;
}

void
wasm_store_delete(wasm_store_t *store)
{
    if (!store || singleton_engine->stores->num_elems == 0) {
        return;
    }

    DEINIT_VEC(store->modules, wasm_module_vec_delete);
    DEINIT_VEC(store->instances, wasm_instance_vec_delete);

    wasm_runtime_free(store);
    bh_vector_remove((Vector *)singleton_engine->stores, 0, NULL);
}

void
wasm_store_vec_new_uninitialized(wasm_store_vec_t *out, size_t size)
{
    bh_assert(out);
    generic_vec_init_data((Vector *)out, size, sizeof(wasm_store_t *));
}

void
wasm_store_vec_delete(wasm_store_vec_t *store_vec)
{
    size_t i = 0;
    if (!store_vec || !store_vec->data) {
        return;
    }

    FREE_VEC_ELEMS(store_vec, wasm_store_delete);
    bh_vector_destroy((Vector *)store_vec);
}

static inline void
check_engine_and_store(wasm_engine_t *engine, wasm_store_t *store)
{
    /* remove it if we are supporting more than one store */
    bh_assert(engine && store && engine->stores->data[0] == store);
}

/* Type Representations */
static wasm_valtype_t *
wasm_valtype_new_internal(uint8 val_type_rt)
{
    switch (val_type_rt) {
        case VALUE_TYPE_I32:
            return wasm_valtype_new_i32();
        case VALUE_TYPE_I64:
            return wasm_valtype_new_i64();
        case VALUE_TYPE_F32:
            return wasm_valtype_new_f32();
        case VALUE_TYPE_F64:
            return wasm_valtype_new_f64();
        case VALUE_TYPE_ANY:
            return wasm_valtype_new_anyref();
        default:
            return NULL;
    }
}

wasm_valtype_t *
wasm_valtype_new(wasm_valkind_t kind)
{
    wasm_valtype_t *val_type = malloc_internal(sizeof(wasm_valtype_t));
    if (val_type) {
        val_type->kind = kind;
    }
    return val_type;
}

void
wasm_valtype_delete(wasm_valtype_t *val_type)
{
    bh_assert(val_type);
    wasm_runtime_free(val_type);
}

wasm_valtype_t *
wasm_valtype_copy(wasm_valtype_t *src)
{
    bh_assert(src);
    return wasm_valtype_new(src->kind);
}

wasm_valkind_t
wasm_valtype_kind(const wasm_valtype_t *val_type)
{
    bh_assert(val_type);
    return val_type->kind;
}

bool
wasm_valtype_same(const wasm_valtype_t *vt1, const wasm_valtype_t *vt2)
{
    if (!vt1 && !vt2) {
        return true;
    }

    if (!vt1 || !vt2) {
        return false;
    }

    return vt1->kind == vt2->kind;
}

void
wasm_valtype_vec_new_uninitialized(wasm_valtype_vec_t *out, size_t size)
{
    bh_assert(out);
    generic_vec_init_data((Vector *)out, size, sizeof(wasm_valtype_t *));
}

void
wasm_valtype_vec_new_empty(wasm_valtype_vec_t *out)
{
    bh_assert(out);
    memset(out, 0, sizeof(wasm_valtype_vec_t));
}

void
wasm_valtype_vec_new(wasm_valtype_vec_t *out,
                     size_t size,
                     wasm_valtype_t *const data[])
{
    size_t size_in_bytes = 0;
    bh_assert(out && data);
    generic_vec_init_data((Vector *)out, size, sizeof(wasm_valtype_t *));
    if (!out->data) {
        goto failed;
    }

    size_in_bytes = size * sizeof(wasm_valtype_t *);
    bh_memcpy_s(out->data, size_in_bytes, data, size_in_bytes);
    out->num_elems = size;
    return;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_valtype_vec_delete(out);
}

void
wasm_valtype_vec_copy(wasm_valtype_vec_t *out, const wasm_valtype_vec_t *src)
{
    size_t i = 0;

    bh_assert(out && src);

    generic_vec_init_data((Vector *)out, src->size, src->size_of_elem);
    if (!out->data) {
        goto failed;
    }

    /* clone every wasm_valtype_t */
    for (i = 0; i < src->num_elems; i++) {
        wasm_valtype_t *cloned = wasm_valtype_copy(*(src->data + i));
        if (!cloned) {
            goto failed;
        }
        if (!bh_vector_append((Vector *)out, &cloned)) {
            goto failed;
        }
    }

    return;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_valtype_vec_delete(out);
}

void
wasm_valtype_vec_delete(wasm_valtype_vec_t *val_type_vec)
{
    size_t i = 0;
    if (!val_type_vec || !val_type_vec->data) {
        return;
    }

    FREE_VEC_ELEMS(val_type_vec, wasm_valtype_delete);
    bh_vector_destroy((Vector *)val_type_vec);
}

static wasm_functype_t *
wasm_functype_new_internal(WASMType *type_rt)
{
    wasm_functype_t *func_type = NULL;
    uint32 i = 0;

    bh_assert(type_rt);

    func_type = malloc_internal(sizeof(wasm_functype_t));
    if (!func_type) {
        goto failed;
    }

    /* WASMType->types[0 : type_rt->param_count) -> func_type->params */
    INIT_VEC(func_type->params, wasm_valtype_vec, type_rt->param_count);
    for (i = 0; i < type_rt->param_count; ++i) {
        wasm_valtype_t *param_type =
          wasm_valtype_new_internal(*(type_rt->types + i));
        if (!param_type) {
            goto failed;
        }

        if (!bh_vector_append((Vector *)func_type->params, &param_type)) {
            goto failed;
        }
    }

    /* WASMType->types[type_rt->param_count : type_rt->result_count) -> func_type->results */
    INIT_VEC(func_type->results, wasm_valtype_vec, type_rt->result_count);
    for (i = type_rt->param_count; i < type_rt->result_count; ++i) {
        wasm_valtype_t *result_type =
          wasm_valtype_new_internal(*(type_rt->types + i));
        if (!result_type) {
            goto failed;
        }

        if (!bh_vector_append((Vector *)func_type->results, &result_type)) {
            goto failed;
        }
    }

    return func_type;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_functype_delete(func_type);
    return NULL;
}

wasm_functype_t *
wasm_functype_new(wasm_valtype_vec_t *params, wasm_valtype_vec_t *results)
{
    wasm_functype_t *func_type = NULL;

    bh_assert(params);
    bh_assert(results);

    func_type = malloc_internal(sizeof(wasm_functype_t));
    if (!func_type) {
        goto failed;
    }

    func_type->extern_kind = WASM_EXTERN_FUNC;

    func_type->params = malloc_internal(sizeof(wasm_valtype_vec_t));
    if (!func_type->params) {
        goto failed;
    }

    /* transfer ownership of contents of params and results */
    bh_memcpy_s(func_type->params, sizeof(wasm_valtype_vec_t), params,
                sizeof(wasm_valtype_vec_t));

    func_type->results = malloc_internal(sizeof(wasm_valtype_vec_t));
    if (!func_type->results) {
        goto failed;
    }

    bh_memcpy_s(func_type->results, sizeof(wasm_valtype_vec_t), results,
                sizeof(wasm_valtype_vec_t));

    return func_type;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    FREEIF(func_type->params);
    FREEIF(func_type->results);
    FREEIF(func_type);
    return NULL;
}

wasm_functype_t *
wasm_functype_copy(wasm_functype_t *src)
{
    wasm_functype_t *dst = NULL;

    bh_assert(src);

    dst = malloc_internal(sizeof(wasm_functype_t));
    if (!dst) {
        goto failed;
    }

    dst->extern_kind = src->extern_kind;

    dst->params = malloc_internal(sizeof(wasm_valtype_vec_t));
    if (!dst->params) {
        goto failed;
    }

    wasm_valtype_vec_copy(dst->params, src->params);
    if (!dst->params) {
        goto failed;
    }

    dst->results = malloc_internal(sizeof(wasm_valtype_vec_t));
    if (!dst->results) {
        goto failed;
    }

    wasm_valtype_vec_copy(dst->results, src->results);
    if (!dst->results) {
        goto failed;
    }

    return dst;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_functype_delete(dst);
    return NULL;
}

void
wasm_functype_delete(wasm_functype_t *func_type)
{
    if (!func_type) {
        return;
    }

    if (func_type->params) {
        wasm_valtype_vec_delete(func_type->params);
        wasm_runtime_free(func_type->params);
        func_type->params = NULL;
    }

    if (func_type->results) {
        wasm_valtype_vec_delete(func_type->results);
        wasm_runtime_free(func_type->results);
        func_type->results = NULL;
    }

    wasm_runtime_free(func_type);
}

const wasm_valtype_vec_t *
wasm_functype_params(const wasm_functype_t *func_type)
{
    bh_assert(func_type);
    return func_type->params;
}

const wasm_valtype_vec_t *
wasm_functype_results(const wasm_functype_t *func_type)
{
    bh_assert(func_type);
    return func_type->results;
}

wasm_globaltype_t *
wasm_globaltype_new(wasm_valtype_t *val_type, wasm_mutability_t mut)
{
    wasm_globaltype_t *global_type = NULL;

    bh_assert(val_type);

    global_type = malloc_internal(sizeof(wasm_globaltype_t));
    if (!global_type) {
        goto failed;
    }

    global_type->val_type = val_type;
    global_type->mutability = mut;

    return global_type;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_globaltype_delete(global_type);
    return NULL;
}

wasm_globaltype_t *
wasm_globaltype_new_internal(uint8 val_type_rt, bool is_mutable)
{
    wasm_globaltype_t *global_type = NULL;

    global_type = malloc_internal(sizeof(wasm_globaltype_t));
    if (!global_type) {
        goto failed;
    }

    global_type->val_type = wasm_valtype_new_internal(val_type_rt);
    if (!global_type->val_type) {
        goto failed;
    }

    global_type->mutability = is_mutable ? WASM_VAR : WASM_CONST;

    return global_type;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_globaltype_delete(global_type);
    return NULL;
}

void
wasm_globaltype_delete(wasm_globaltype_t *global_type)
{
    if (!global_type) {
        return;
    }

    if (global_type->val_type) {
        wasm_valtype_delete(global_type->val_type);
        global_type->val_type = NULL;
    }

    wasm_runtime_free(global_type);
}

wasm_globaltype_t *
wasm_globaltype_copy(wasm_globaltype_t *src)
{
    wasm_globaltype_t *dst = NULL;

    bh_assert(src);

    dst = malloc_internal(sizeof(wasm_globaltype_t));
    if (!dst) {
        goto failed;
    }

    dst->val_type = wasm_valtype_copy(src->val_type);
    if (!dst->val_type) {
        goto failed;
    }

    dst->mutability = src->mutability;

    return dst;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_globaltype_delete(dst);
    return NULL;
}

const wasm_valtype_t *
wasm_globaltype_content(const wasm_globaltype_t *global_type)
{
    bh_assert(global_type);
    return global_type->val_type;
}

wasm_mutability_t
wasm_globaltype_mutability(const wasm_globaltype_t *global_type)
{
    bh_assert(global_type);
    return global_type->mutability;
}

bool
wasm_globaltype_same(const wasm_globaltype_t *gt1,
                     const wasm_globaltype_t *gt2)
{
    if (!gt1 && !gt2) {
        return true;
    }

    if (!gt1 || !gt2) {
        return false;
    }

    return wasm_valtype_same(gt1->val_type, gt2->val_type)
           || gt1->mutability == gt2->mutability;
}

wasm_tabletype_t *
wasm_tabletype_new(wasm_valtype_t *val_type, const wasm_limits_t *limits)
{
    return NULL;
}

void
wasm_tabletype_delete(wasm_tabletype_t *table_type)
{}

wasm_memorytype_t *
wasm_memorytype_new(const wasm_limits_t *limits)
{
    return NULL;
}

void
wasm_memorytype_delete(wasm_memorytype_t *memory_type)
{}

/* Runtime Objects */

void
wasm_val_delete(wasm_val_t *v)
{
    /* do nothing */
}

void
wasm_val_copy(wasm_val_t *out, const wasm_val_t *src)
{
    bh_assert(out && src);
    bh_memcpy_s(out, sizeof(wasm_val_t), src, sizeof(wasm_val_t));
}

bool
wasm_val_same(const wasm_val_t *v1, const wasm_val_t *v2)
{
    if (!v1 && !v2) {
        return true;
    }

    if (!v1 || !v2) {
        return false;
    }

    if (v1->kind != v2->kind) {
        return false;
    }

    switch (v1->kind) {
        case WASM_I32:
            return v1->of.i32 == v2->of.i32;
        case WASM_I64:
            return v1->of.i64 == v2->of.i64;
        case WASM_F32:
            return v1->of.f32 == v2->of.f32;
        case WASM_F64:
            return v1->of.f64 == v2->of.f64;
        case WASM_FUNCREF:
            return v1->of.ref == v2->of.ref;
        default:
            break;
    }
    return false;
}

wasm_trap_t *
wasm_trap_new(wasm_store_t *store, const wasm_message_t *message)
{
    wasm_trap_t *trap;

    bh_assert(store && message);

    trap = malloc_internal(sizeof(wasm_trap_t));
    if (!trap) {
        goto failed;
    }

    wasm_byte_vec_new(trap->message, message->num_elems, message->data);
    if (!trap->message->data) {
        goto failed;
    }

    return trap;

failed:
    wasm_trap_delete(trap);
    return NULL;
}

void
wasm_trap_delete(wasm_trap_t *trap)
{
    if (!trap) {
        return;
    }

    if (trap->message) {
        wasm_byte_vec_delete(trap->message);
    }

    wasm_runtime_free(trap);
}

void
wasm_trap_message(const wasm_trap_t *trap, wasm_message_t *out)
{
    bh_assert(trap && out);
    wasm_byte_vec_copy(out, trap->message);
}

struct wasm_module_ex_t {
    struct WASMModuleCommon *module_comm_rt;
    wasm_byte_vec_t *binary;
};

static inline wasm_module_t *
module_ext_to_module(wasm_module_ex_t *module_ex)
{
    return (wasm_module_t *)module_ex;
}

static inline wasm_module_ex_t *
module_to_module_ext(wasm_module_t *module)
{
    return (wasm_module_ex_t *)module;
}

wasm_module_t *
wasm_module_new(wasm_store_t *store, const wasm_byte_vec_t *binary)
{
    char error[128] = { 0 };
    wasm_module_ex_t *module_ex = NULL;
    PackageType pkg_type = Package_Type_Unknown;

    check_engine_and_store(singleton_engine, store);
    bh_assert(binary && binary->data && binary->size);

    pkg_type = get_package_type((uint8 *)binary->data, binary->size);
    if (Package_Type_Unknown == pkg_type
        || (Wasm_Module_Bytecode == pkg_type
            && INTERP_MODE != current_runtime_mode())
        || (Wasm_Module_AoT == pkg_type
            && INTERP_MODE == current_runtime_mode())) {
        LOG_WARNING(
          "current runtime mode %d doesn\'t support the package type "
          "%d",
          current_runtime_mode(), pkg_type);
        goto failed;
    }

    module_ex = malloc_internal(sizeof(wasm_module_ex_t));
    if (!module_ex) {
        goto failed;
    }

    module_ex->binary = malloc_internal(sizeof(wasm_byte_vec_t));
    if (!module_ex->binary) {
        goto failed;
    }

    wasm_byte_vec_copy(module_ex->binary, binary);
    if (!module_ex->binary->data) {
        goto failed;
    }

    module_ex->module_comm_rt =
      wasm_runtime_load((uint8 *)module_ex->binary->data,
                        module_ex->binary->size, error, (uint32)sizeof(error));
    if (!(module_ex->module_comm_rt)) {
        LOG_ERROR(error);
        goto failed;
    }

    /* add it to a watching list in store */
    if (!bh_vector_append((Vector *)store->modules, &module_ex)) {
        goto failed;
    }

    return module_ext_to_module(module_ex);

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_module_delete_internal(module_ext_to_module(module_ex));
    return NULL;
}

void
wasm_module_delete_internal(wasm_module_t *module)
{
    wasm_module_ex_t *module_ex;
    if (!module) {
        return;
    }

    module_ex = module_to_module_ext(module);
    if (module_ex->binary) {
        wasm_byte_vec_delete(module_ex->binary);
        wasm_runtime_free(module_ex->binary);
        module_ex->binary = NULL;
    }

    if (module_ex->module_comm_rt) {
        wasm_runtime_unload(module_ex->module_comm_rt);
        module_ex->module_comm_rt = NULL;
    }

    wasm_runtime_free(module_ex);
}

/* will release module when releasing the store */
void
wasm_module_delete(wasm_module_t *module)
{
    /* pass */
}

void
wasm_module_vec_new_uninitialized(wasm_module_vec_t *out, size_t size)
{
    generic_vec_init_data((Vector *)out, size, sizeof(wasm_module_t *));
}

void
wasm_module_vec_delete(wasm_module_vec_t *module_vec)
{
    size_t i = 0;
    if (!module_vec || !module_vec->data) {
        return;
    }

    FREE_VEC_ELEMS(module_vec, wasm_module_delete_internal);
    bh_vector_destroy((Vector *)module_vec);
}

static uint32
argv_to_params(const uint64 *argv,
               const wasm_valtype_vec_t *param_defs,
               wasm_val_t *out)
{
    size_t i = 0;
    uint32 argc = 0;
    void *argv_p = (void *)argv;

    for (i = 0; i < param_defs->num_elems; i++) {
        wasm_valtype_t *param_def = param_defs->data[i];
        wasm_val_t *param = out + i;
        switch (param_def->kind) {
            case WASM_I32:
                param->kind = WASM_I32;
                param->of.i32 = *(int32 *)argv_p;
                argv_p = (uint32 *)argv_p + 1;
                argc++;
                break;
            case WASM_I64:
                param->kind = WASM_I64;
                param->of.i64 = *(int64 *)argv_p;
                argv_p = (uint64 *)argv_p + 1;
                argc++;
                break;
            case WASM_F32:
                param->kind = WASM_F32;
                param->of.f32 = *(float32 *)argv_p;
                argv_p = (float32 *)argv_p + 1;
                argc++;
                break;
            case WASM_F64:
                param->kind = WASM_F64;
                param->of.f64 = *(float64 *)argv_p;
                argv_p = (float64 *)argv_p + 1;
                argc++;
                break;
            default:
                NOT_REACHED();
                goto failed;
        }
    }

    return argc;
failed:
    return 0;
}

static uint32
results_to_argv(const wasm_val_t *results,
                const wasm_valtype_vec_t *result_defs,
                uint64 *out)
{
    size_t i = 0;
    uint32 argc = 0;
    void *argv_p = out;

    for (i = 0; i < result_defs->num_elems; ++i) {
        wasm_valtype_t *result_def = result_defs->data[i];
        const wasm_val_t *result = results + i;
        switch (result_def->kind) {
            case WASM_I32:
                *(int32 *)argv_p = result->of.i32;
                argv_p = (uint32 *)argv_p + 1;
                argc++;
                break;
            case WASM_I64:
                *(int64 *)argv_p = result->of.i64;
                argv_p = (uint64 *)argv_p + 1;
                argc++;
                break;
            case WASM_F32:
                *(float32 *)argv_p = result->of.f32;
                argv_p = (float32 *)argv_p + 1;
                argc++;
                break;
            case WASM_F64:
                *(float64 *)argv_p = result->of.f64;
                argv_p = (float64 *)argv_p + 1;
                argc++;
                break;
            default:
                NOT_REACHED();
                goto failed;
        }
    }

    return argc;
failed:
    return 0;
}

static void
native_func_trampoline(wasm_exec_env_t exec_env, uint64 *argv)
{
    wasm_val_t *params = NULL;
    wasm_val_t *results = NULL;
    uint32 argc = 0;
    const wasm_func_t *func = NULL;
    wasm_trap_t *trap = NULL;

    bh_assert(argv);

    func = wasm_runtime_get_function_attachment(exec_env);
    bh_assert(func);

    params = malloc_internal(wasm_func_param_arity(func) * sizeof(wasm_val_t));
    if (!params) {
        goto failed;
    }

    results =
      malloc_internal(wasm_func_result_arity(func) * sizeof(wasm_val_t));
    if (!results) {
        goto failed;
    }

    /* argv -> const wasm_val_t params[] */
    argc =
      argv_to_params(argv, wasm_functype_params(wasm_func_type(func)), params);
    if (wasm_func_param_arity(func) && !argc) {
        goto failed;
    }

    if (func->with_env) {
        trap = func->u.cb_env.cb(func->u.cb_env.env, params, results);
    }
    else {
        trap = func->u.cb(params, results);
    }

    if (trap) {
        wasm_name_t *message = NULL;
        wasm_trap_message(trap, message);
        LOG_WARNING("got a trap %s", message->data);
        wasm_name_delete(message);
    }

    /* there is no result or there is an exception */
    if (trap || !wasm_func_result_arity(func)) {
        memset(argv, 0, wasm_func_param_arity(func) * sizeof(uint64));
    }

    /* wasm_val_t results[] -> argv */
    argc = results_to_argv(results,
                           wasm_functype_results(wasm_func_type(func)), argv);
    if (wasm_func_result_arity(func) && !argc) {
        goto failed;
    }

failed:
    FREEIF(params);
    FREEIF(results);
    return;
}

wasm_func_t *
wasm_func_new(wasm_store_t *store,
              const wasm_functype_t *func_type,
              wasm_func_callback_t callback)
{
    wasm_func_t *func = NULL;

    check_engine_and_store(singleton_engine, store);
    bh_assert(func_type);

    func = malloc_internal(sizeof(wasm_func_t));
    if (!func) {
        goto failed;
    }

    func->kind = WASM_EXTERN_FUNC;
    func->with_env = false;
    func->u.cb = callback;

    func->func_type = wasm_functype_copy((wasm_functype_t *)func_type);
    if (!func->func_type) {
        goto failed;
    }

    return func;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_func_delete(func);
    return NULL;
}

wasm_func_t *
wasm_func_new_with_env(wasm_store_t *store,
                       const wasm_functype_t *func_type,
                       wasm_func_callback_with_env_t callback,
                       void *env,
                       void (*finalizer)(void *))
{
    wasm_func_t *func = NULL;

    check_engine_and_store(singleton_engine, store);
    bh_assert(func_type);

    func = malloc_internal(sizeof(wasm_func_t));
    if (!func) {
        goto failed;
    }

    func->kind = WASM_EXTERN_FUNC;
    func->with_env = true;
    func->u.cb_env.cb = callback;
    func->u.cb_env.env = env;
    func->u.cb_env.finalizer = finalizer;

    func->func_type = wasm_functype_copy((wasm_functype_t *)func_type);
    if (!func->func_type) {
        goto failed;
    }

    return func;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_func_delete(func);
    return NULL;
}

static wasm_func_t *
wasm_func_new_internal(wasm_store_t *store,
                       uint16 func_idx_rt,
                       WASMModuleInstanceCommon *inst_comm_rt)
{
    wasm_func_t *func = NULL;
    WASMType *type_rt = NULL;

    check_engine_and_store(singleton_engine, store);
    bh_assert(inst_comm_rt);

    func = malloc_internal(sizeof(wasm_func_t));
    if (!func) {
        goto failed;
    }

    func->kind = WASM_EXTERN_FUNC;

    if (INTERP_MODE == current_runtime_mode()) {
#if WASM_ENABLE_INTERP != 0
        bh_assert(func_idx_rt
                  < ((WASMModuleInstance *)inst_comm_rt)->function_count);
        WASMFunctionInstance *func_interp =
          ((WASMModuleInstance *)inst_comm_rt)->functions + func_idx_rt;
        type_rt = func_interp->is_import_func
                    ? func_interp->u.func_import->func_type
                    : func_interp->u.func->func_type;
#endif
    }
    else {
#if WASM_ENABLE_AOT != 0
        /* use same index to trace the function type in AOTFuncType **func_types */
        AOTModuleInstance *inst_aot = (AOTModuleInstance *)inst_comm_rt;
        AOTFunctionInstance *func_aot =
          (AOTFunctionInstance *)inst_aot->export_funcs.ptr + func_idx_rt;
        type_rt = func_aot->is_import_func ? func_aot->u.func_import->func_type
                                           : func_aot->u.func.func_type;
#endif
    }

    if (!type_rt) {
        goto failed;
    }

    func->func_type = wasm_functype_new_internal(type_rt);
    if (!func->func_type) {
        goto failed;
    }

    /* will add name information when processing "exports" */
    func->module_name = NULL;
    func->name = NULL;
    func->func_idx_rt = func_idx_rt;
    func->inst_comm_rt = inst_comm_rt;
    return func;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_func_delete(func);
    return NULL;
}

void
wasm_func_delete(wasm_func_t *func)
{
    if (!func) {
        return;
    }

    if (func->func_type) {
        wasm_functype_delete(func->func_type);
        func->func_type = NULL;
    }

    if (func->with_env) {
        if (func->u.cb_env.finalizer) {
            func->u.cb_env.finalizer(func->u.cb_env.env);
            func->u.cb_env.finalizer = NULL;
            func->u.cb_env.env = NULL;
        }
    }

    wasm_runtime_free(func);
}

wasm_func_t *
wasm_func_copy(const wasm_func_t *func)
{
    wasm_func_t *cloned = NULL;

    bh_assert(func);

    cloned = malloc_internal(sizeof(wasm_func_t));
    if (!cloned) {
        goto failed;
    }

    cloned->kind = func->kind;
    cloned->with_env = func->with_env;
    if (cloned->with_env) {
        cloned->u.cb_env.cb = func->u.cb_env.cb;
        cloned->u.cb_env.env = func->u.cb_env.env;
        cloned->u.cb_env.finalizer = func->u.cb_env.finalizer;
    }
    else {
        cloned->u.cb = func->u.cb;
    }

    cloned->func_idx_rt = func->func_idx_rt;
    cloned->inst_comm_rt = func->inst_comm_rt;
    cloned->func_type = wasm_functype_copy(func->func_type);
    if (!cloned->func_type) {
        goto failed;
    }

    return cloned;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_func_delete(cloned);
    return NULL;
}

wasm_functype_t *
wasm_func_type(const wasm_func_t *func)
{
    bh_assert(func);
    return func->func_type;
}

static uint32
params_to_argv(const wasm_val_t *params,
               const wasm_valtype_vec_t *param_defs,
               size_t param_arity,
               uint32 *out)
{
    size_t i = 0;
    uint32 argc = 0;
    const wasm_val_t *param = NULL;

    if (!param_arity) {
        return 0;
    }

    bh_assert(params && param_defs && out);
    bh_assert(param_defs->num_elems == param_arity);

    for (i = 0; out && i < param_arity; ++i) {
        param = params + i;
        bh_assert((*(param_defs->data + i))->kind == param->kind);

        switch (param->kind) {
            case WASM_I32:
                *(int32 *)out = param->of.i32;
                out += 1;
                argc += 1;
                break;
            case WASM_I64:
                *(int64 *)out = param->of.i64;
                out += 2;
                argc += 2;
                break;
            case WASM_F32:
                *(float32 *)out = param->of.f32;
                out += 1;
                argc += 1;
                break;
            case WASM_F64:
                *(float64 *)out = param->of.f64;
                out += 2;
                argc += 2;
                break;
            default:
                LOG_DEBUG("unexpected parameter val type %d", param->kind);
                goto failed;
        }
    }

    return argc;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    return 0;
}

static uint32
argv_to_results(const uint32 *results,
                const wasm_valtype_vec_t *result_defs,
                size_t result_arity,
                wasm_val_t *out)
{
    size_t i = 0;
    uint32 argc = 0;
    const uint32 *result = results;
    const wasm_valtype_t *def = NULL;

    if (!result_arity) {
        return 0;
    }

    bh_assert(results && result_defs && out);
    bh_assert(result_arity == result_defs->num_elems);

    for (i = 0; out && i < result_arity; i++) {
        def = *(result_defs->data + i);

        switch (def->kind) {
            case WASM_I32:
                out->kind = WASM_I32;
                out->of.i32 = *(int32 *)result;
                result += 1;
                break;
            case WASM_I64:
                out->kind = WASM_I64;
                out->of.i64 = *(int64 *)result;
                result += 2;
                break;
            case WASM_F32:
                out->kind = WASM_F32;
                out->of.f32 = *(float32 *)result;
                result += 1;
                break;
            case WASM_F64:
                out->kind = WASM_F64;
                out->of.f64 = *(float64 *)result;
                result += 2;
                break;
            default:
                LOG_DEBUG("unexpected parameter val type %d", def->kind);
                goto failed;
        }
        out++;
        argc++;
    }

    return argc;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    return 0;
}

wasm_trap_t *
wasm_func_call(const wasm_func_t *func,
               const wasm_val_t params[],
               wasm_val_t results[])
{
    /* parameters count as if all are uint32 */
    /* a int64 or float64 parameter means 2 */
    uint32 argc = 0;
    /* a parameter list and a return value list */
    uint32 *argv = NULL;
    WASMFunctionInstanceCommon *func_comm_rt = NULL;
    size_t param_count = 0;
    size_t result_count = 0;

    bh_assert(func && func->func_type && func->inst_comm_rt);

    if (INTERP_MODE == current_runtime_mode()) {
#if WASM_ENABLE_INTERP != 0
        func_comm_rt = ((WASMModuleInstance *)func->inst_comm_rt)->functions
                       + func->func_idx_rt;
#endif
    }
    else {
#if WASM_ENABLE_AOT != 0
        AOTModuleInstance *inst_aot = (AOTModuleInstance *)func->inst_comm_rt;
        func_comm_rt = (AOTFunctionInstance *)inst_aot->export_funcs.ptr
                       + func->func_idx_rt;
#endif
    }
    if (!func_comm_rt) {
        goto failed;
    }

    param_count = wasm_func_param_arity(func);
    result_count = wasm_func_result_arity(func);
    argv = malloc_internal(
      sizeof(uint64)
      * (param_count > result_count ? param_count : result_count));
    if (!argv) {
        goto failed;
    }

    /* copy parametes */
    argc = params_to_argv(params, wasm_functype_params(wasm_func_type(func)),
                          wasm_func_param_arity(func), argv);
    if (wasm_func_param_arity(func) && !argc) {
        goto failed;
    }

    if (!wasm_runtime_create_exec_env_and_call_wasm(
          func->inst_comm_rt, func_comm_rt, argc, argv)) {
        LOG_DEBUG("wasm_runtime_create_exec_env_and_call_wasm failed");
        goto failed;
    }

    /* copy results */
    argc = argv_to_results(argv, wasm_functype_results(wasm_func_type(func)),
                           wasm_func_result_arity(func), results);
    if (wasm_func_result_arity(func) && !argc) {
        goto failed;
    }

    FREEIF(argv);
    /* should return trap */
    return NULL;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    FREEIF(argv);
    return NULL;
}

size_t
wasm_func_param_arity(const wasm_func_t *func)
{
    bh_assert(func && func->func_type && func->func_type->params);
    return func->func_type->params->num_elems;
}

size_t
wasm_func_result_arity(const wasm_func_t *func)
{
    bh_assert(func && func->func_type && func->func_type->results);
    return func->func_type->results->num_elems;
}

wasm_extern_t *
wasm_func_as_extern(wasm_func_t *func)
{
    return (wasm_extern_t *)func;
}

wasm_global_t *
wasm_global_new(wasm_store_t *store,
                const wasm_globaltype_t *global_type,
                const wasm_val_t *init)
{
    wasm_global_t *global = NULL;

    check_engine_and_store(singleton_engine, store);
    bh_assert(store && global_type && init);

    global = malloc_internal(sizeof(wasm_global_t));
    if (!global) {
        goto failed;
    }

    global->kind = WASM_EXTERN_GLOBAL;
    global->type = wasm_globaltype_copy((wasm_globaltype_t *)global_type);
    if (!global->type) {
        goto failed;
    }

    global->init = malloc_internal(sizeof(wasm_val_t));
    if (!global->init) {
        goto failed;
    }

    wasm_val_copy(global->init, init);
    /* TODO: how to check if above is failed */

    return global;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_global_delete(global);
    return NULL;
}

/* almost same with wasm_global_new */
wasm_global_t *
wasm_global_copy(const wasm_global_t *src)
{
    wasm_global_t *global = NULL;

    bh_assert(src);

    global = malloc_internal(sizeof(wasm_global_t));
    if (!global) {
        goto failed;
    }

    global->kind = WASM_EXTERN_GLOBAL;
    global->type = wasm_globaltype_copy((wasm_globaltype_t *)src->type);
    if (!global->type) {
        goto failed;
    }

    global->init = malloc_internal(sizeof(wasm_val_t));
    if (!global->init) {
        goto failed;
    }

    wasm_val_copy(global->init, src->init);

    global->global_idx_rt = src->global_idx_rt;
    global->inst_comm_rt = src->inst_comm_rt;

    return global;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_global_delete(global);
    return NULL;
}

void
wasm_global_delete(wasm_global_t *global)
{
    if (!global) {
        return;
    }

    if (global->init) {
        wasm_val_delete(global->init);
        wasm_runtime_free(global->init);
        global->init = NULL;
    }

    if (global->type) {
        wasm_globaltype_delete(global->type);
        global->type = NULL;
    }

    wasm_runtime_free(global);
}

bool
wasm_global_same(const wasm_global_t *g1, const wasm_global_t *g2)
{
    if (!g1 && !g2) {
        return true;
    }

    if (!g1 || !g2) {
        return false;
    }

    return g1->kind == g2->kind && wasm_globaltype_same(g1->type, g2->type)
           && wasm_val_same(g1->init, g2->init);
}

#if WASM_ENABLE_INTERP != 0
static bool
interp_global_set(const WASMModuleInstance *inst_interp,
                  uint16 global_idx_rt,
                  const wasm_val_t *v)
{
    const WASMGlobalInstance *global_interp =
      inst_interp->globals + global_idx_rt;
    uint8 val_type_rt = global_interp->type;
    uint8 *data = inst_interp->global_data + global_interp->data_offset;
    bool ret = true;

    switch (val_type_rt) {
        case VALUE_TYPE_I32:
            bh_assert(WASM_I32 == v->kind);
            *((int32 *)data) = v->of.i32;
            break;
        case VALUE_TYPE_F32:
            bh_assert(WASM_F32 == v->kind);
            *((float32 *)data) = v->of.f32;
            break;
        case VALUE_TYPE_I64:
            bh_assert(WASM_I64 == v->kind);
            *((int64 *)data) = v->of.i64;
            break;
        case VALUE_TYPE_F64:
            bh_assert(WASM_F64 == v->kind);
            *((float64 *)data) = v->of.f64;
            break;
        default:
            LOG_DEBUG("unexpected value type %d", val_type_rt);
            ret = false;
            break;
    }

    return ret;
}

static bool
interp_global_get(const WASMModuleInstance *inst_interp,
                  uint16 global_idx_rt,
                  wasm_val_t *out)
{
    WASMGlobalInstance *global_interp = inst_interp->globals + global_idx_rt;
    uint8 val_type_rt = global_interp->type;
    uint8 *data = inst_interp->global_data + global_interp->data_offset;
    bool ret = true;

    switch (val_type_rt) {
        case VALUE_TYPE_I32:
            out->kind = WASM_I32;
            out->of.i32 = *((int32 *)data);
            break;
        case VALUE_TYPE_F32:
            out->kind = WASM_F32;
            out->of.f32 = *((float32 *)data);
            break;
        case VALUE_TYPE_I64:
            out->kind = WASM_I64;
            out->of.i64 = *((int64 *)data);
            break;
        case VALUE_TYPE_F64:
            out->kind = WASM_F64;
            out->of.f64 = *((float64 *)data);
            break;
        default:
            LOG_DEBUG("unexpected value type %d", val_type_rt);
            ret = false;
    }
    return ret;
}
#endif

#if WASM_ENABLE_AOT != 0
static bool
aot_global_set(const AOTModuleInstance *inst_aot,
               uint16 global_idx_rt,
               const wasm_val_t *v)
{
    AOTModule *module_aot = inst_aot->aot_module.ptr;
    uint8 val_type_rt = 0;
    uint32 data_offset = 0;
    void *data = NULL;
    bool ret = true;

    if (global_idx_rt < module_aot->import_global_count) {
        data_offset = module_aot->import_globals[global_idx_rt].data_offset;
        val_type_rt = module_aot->import_globals[global_idx_rt].type;
    }
    else {
        data_offset =
          module_aot->globals[global_idx_rt - module_aot->import_global_count]
            .data_offset;
        val_type_rt =
          module_aot->globals[global_idx_rt - module_aot->import_global_count]
            .type;
    }

    data = (void *)((uint8 *)inst_aot->global_data.ptr + data_offset);
    switch (val_type_rt) {
        case VALUE_TYPE_I32:
            bh_assert(WASM_I32 == v->kind);
            *((int32 *)data) = v->of.i32;
            break;
        case VALUE_TYPE_F32:
            bh_assert(WASM_F32 == v->kind);
            *((float32 *)data) = v->of.f32;
            break;
        case VALUE_TYPE_I64:
            bh_assert(WASM_I64 == v->kind);
            *((int64 *)data) = v->of.i64;
            break;
        case VALUE_TYPE_F64:
            bh_assert(WASM_F64 == v->kind);
            *((float64 *)data) = v->of.f64;
            break;
        default:
            LOG_DEBUG("unexpected value type %d", val_type_rt);
            ret = false;
    }
    return ret;
}

static bool
aot_global_get(const AOTModuleInstance *inst_aot,
               uint16 global_idx_rt,
               wasm_val_t *out)
{
    AOTModule *module_aot = inst_aot->aot_module.ptr;
    uint8 val_type_rt = 0;
    uint32 data_offset = 0;
    void *data = NULL;
    bool ret = true;

    if (global_idx_rt < module_aot->import_global_count) {
        data_offset = module_aot->import_globals[global_idx_rt].data_offset;
        val_type_rt = module_aot->import_globals[global_idx_rt].type;
    }
    else {
        data_offset =
          module_aot->globals[global_idx_rt - module_aot->import_global_count]
            .data_offset;
        val_type_rt =
          module_aot->globals[global_idx_rt - module_aot->import_global_count]
            .type;
    }

    data = (void *)((uint8 *)inst_aot->global_data.ptr + data_offset);
    switch (val_type_rt) {
        case VALUE_TYPE_I32:
            out->kind = WASM_I32;
            out->of.i32 = *((int32 *)data);
            break;
        case VALUE_TYPE_F32:
            out->kind = WASM_F32;
            out->of.f32 = *((float32 *)data);
            break;
        case VALUE_TYPE_I64:
            out->kind = WASM_I64;
            out->of.i64 = *((int64 *)data);
            break;
        case VALUE_TYPE_F64:
            out->kind = WASM_F64;
            out->of.f64 = *((float64 *)data);
            break;
        default:
            LOG_DEBUG("unexpected value type %d", val_type_rt);
            ret = false;
    }
    return ret;
}
#endif

void
wasm_global_set(wasm_global_t *global, const wasm_val_t *v)
{
    bh_assert(global && v);

    if (INTERP_MODE == current_runtime_mode()) {
#if WASM_ENABLE_INTERP != 0
        (void)interp_global_set((WASMModuleInstance *)global->inst_comm_rt,
                                global->global_idx_rt, v);
#endif
    }
    else {
#if WASM_ENABLE_AOT != 0
        (void)aot_global_set((AOTModuleInstance *)global->inst_comm_rt,
                             global->global_idx_rt, v);
#endif
    }
}

void
wasm_global_get(const wasm_global_t *global, wasm_val_t *out)
{
    bh_assert(global && out);

    memset(out, 0, sizeof(wasm_val_t));

    if (INTERP_MODE == current_runtime_mode()) {
#if WASM_ENABLE_INTERP != 0
        (void)interp_global_get((WASMModuleInstance *)global->inst_comm_rt,
                                global->global_idx_rt, out);
#endif
    }
    else {
#if WASM_ENABLE_AOT != 0
        (void)aot_global_get((AOTModuleInstance *)global->inst_comm_rt,
                             global->global_idx_rt, out);
#endif
    }

    bh_assert(global->init->kind == out->kind);
}

static wasm_global_t *
wasm_global_new_internal(wasm_store_t *store,
                         uint16 global_idx_rt,
                         WASMModuleInstanceCommon *inst_comm_rt)
{
    wasm_global_t *global = NULL;
    uint8 val_type_rt = 0;
    bool is_mutable = 0;

    check_engine_and_store(singleton_engine, store);
    bh_assert(inst_comm_rt);

    global = malloc_internal(sizeof(wasm_global_t));
    if (!global) {
        goto failed;
    }

    /*
     * global->module_name = NULL;
     * global->name = NULL;
     */
    global->kind = WASM_EXTERN_GLOBAL;

    if (INTERP_MODE == current_runtime_mode()) {
#if WASM_ENABLE_INTERP != 0
        WASMGlobalInstance *global_interp =
          ((WASMModuleInstance *)inst_comm_rt)->globals + global_idx_rt;
        val_type_rt = global_interp->type;
        is_mutable = global_interp->is_mutable;
#endif
    }
    else {
#if WASM_ENABLE_AOT != 0
        AOTModuleInstance *inst_aot = (AOTModuleInstance *)inst_comm_rt;
        AOTModule *module_aot = inst_aot->aot_module.ptr;
        if (global_idx_rt < module_aot->import_global_count) {
            AOTImportGlobal *global_import_aot =
              module_aot->import_globals + global_idx_rt;
            val_type_rt = global_import_aot->type;
            is_mutable = global_import_aot->is_mutable;
        }
        else {
            AOTGlobal *global_aot =
              module_aot->globals
              + (global_idx_rt - module_aot->import_global_count);
            val_type_rt = global_aot->type;
            is_mutable = global_aot->is_mutable;
        }
#endif
    }

    global->type = wasm_globaltype_new_internal(val_type_rt, is_mutable);
    if (!global->type) {
        goto failed;
    }

    global->init = malloc_internal(sizeof(wasm_val_t));
    if (!global->init) {
        goto failed;
    }

    if (INTERP_MODE == current_runtime_mode()) {
#if WASM_ENABLE_INTERP != 0
        interp_global_get((WASMModuleInstance *)inst_comm_rt, global_idx_rt,
                          global->init);
#endif
    }
    else {
#if WASM_ENABLE_AOT != 0
        aot_global_get((AOTModuleInstance *)inst_comm_rt, global_idx_rt,
                       global->init);
#endif
    }

    global->inst_comm_rt = inst_comm_rt;
    global->global_idx_rt = global_idx_rt;

    return global;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_global_delete(global);
    return NULL;
}

wasm_globaltype_t *
wasm_global_type(const wasm_global_t *global)
{
    bh_assert(global);
    return global->type;
}

wasm_extern_t *
wasm_global_as_extern(wasm_global_t *global)
{
    return (wasm_extern_t *)global;
}

void
wasm_table_delete(wasm_table_t *table)
{}

void
wasm_memory_delete(wasm_memory_t *memory)
{}

#if WASM_ENABLE_INTERP != 0
static bool
interp_link_func(const wasm_instance_t *inst,
                 const WASMModule *module_interp,
                 uint16 func_idx_rt,
                 wasm_func_t *import)
{
    WASMImport *imported_func_interp = NULL;
    wasm_func_t *cloned = NULL;

    bh_assert(inst && module_interp && import);
    bh_assert(func_idx_rt < module_interp->import_function_count);
    bh_assert(WASM_EXTERN_FUNC == import->kind);

    imported_func_interp = module_interp->import_functions + func_idx_rt;
    bh_assert(imported_func_interp);

    cloned = wasm_func_copy(import);
    if (!cloned || !bh_vector_append((Vector *)inst->imports, &cloned)) {
        return false;
    }

    /* add native_func_trampoline as a NativeSymbol */
    imported_func_interp->u.function.call_conv_raw = true;
    imported_func_interp->u.function.attachment = cloned;
    imported_func_interp->u.function.func_ptr_linked = native_func_trampoline;
    import->func_idx_rt = func_idx_rt;

    return true;
}

static bool
interp_link_global(const WASMModule *module_interp,
                   uint16 global_idx_rt,
                   wasm_global_t *import)
{
    WASMImport *imported_global_interp = NULL;

    bh_assert(module_interp && import);
    bh_assert(global_idx_rt < module_interp->import_global_count);
    bh_assert(WASM_EXTERN_GLOBAL == import->kind);

    imported_global_interp = module_interp->import_globals + global_idx_rt;
    bh_assert(imported_global_interp);

    /* set init value */
    switch (wasm_valtype_kind(import->type->val_type)) {
        case WASM_I32:
            bh_assert(VALUE_TYPE_I32 == imported_global_interp->u.global.type);
            imported_global_interp->u.global.global_data_linked.i32 =
              import->init->of.i32;
            break;
        case WASM_I64:
            bh_assert(VALUE_TYPE_I64 == imported_global_interp->u.global.type);
            imported_global_interp->u.global.global_data_linked.i64 =
              import->init->of.i64;
            break;
        case WASM_F32:
            bh_assert(VALUE_TYPE_F32 == imported_global_interp->u.global.type);
            imported_global_interp->u.global.global_data_linked.f32 =
              import->init->of.f32;
            break;
        case WASM_F64:
            bh_assert(VALUE_TYPE_F64 == imported_global_interp->u.global.type);
            imported_global_interp->u.global.global_data_linked.f64 =
              import->init->of.f64;
            break;
        default:
            return false;
    }

    import->global_idx_rt = global_idx_rt;
    return true;
}

static bool
interp_link_memory(const WASMModule *module_interp,
                   uint16 memory_inst_index,
                   wasm_memory_t *import)
{
    return false;
}

static bool
interp_link_table(const WASMModule *module_interp,
                  uint16 table_inst_index,
                  wasm_table_t *import)
{
    return false;
}

static uint32
interp_link(const wasm_instance_t *inst,
            const WASMModule *module_interp,
            wasm_extern_t *imports[])
{
    uint32 i = 0;
    uint32 import_func_i = 0;
    uint32 import_global_i = 0;
    uint32 import_table_i = 0;
    uint32 import_memory_i = 0;
    wasm_func_t *func = NULL;
    wasm_global_t *global = NULL;

    bh_assert(inst && module_interp && imports);

    for (i = 0; i < module_interp->import_count; ++i) {
        wasm_extern_t *import = imports[i];
        WASMImport *import_rt = module_interp->imports + i;

        switch (import_rt->kind) {
            case IMPORT_KIND_FUNC:
                func = wasm_extern_as_func(import);
                if (!interp_link_func(inst, module_interp, import_func_i++,
                                      func)) {
                    goto failed;
                }

                break;
            case IMPORT_KIND_GLOBAL:
                global = wasm_extern_as_global(import);
                if (!interp_link_global(module_interp, import_global_i++,
                                        global)) {
                    goto failed;
                }

                break;
            case IMPORT_KIND_MEMORY:
                if (!interp_link_memory(module_interp, import_memory_i++,
                                        wasm_extern_as_memory(import))) {
                    goto failed;
                };
                break;
            case IMPORT_KIND_TABLE:
                if (!interp_link_table(module_interp, import_table_i++,
                                       wasm_extern_as_table(import))) {
                    goto failed;
                }
                break;
            default:
                NOT_REACHED();
                break;
        }
    }

    return module_interp->import_count;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    return (uint32)-1;
}

static bool
interp_process_export(wasm_store_t *store,
                      const WASMModuleInstance *inst_interp,
                      wasm_extern_vec_t *externals)
{
    WASMExport *exports = NULL;
    WASMExport *export = NULL;
    wasm_func_t *func = NULL;
    wasm_global_t *global = NULL;
    wasm_extern_t *external = NULL;
    uint32 export_cnt = 0;
    uint32 i = 0;

    bh_assert(store && inst_interp && externals);

    exports = inst_interp->module->exports;
    export_cnt = inst_interp->module->export_count;

    for (i = 0; i < export_cnt; ++i) {
        export = exports + i;

        switch (export->kind) {
            case EXPORT_KIND_FUNC:
                func = wasm_func_new_internal(
                  store, export->index,
                  (WASMModuleInstanceCommon *)inst_interp);
                if (!func) {
                    goto failed;
                }

                external = wasm_func_as_extern(func);
                break;
            case EXPORT_KIND_GLOBAL:
                global = wasm_global_new_internal(
                  store, export->index,
                  (WASMModuleInstanceCommon *)inst_interp);
                if (!global) {
                    goto failed;
                }

                external = wasm_global_as_extern(global);
                break;
            case EXPORT_KIND_MEMORY:
            case EXPORT_KIND_TABLE:
                /* TODO: */
                break;
            default:
                goto failed;
        }

        if (!bh_vector_append((Vector *)externals, &external)) {
            goto failed;
        }
    }

    return true;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    return false;
}
#endif /* WASM_ENABLE_INTERP */

#if WASM_ENABLE_AOT != 0
static bool
aot_link_func(const wasm_instance_t *inst,
              const AOTModule *module_aot,
              uint32 import_func_idx_rt,
              wasm_func_t *import)
{
    AOTImportFunc *import_aot_func = NULL;
    wasm_func_t *cloned = NULL;

    bh_assert(inst && module_aot && import);

    import_aot_func = module_aot->import_funcs + import_func_idx_rt;
    bh_assert(import_aot_func);

    cloned = wasm_func_copy(import);
    if (!cloned || !bh_vector_append((Vector *)inst->imports, &cloned)) {
        return false;
    }

    import_aot_func->call_conv_raw = true;
    import_aot_func->attachment = wasm_func_copy(import);
    import_aot_func->func_ptr_linked = native_func_trampoline;
    import->func_idx_rt = import_func_idx_rt;

    return true;
}

static bool
aot_link_global(const AOTModule *module_aot,
                uint16 global_idx_rt,
                wasm_global_t *import)
{
    AOTImportGlobal *import_aot_global = NULL;
    const wasm_valtype_t *val_type = NULL;

    bh_assert(module_aot && import);

    import_aot_global = module_aot->import_globals + global_idx_rt;
    bh_assert(import_aot_global);

    val_type = wasm_globaltype_content(wasm_global_type(import));
    bh_assert(val_type);

    switch (wasm_valtype_kind(val_type)) {
        case WASM_I32:
            bh_assert(VALUE_TYPE_I32 == import_aot_global->type);
            import_aot_global->global_data_linked.i32 = import->init->of.i32;
            break;
        case WASM_I64:
            bh_assert(VALUE_TYPE_I64 == import_aot_global->type);
            import_aot_global->global_data_linked.i64 = import->init->of.i64;
            break;
        case WASM_F32:
            bh_assert(VALUE_TYPE_F32 == import_aot_global->type);
            import_aot_global->global_data_linked.f32 = import->init->of.f32;
            break;
        case WASM_F64:
            bh_assert(VALUE_TYPE_F64 == import_aot_global->type);
            import_aot_global->global_data_linked.f64 = import->init->of.f64;
            break;
        default:
            goto failed;
    }

    import->global_idx_rt = global_idx_rt;
    return true;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    return false;
}

static uint32
aot_link(const wasm_instance_t *inst,
         const AOTModule *module_aot,
         wasm_extern_t *imports[])
{
    uint32 i = 0;
    uint32 import_func_i = 0;
    uint32 import_global_i = 0;
    wasm_extern_t *import = NULL;
    wasm_func_t *func = NULL;
    wasm_global_t *global = NULL;

    bh_assert(inst && module_aot && imports);

    while (import_func_i < module_aot->import_func_count
           || import_global_i < module_aot->import_global_count) {
        import = imports[i++];

        bh_assert(import);

        switch (wasm_extern_kind(import)) {
            case WASM_EXTERN_FUNC:
                bh_assert(import_func_i < module_aot->import_func_count);
                func = wasm_extern_as_func((wasm_extern_t *)import);
                if (!aot_link_func(inst, module_aot, import_func_i++, func)) {
                    goto failed;
                }

                break;
            case WASM_EXTERN_GLOBAL:
                bh_assert(import_global_i < module_aot->import_global_count);
                global = wasm_extern_as_global((wasm_extern_t *)import);
                if (!aot_link_global(module_aot, import_global_i++, global)) {
                    goto failed;
                }

                break;
            case WASM_EXTERN_MEMORY:
                break;
            case WASM_EXTERN_TABLE:
                break;
            default:
                goto failed;
        }
    }

    return i;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    return (uint32)-1;
}

static bool
aot_process_export(wasm_store_t *store,
                   const AOTModuleInstance *inst_aot,
                   wasm_extern_vec_t *externals)
{
    uint32 i = 0;
    uint32 export_func_i = 0;
    wasm_extern_t *external = NULL;
    AOTModule *module_aot = NULL;

    bh_assert(store && inst_aot && externals);

    module_aot = (AOTModule *)inst_aot->aot_module.ptr;
    bh_assert(module_aot);

    for (i = 0; i < module_aot->export_count; ++i) {
        AOTExport *export = module_aot->exports + i;
        wasm_func_t *func = NULL;
        wasm_global_t *global = NULL;

        switch (export->kind) {
            case EXPORT_KIND_FUNC:
                func =
                  wasm_func_new_internal(store, export_func_i++,
                                         (WASMModuleInstanceCommon *)inst_aot);
                if (!func) {
                    goto failed;
                }

                external = wasm_func_as_extern(func);
                break;
            case EXPORT_KIND_GLOBAL:
                global = wasm_global_new_internal(
                  store, export->index, (WASMModuleInstanceCommon *)inst_aot);
                if (!global) {
                    goto failed;
                }

                external = wasm_global_as_extern(global);
                break;
            case EXPORT_KIND_MEMORY:
            case EXPORT_KIND_TABLE:
                break;
            default:
                NOT_REACHED();
                goto failed;
        }

        if (!bh_vector_append((Vector *)externals, &external)) {
            goto failed;
        }
    }

    return true;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    return false;
}
#endif /* WASM_ENABLE_AOT */

wasm_instance_t *
wasm_instance_new(wasm_store_t *store,
                  const wasm_module_t *module,
                  const wasm_extern_t *const imports[],
                  wasm_trap_t **traps)
{
    char error[128] = { 0 };
    const uint32 stack_size = 16 * 1024;
    const uint32 heap_size = 16 * 1024;
    uint32 import_count = 0;
    wasm_instance_t *instance = NULL;
    uint32 i = 0;
    (void)traps;

    check_engine_and_store(singleton_engine, store);

    instance = malloc_internal(sizeof(wasm_instance_t));
    if (!instance) {
        goto failed;
    }

    /* link module and imports */
    if (INTERP_MODE == current_runtime_mode()) {
#if WASM_ENABLE_INTERP != 0
        import_count = ((WASMModule *)*module)->import_count;
        INIT_VEC(instance->imports, wasm_extern_vec, import_count);
        if (!instance->imports) {
            goto failed;
        }

        import_count = interp_link(instance, (WASMModule *)*module,
                                   (wasm_extern_t **)imports);
#endif
    }
    else {
#if WASM_ENABLE_AOT != 0
        import_count = ((AOTModule *)*module)->import_func_count
                       + ((AOTModule *)*module)->import_global_count
                       + ((AOTModule *)*module)->import_memory_count
                       + ((AOTModule *)*module)->import_table_count;
        INIT_VEC(instance->imports, wasm_extern_vec, import_count);
        if (!instance->imports) {
            goto failed;
        }

        import_count =
          aot_link(instance, (AOTModule *)*module, (wasm_extern_t **)imports);
#endif
    }
    if ((int32)import_count < 0) {
        goto failed;
    }

    instance->inst_comm_rt = wasm_runtime_instantiate(
      *module, stack_size, heap_size, error, sizeof(error));
    if (!instance->inst_comm_rt) {
        LOG_ERROR(error);
        goto failed;
    }

    /* fill in inst */
    for (i = 0; i < (uint32)import_count; ++i) {
        wasm_extern_t *import = (wasm_extern_t *)imports[i];
        switch (import->kind) {
            case WASM_EXTERN_FUNC:
                wasm_extern_as_func(import)->inst_comm_rt =
                  instance->inst_comm_rt;
                break;
            case WASM_EXTERN_GLOBAL:
                wasm_extern_as_global(import)->inst_comm_rt =
                  instance->inst_comm_rt;
                break;
            case WASM_EXTERN_MEMORY:
                wasm_extern_as_memory(import)->inst_comm_rt =
                  instance->inst_comm_rt;
                break;
            case WASM_EXTERN_TABLE:
                wasm_extern_as_table(import)->inst_comm_rt =
                  instance->inst_comm_rt;
                break;
            default:
                goto failed;
        }
    }

    /* build the exports list */
    if (INTERP_MODE == current_runtime_mode()) {
#if WASM_ENABLE_INTERP != 0
        uint32 export_cnt =
          ((WASMModuleInstance *)instance->inst_comm_rt)->module->export_count;

        INIT_VEC(instance->exports, wasm_extern_vec, export_cnt);

        if (!interp_process_export(
              store, (WASMModuleInstance *)instance->inst_comm_rt,
              instance->exports)) {
            goto failed;
        }
#endif
    }
    else {
#if WASM_ENABLE_AOT != 0
        uint32 export_cnt =
          ((AOTModuleInstance *)instance->inst_comm_rt)->export_func_count;

        INIT_VEC(instance->exports, wasm_extern_vec, export_cnt);

        if (!aot_process_export(store,
                                (AOTModuleInstance *)instance->inst_comm_rt,
                                instance->exports)) {
            goto failed;
        }
#endif
    }

    /* add it to a watching list in store */
    if (!bh_vector_append((Vector *)store->instances, &instance)) {
        goto failed;
    }

    return instance;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_instance_delete_internal(instance);
    return NULL;
}

void
wasm_instance_delete_internal(wasm_instance_t *instance)
{
    if (!instance) {
        return;
    }

    DEINIT_VEC(instance->imports, wasm_extern_vec_delete);
    DEINIT_VEC(instance->exports, wasm_extern_vec_delete);

    if (instance->inst_comm_rt) {
        wasm_runtime_deinstantiate(instance->inst_comm_rt);
        instance->inst_comm_rt = NULL;
    }
    wasm_runtime_free(instance);
}

/* will release instance when releasing the store */
void
wasm_instance_delete(wasm_instance_t *module)
{
    /* pass */
}

void
wasm_instance_exports(const wasm_instance_t *instance, wasm_extern_vec_t *out)
{
    bh_assert(instance && out);
    wasm_extern_vec_copy(out, instance->exports);
}

void
wasm_instance_vec_new_uninitialized(wasm_instance_vec_t *out, size_t size)
{
    generic_vec_init_data((Vector *)out, size, sizeof(wasm_instance_t));
}

void
wasm_instance_vec_delete(wasm_instance_vec_t *instance_vec)
{
    size_t i = 0;
    if (!instance_vec || !instance_vec->data) {
        return;
    }

    FREE_VEC_ELEMS(instance_vec, wasm_instance_delete_internal);
    bh_vector_destroy((Vector *)instance_vec);
}

wasm_extern_t *
wasm_extern_copy(const wasm_extern_t *src)
{
    wasm_extern_t *dst = NULL;
    wasm_func_t *func = NULL;
    wasm_global_t *global = NULL;
    bh_assert(src);

    switch (wasm_extern_kind(src)) {
        case WASM_EXTERN_FUNC:
            func = wasm_func_copy(wasm_extern_as_func_const(src));
            dst = wasm_func_as_extern(func);
            break;
        case WASM_EXTERN_GLOBAL:
            global = wasm_global_copy(wasm_extern_as_global_const(src));
            dst = wasm_global_as_extern(global);
            break;
        case WASM_EXTERN_MEMORY:
        case WASM_EXTERN_TABLE:
        default:
            break;
    }

    if (!dst) {
        goto failed;
    }

    return dst;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_extern_delete(dst);
    return NULL;
}

void
wasm_extern_delete(wasm_extern_t *external)
{
    if (!external) {
        return;
    }

    switch (wasm_extern_kind(external)) {
        case WASM_EXTERN_FUNC:
            wasm_func_delete(wasm_extern_as_func(external));
            break;
        case WASM_EXTERN_GLOBAL:
            wasm_global_delete(wasm_extern_as_global(external));
            break;
        case WASM_EXTERN_MEMORY:
        case WASM_EXTERN_TABLE:
        default:
            break;
    }
}

wasm_externkind_t
wasm_extern_kind(const wasm_extern_t *extrenal)
{
    return extrenal->kind;
}

wasm_func_t *
wasm_extern_as_func(wasm_extern_t *external)
{
    return (wasm_func_t *)external;
}

const wasm_func_t *
wasm_extern_as_func_const(const wasm_extern_t *external)
{
    return (const wasm_func_t *)external;
}

wasm_global_t *
wasm_extern_as_global(wasm_extern_t *external)
{
    return (wasm_global_t *)external;
}

const wasm_global_t *
wasm_extern_as_global_const(const wasm_extern_t *external)
{
    return (const wasm_global_t *)external;
}

wasm_memory_t *
wasm_extern_as_memory(wasm_extern_t *external)
{
    return (wasm_memory_t *)external;
}

const wasm_memory_t *
wasm_extern_as_memory_const(const wasm_extern_t *external)
{
    return (const wasm_memory_t *)external;
}

wasm_table_t *
wasm_extern_as_table(wasm_extern_t *external)
{
    return (wasm_table_t *)external;
}

const wasm_table_t *
wasm_extern_as_table_const(const wasm_extern_t *external)
{
    return (const wasm_table_t *)external;
}

void
wasm_extern_vec_copy(wasm_extern_vec_t *out, const wasm_extern_vec_t *src)
{
    size_t i = 0;
    bh_assert(out && src);

    generic_vec_init_data((Vector *)out, src->size, src->size_of_elem);
    if (!out->data) {
        goto failed;
    }

    for (i = 0; i < src->num_elems; ++i) {
        wasm_extern_t *orig = *(src->data + i);
        wasm_extern_t *cloned = wasm_extern_copy(orig);
        if (!cloned) {
            goto failed;
        }

        if (!bh_vector_append((Vector *)out, &cloned)) {
            goto failed;
        }
    }

    return;

failed:
    LOG_DEBUG("%s failed", __FUNCTION__);
    wasm_extern_vec_delete(out);
    return;
}

void
wasm_extern_vec_new_uninitialized(wasm_extern_vec_t *out, size_t size)
{
    generic_vec_init_data((Vector *)out, size, sizeof(wasm_extern_t *));
}

void
wasm_extern_vec_delete(wasm_extern_vec_t *extern_vec)
{
    size_t i = 0;
    if (!extern_vec || !extern_vec->data) {
        return;
    }

    FREE_VEC_ELEMS(extern_vec, wasm_extern_delete);
    bh_vector_destroy((Vector *)extern_vec);
}
