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
wasm_defined_type_is_func_type(WASMType *const def_type1)
{
    /* TODO */
    return false;
}

bool
wasm_defined_type_is_struct_type(WASMType *const def_type1)
{
    /* TODO */
    return false;
}

bool
wasm_defined_type_is_array_type(WASMType *const def_type1)
{
    /* TODO */
    return false;
}

uint32
wasm_func_type_get_param_count(WASMFuncType *const func_type)
{
    /* TODO */
    return 0;
}

wasm_ref_type_t
wasm_func_type_get_param_type(WASMFuncType *const func_type, uint32 param_idx)
{
    wasm_ref_type_t ref_type = { 0 };
    /* TODO */
    return ref_type;
}

uint32
wasm_func_type_get_result_count(WASMFuncType *const func_type)
{
    /* TODO */
    return 0;
}

wasm_ref_type_t
wasm_func_type_get_result_type(WASMFuncType *const func_type, uint32 param_idx)
{
    wasm_ref_type_t ref_type = { 0 };
    /* TODO */
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
    wasm_ref_type_t ref_type = { 0 };
    /* TODO */
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
    /* TODO */
    return false;
}

bool
wasm_ref_type_is_subtype_of(const wasm_ref_type_t *ref_type1,
                            const wasm_ref_type_t *ref_type2,
                            WASMModuleCommon *const module)
{
    /* TODO */
    return false;
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
wasm_array_obj_new_with_typeidx(WASMExecEnv *exec_env, uint32 type_idx)
{
    /* TODO */
    return NULL;
}

WASMArrayObjectRef
wasm_array_obj_new_with_type(WASMExecEnv *exec_env, WASMArrayType *type)
{
    /* TODO */
    return NULL;
}

WASMFuncObjectRef
wasm_func_obj_new_with_typeidx(WASMExecEnv *exec_env, uint32 type_idx)
{
    /* TODO */
    return NULL;
}

WASMFuncObjectRef
wasm_func_obj_new_with_type(WASMExecEnv *exec_env, WASMFuncType *type)
{
    /* TODO */
    return NULL;
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
    /* TODO */
    return false;
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
