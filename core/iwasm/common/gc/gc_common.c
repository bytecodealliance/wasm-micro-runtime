/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "../wasm_runtime_common.h"
#if WASM_ENABLE_INTERP != 0
#include "../interpreter/wasm_runtime.h"
#endif
#if WASM_ENABLE_AOT != 0
#include "../aot/aot_runtime.h"
#endif

uint32
wasm_get_defined_type_count(WASMModuleCommon *const module)
{
    /* TODO */
    return 0;
}

WASMType *
wasm_get_defined_type(WASMModuleCommon *const module, uint32 index)
{
    /* TODO */
    return NULL;
}

bool
wasm_defined_type_is_func_type(WASMType *const def_type)
{
    return wasm_type_is_func_type(def_type);
}

bool
wasm_defined_type_is_struct_type(WASMType *const def_type)
{
    /* TODO */
    return false;
}

bool
wasm_defined_type_is_array_type(WASMType *const def_type)
{
    return wasm_type_is_array_type(def_type);
}

uint32
wasm_func_type_get_param_count(WASMFuncType *const func_type)
{
    return func_type->param_count;
}

wasm_ref_type_t
wasm_func_type_get_param_type(WASMFuncType *const func_type, uint32 param_idx)
{
    bh_assert(param_idx < func_type->param_count);
    WASMRefTypeMap ref_type_maps = func_type->ref_type_maps[param_idx];
    RefHeapType_Common ref_ht_common = ref_type_maps.ref_type->ref_ht_common;
    wasm_ref_type_t ref_type = {
        .value_type = ref_ht_common.ref_type,
        .nullable = ref_ht_common.nullable,
        .heap_type = ref_ht_common.heap_type,
    };
    return ref_type;
}

uint32
wasm_func_type_get_result_count(WASMFuncType *const func_type)
{
    return (uint32)func_type->result_count;
}

wasm_ref_type_t
wasm_func_type_get_result_type(WASMFuncType *const func_type, uint32 result_idx)
{
    bh_assert(result_idx < func_type->result_count);
    WASMRefTypeMap result_ref_type_maps =
        func_type->result_ref_type_maps[result_idx];
    RefHeapType_Common ref_ht_common =
        result_ref_type_maps.ref_type->ref_ht_common;
    wasm_ref_type_t ref_type = {
        .value_type = ref_ht_common.ref_type,
        .nullable = ref_ht_common.nullable,
        .heap_type = ref_ht_common.heap_type,
    };
    return ref_type;
}

uint32
wasm_struct_type_get_field_count(WASMStructType *const struct_type)
{
    /* TODO */
    return 0;
}

wasm_ref_type_t
wasm_struct_type_get_field_type(WASMStructType *const struct_type,
                                uint32 field_idx, bool *p_is_mutable)
{
    wasm_ref_type_t ref_type = { 0 };
    /* TODO */
    return ref_type;
}

wasm_ref_type_t
wasm_array_type_get_elem_type(WASMArrayType *const array_type,
                              bool *p_is_mutable)
{
    WASMRefType *elem_ref_type = array_type->elem_ref_type;
    RefHeapType_Common ref_ht_common = elem_ref_type->ref_ht_common;
    wasm_ref_type_t ref_type = {
        .value_type = ref_ht_common.ref_type,
        .nullable = ref_ht_common.nullable,
        .heap_type = ref_ht_common.heap_type,
    };

    if (p_is_mutable != NULL) {
        if (array_type->elem_flags & 1) {
            *p_is_mutable = true;
        }
        else {
            *p_is_mutable = false;
        }
    }

    return ref_type;
}

bool
wasm_defined_type_equal(WASMType *const def_type1, WASMType *const def_type2,
                        WASMModuleCommon *const module)
{
    /* TODO */
    return false;
}

bool
wasm_defined_type_is_subtype_of(WASMType *const def_type1,
                                WASMType *const def_type2,
                                WASMModuleCommon *const module)
{
    /* TODO */
    return false;
}

void
wasm_ref_type_set_type_idx(wasm_ref_type_t *ref_type, bool nullable,
                           int32 type_idx)
{
    /* TODO */
}

void
wasm_ref_type_set_heap_type(wasm_ref_type_t *ref_type, bool nullable,
                            int32 heap_type)
{
    /* TODO */
}

bool
wasm_ref_type_equal(const wasm_ref_type_t *ref_type1,
                    const wasm_ref_type_t *ref_type2,
                    WASMModuleCommon *const module)
{
    uint8 type1 = ref_type1->value_type;
    uint8 type2 = ref_type2->value_type;
    WASMTypePtr *types = ((WASMModule *)module)->types;
    uint32 type_count = wasm_get_defined_type_count(module);
    return wasm_reftype_equal(type1, (WASMRefType *)ref_type1, type2,
                              (WASMRefType *)ref_type2, types, type_count);
}

bool
wasm_ref_type_is_subtype_of(const wasm_ref_type_t *ref_type1,
                            const wasm_ref_type_t *ref_type2,
                            WASMModuleCommon *const module)
{
    uint8 type1 = ref_type1->value_type;
    uint8 type2 = ref_type2->value_type;
    WASMTypePtr *types = ((WASMModule *)module)->types;
    uint32 type_count = wasm_get_defined_type_count(module);
    return wasm_reftype_is_subtype_of(type1, (WASMRefType *)ref_type1, type2,
                                      (WASMRefType *)ref_type2, types,
                                      type_count);
}

WASMStructObjectRef
wasm_struct_obj_new_with_typeidx(WASMExecEnv *exec_env, uint32 type_idx)
{
    /* TODO */
    return NULL;
}

WASMStructObjectRef
wasm_struct_obj_new_with_type(WASMExecEnv *exec_env, WASMStructType *type)
{
    /* TODO */
    return NULL;
}

WASMArrayObjectRef
wasm_array_obj_new_with_typeidx(WASMExecEnv *exec_env, uint32 type_idx,
                                uint32 length, wasm_value_t *init_value)
{
    WASMModuleCommon *module = wasm_exec_env_get_module(exec_env);
    WASMModuleInstanceCommon *module_inst =
        wasm_exec_env_get_module_inst(exec_env);
    WASMModule *wasm_module = (WASMModule *)module;
    WASMType *defined_type = wasm_get_defined_type(module, type_idx);
    WASMRttTypeRef rtt_type;
    if (!(rtt_type = wasm_rtt_type_new(
              defined_type, type_idx, wasm_module->rtt_types,
              wasm_module->type_count, &wasm_module->rtt_type_lock))) {
        wasm_set_exception((WASMModuleInstance *)module_inst,
                           "create rtt type failed");
        goto got_exception;
    }
    rtt_type->type_flag = defined_type->type_flag;
    rtt_type->inherit_depth = defined_type->inherit_depth;
    rtt_type->defined_type = defined_type;
    rtt_type->root_type = defined_type->root_type;

    WASMArrayObjectRef array_obj =
        wasm_array_obj_new(exec_env, rtt_type, length, init_value);
    return array_obj;
got_exception:
    return NULL;
}

WASMArrayObjectRef
wasm_array_obj_new_with_type(WASMExecEnv *exec_env, WASMArrayType *type,
                             uint32 length, wasm_value_t *init_value)
{
    WASMRttTypeRef rtt_type = wasm_runtime_malloc(sizeof(WASMRttType));
    rtt_type->type_flag = type->type_flag;
    rtt_type->inherit_depth = type->inherit_depth;
    rtt_type->defined_type = (WASMType *)type;
    rtt_type->root_type = type->root_type;

    WASMArrayObjectRef array_obj =
        wasm_array_obj_new(exec_env, rtt_type, length, init_value);
    return array_obj;
}

WASMFuncObjectRef
wasm_func_obj_new_with_typeidx(WASMExecEnv *exec_env, uint32 type_idx,
                               uint32 func_idx_bound)
{
    WASMModuleCommon *module = wasm_exec_env_get_module(exec_env);
    WASMModuleInstanceCommon *module_inst =
        wasm_exec_env_get_module_inst(exec_env);
    WASMModule *wasm_module = (WASMModule *)module;
    WASMType *defined_type = wasm_get_defined_type(module, type_idx);
    WASMRttTypeRef rtt_type;
    if (!(rtt_type = wasm_rtt_type_new(
              defined_type, type_idx, wasm_module->rtt_types,
              wasm_module->type_count, &wasm_module->rtt_type_lock))) {
        wasm_set_exception((WASMModuleInstance *)module_inst,
                           "create rtt type failed");
        goto got_exception;
    }
    rtt_type->type_flag = defined_type->type_flag;
    rtt_type->inherit_depth = defined_type->inherit_depth;
    rtt_type->defined_type = defined_type;
    rtt_type->root_type = defined_type->root_type;

    WASMFuncObjectRef func_obj =
        wasm_func_obj_new(exec_env, rtt_type, func_idx_bound);
    return func_obj;
got_exception:
    return NULL;
}

WASMFuncObjectRef
wasm_func_obj_new_with_type(WASMExecEnv *exec_env, WASMFuncType *type,
                            uint32 func_idx_bound)
{
    WASMRttTypeRef rtt_type = wasm_runtime_malloc(sizeof(WASMRttType));
    rtt_type->type_flag = type->type_flag;
    rtt_type->inherit_depth = type->inherit_depth;
    rtt_type->defined_type = (WASMType *)type;
    rtt_type->root_type = type->root_type;

    WASMFuncObjectRef func_obj =
        wasm_func_obj_new(exec_env, rtt_type, func_idx_bound);
    return func_obj;
}

bool
wasm_obj_is_instance_of_defined_type(WASMObjectRef obj, WASMType *defined_type,
                                     WASMModuleCommon *const module)
{
    /* TODO */
    return false;
}

bool
wasm_obj_is_instance_of_type_idx(WASMObjectRef obj, uint32 type_idx,
                                 WASMModuleCommon *const module)
{
    /* TODO */
    return false;
}

bool
wasm_obj_is_instance_of_ref_type(const WASMObjectRef obj,
                                 const wasm_ref_type_t *ref_type)
{
    int32 heap_type = ref_type->heap_type;
    return wasm_obj_is_type_of(obj, heap_type);
}

void
wasm_runtime_push_local_object_ref(WASMExecEnv *exec_env,
                                   WASMLocalObjectRef *ref)
{
    ref->val = NULL;
    ref->prev = exec_env->cur_local_object_ref;
    exec_env->cur_local_object_ref = ref;
}

WASMLocalObjectRef *
wasm_runtime_pop_local_object_ref(WASMExecEnv *exec_env)
{
    WASMLocalObjectRef *local_ref = exec_env->cur_local_object_ref;
    exec_env->cur_local_object_ref = exec_env->cur_local_object_ref->prev;
    return local_ref;
}

void
wasm_runtime_pop_local_object_refs(WASMExecEnv *exec_env, uint32 n)
{
    bh_assert(n > 0);

    do {
        exec_env->cur_local_object_ref = exec_env->cur_local_object_ref->prev;
    } while (--n > 0);
}

void
wasm_runtime_gc_prepare(WASMExecEnv *exec_env)
{
#if 0
    /* TODO: implement wasm_runtime_gc_prepare for multi-thread */
    exec_env->is_gc_reclaiming = false;
    wasm_thread_suspend_all();
    exec_env->is_gc_reclaim = 1;
    exec_env->requesting_suspend = 0;
#endif
}

void
wasm_runtime_gc_finalize(WASMExecEnv *exec_env)
{
#if 0
    /* TODO: implement wasm_runtime_gc_finalize for multi-thread */
    wasm_thread_resume_all();
    exec_env->doing_gc_reclaim = 0;
#endif
}

bool
wasm_runtime_get_wasm_object_ref_list(WASMObjectRef obj,
                                      bool *p_is_compact_mode,
                                      uint32 *p_ref_num, uint16 **p_ref_list,
                                      uint32 *p_ref_start_offset)
{
    return wasm_object_get_ref_list(obj, p_is_compact_mode, p_ref_num,
                                    p_ref_list, p_ref_start_offset);
}

bool
wasm_runtime_traverse_gc_rootset(WASMExecEnv *exec_env, void *heap)
{
#if WASM_ENABLE_INTERP != 0
    if (exec_env->module_inst->module_type == Wasm_Module_Bytecode) {
        return wasm_traverse_gc_rootset(exec_env, heap);
    }
#endif
#if WASM_ENABLE_AOT != 0
    if (exec_env->module_inst->module_type == Wasm_Module_AoT) {
        /* TODO */
        /*return aot_traverse_gc_rootset(exec_env, heap);*/
    }
#endif
    return false;
}

void
wasm_runtime_set_gc_heap_handle(WASMModuleInstanceCommon *module_inst,
                                void *gc_heap_handle)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode)
        ((WASMModuleInstance *)module_inst)->e->gc_heap_handle = gc_heap_handle;
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT) {
        /* TODO */
        /*
        ((AOTModuleInstance *)module_inst)->e->gc_heap_handle.ptr =
        gc_heap_handle;
        */
    }
#endif
}

void *
wasm_runtime_get_gc_heap_handle(WASMModuleInstanceCommon *module_inst)
{
#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode)
        return ((WASMModuleInstance *)module_inst)->e->gc_heap_handle;
#endif
#if WASM_ENABLE_AOT != 0
    if (module_inst->module_type == Wasm_Module_AoT) {
        /* TODO */
        /*
        return ((AOTModuleInstance *)module_inst)->e->gc_heap_handle.ptr;
        */
    }
#endif
    return NULL;
}
