/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "../wasm_runtime_common.h"
#include "gc_export.h"
#include "gc_object.h"
#if WASM_ENABLE_INTERP != 0
#include "../interpreter/wasm_runtime.h"
#endif
#if WASM_ENABLE_AOT != 0
#include "../aot/aot_runtime.h"
#endif

uint32
wasm_get_defined_type_count(WASMModuleCommon *const module)
{
    uint32 type_count = 0;

#if WASM_ENABLE_INTERP != 0
    if (module->module_type == Wasm_Module_Bytecode) {
        WASMModule *wasm_module = (WASMModule *)module;
        type_count = wasm_module->type_count;
    }
#endif
#if WASM_ENABLE_AOT != 0
    // TODO
#endif

    return type_count;
}

WASMType *
wasm_get_defined_type(WASMModuleCommon *const module, uint32 index)
{
    WASMType *type = NULL;

#if WASM_ENABLE_INTERP != 0
    if (module->module_type == Wasm_Module_Bytecode) {
        WASMModule *wasm_module = (WASMModule *)module;

        bh_assert(index < wasm_module->type_count);
        type = wasm_module->types[index];
    }
#endif
#if WASM_ENABLE_AOT != 0
    // TODO
#endif

    return type;
}

bool
wasm_defined_type_is_func_type(WASMType *const def_type)
{
    /* TODO */
    return false;
}

bool
wasm_defined_type_is_struct_type(WASMType *const def_type)
{
    return wasm_type_is_struct_type(def_type);
}

bool
wasm_defined_type_is_array_type(WASMType *const def_type)
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

wasm_ref_type_t *
wasm_ref_type_normalize(wasm_ref_type_t *ref_type)
{
    int32 heap_type;

    if (ref_type->value_type != REF_TYPE_HT_NULLABLE) {
        return ref_type;
    }
    heap_type = ref_type->heap_type;

    if (heap_type >= HEAP_TYPE_NONE && heap_type <= HEAP_TYPE_FUNC) {
        ref_type->value_type =
            (uint8)(REF_TYPE_NULLREF + heap_type - HEAP_TYPE_NONE);
        ref_type->nullable = false;
        ref_type->heap_type = 0;
    }
    else {
        ref_type->nullable = true;
    }

    return ref_type;
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
    return struct_type->field_count;
}

wasm_ref_type_t
wasm_struct_type_get_field_type(WASMStructType *const struct_type,
                                uint32 field_idx, bool *p_is_mutable)
{
    wasm_ref_type_t ref_type = { 0 };
    WASMRefTypeMap *ref_type_maps;
    WASMStructFieldType field;
    uint32 i;
    uint32 ref_type_map_count;

    bh_assert(field_idx < struct_type->field_count);

    field = struct_type->fields[field_idx];
    ref_type_maps = struct_type->ref_type_maps;
    ref_type_map_count = struct_type->ref_type_map_count;

    for (i = 0; i < ref_type_map_count; i++) {
        if (ref_type_maps[i].index == field_idx) {
            WASMRefType *field_ref_type =
                struct_type->ref_type_maps[i].ref_type;

            ref_type.nullable = field_ref_type->ref_ht_common.nullable;
            ref_type.heap_type = field_ref_type->ref_ht_common.heap_type;
            break;
        }
    }

    ref_type.value_type = field.field_type;
    *p_is_mutable = field.field_flags & 1;

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
    WASMTypePtr *types = NULL;
    uint32 type_count = 0;

#if WASM_ENABLE_INTERP != 0
    if (module->module_type == Wasm_Module_Bytecode) {
        WASMModule *wasm_module = (WASMModule *)module;

        types = wasm_module->types;
        type_count = wasm_module->type_count;
    }
#endif
#if WASM_ENABLE_AOT != 0
    // TODO
#endif

    return wasm_type_equal(def_type1, def_type2, types, type_count);
}

bool
wasm_defined_type_is_subtype_of(WASMType *const def_type1,
                                WASMType *const def_type2,
                                WASMModuleCommon *const module)
{
    WASMTypePtr *types = NULL;
    uint32 type_count = 0;

#if WASM_ENABLE_INTERP != 0
    if (module->module_type == Wasm_Module_Bytecode) {
        WASMModule *wasm_module = (WASMModule *)module;

        types = wasm_module->types;
        type_count = wasm_module->type_count;
    }
#endif
#if WASM_ENABLE_AOT != 0
    // TODO
#endif

    return wasm_type_is_subtype_of(def_type1, def_type2, types, type_count);
}

void
wasm_ref_type_set_type_idx(wasm_ref_type_t *ref_type, bool nullable,
                           int32 type_idx)
{
    ref_type->value_type =
        nullable ? VALUE_TYPE_HT_NULLABLE_REF : VALUE_TYPE_HT_NON_NULLABLE_REF;
    ref_type->nullable = nullable;
    ref_type->heap_type = type_idx;
}

void
wasm_ref_type_set_heap_type(wasm_ref_type_t *ref_type, bool nullable,
                            int32 heap_type)
{
    bh_assert(heap_type <= HEAP_TYPE_FUNC && heap_type >= HEAP_TYPE_NONE);
    ref_type->value_type =
        nullable ? VALUE_TYPE_HT_NULLABLE_REF : VALUE_TYPE_HT_NON_NULLABLE_REF;
    ref_type->nullable = nullable;
    ref_type->heap_type = heap_type;

    ref_type = wasm_ref_type_normalize(ref_type);
}

bool
wasm_ref_type_equal(const wasm_ref_type_t *ref_type1,
                    const wasm_ref_type_t *ref_type2,
                    WASMModuleCommon *const module)
{
    wasm_ref_type_t ref_type1_norm = { 0 };
    wasm_ref_type_t ref_type2_norm = { 0 };
    uint32 type_count = 0;
    WASMTypePtr *types = NULL;
    uint8 type1;
    uint8 type2;

    bh_memcpy_s(&ref_type1_norm, (uint32)sizeof(wasm_ref_type_t), ref_type1,
                (uint32)sizeof(wasm_ref_type_t));
    bh_memcpy_s(&ref_type2_norm, (uint32)sizeof(wasm_ref_type_t), ref_type2,
                (uint32)sizeof(wasm_ref_type_t));
    type1 = ref_type1_norm.value_type;
    type2 = ref_type2_norm.value_type;

#if WASM_ENABLE_INTERP != 0
    if (module->module_type == Wasm_Module_Bytecode) {
        types = ((WASMModule *)module)->types;
        type_count = wasm_get_defined_type_count(module);
    }
#endif
#if WASM_ENABLE_AOT != 0
    // TODO
#endif

    return wasm_reftype_equal(type1, (WASMRefType *)&ref_type1_norm, type2,
                              (WASMRefType *)&ref_type2_norm, types,
                              type_count);
}

bool
wasm_ref_type_is_subtype_of(const wasm_ref_type_t *ref_type1,
                            const wasm_ref_type_t *ref_type2,
                            WASMModuleCommon *const module)
{
    wasm_ref_type_t ref_type1_norm = { 0 };
    wasm_ref_type_t ref_type2_norm = { 0 };
    uint8 type1;
    uint8 type2;
    WASMTypePtr *types = NULL;
    uint32 type_count = 0;

    bh_memcpy_s(&ref_type1_norm, (uint32)sizeof(wasm_ref_type_t), ref_type1,
                (uint32)sizeof(wasm_ref_type_t));
    bh_memcpy_s(&ref_type2_norm, (uint32)sizeof(wasm_ref_type_t), ref_type2,
                (uint32)sizeof(wasm_ref_type_t));
    type1 = ref_type1_norm.value_type;
    type2 = ref_type2_norm.value_type;

#if WASM_ENABLE_INTERP != 0
    if (module->module_type == Wasm_Module_Bytecode) {
        types = ((WASMModule *)module)->types;
        type_count = wasm_get_defined_type_count(module);
    }
#endif
#if WASM_ENABLE_AOT != 0
    // TODO
#endif

    return wasm_reftype_is_subtype_of(type1, (WASMRefType *)&ref_type1_norm,
                                      type2, (WASMRefType *)&ref_type2_norm,
                                      types, type_count);
}

WASMStructObjectRef
wasm_struct_obj_new_with_typeidx(WASMExecEnv *exec_env, uint32 type_idx)
{
    WASMStructObjectRef struct_obj;
    WASMModuleInstanceCommon *module_inst =
        wasm_runtime_get_module_inst(exec_env);
    WASMType *type = NULL;
    WASMRttTypeRef rtt_type = NULL;

#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        WASMModule *module = ((WASMModuleInstance *)module_inst)->module;

        bh_assert(type_idx < module->type_count);
        type = module->types[type_idx];
        bh_assert(wasm_defined_type_is_struct_type(type));
        rtt_type =
            wasm_rtt_type_new(type, type_idx, module->rtt_types,
                              module->type_count, &module->rtt_type_lock);
    }
#endif
#if WASM_ENABLE_AOT != 0
    // TODO
#endif

    bh_assert(rtt_type);
    struct_obj = wasm_struct_obj_new(exec_env, rtt_type);

    return struct_obj;
}

WASMStructObjectRef
wasm_struct_obj_new_with_type(WASMExecEnv *exec_env, WASMStructType *type)
{
    WASMStructObjectRef struct_obj;
    WASMModuleInstanceCommon *module_inst =
        wasm_runtime_get_module_inst(exec_env);
    WASMRttTypeRef rtt_type = NULL;

#if WASM_ENABLE_INTERP != 0
    if (module_inst->module_type == Wasm_Module_Bytecode) {
        WASMModule *module = ((WASMModuleInstance *)module_inst)->module;
        uint32 type_count = module->type_count;

        for (uint32 i = 0; i < type_count; i++) {
            if (module->types[i]->type_flag != WASM_TYPE_STRUCT) {
                continue;
            }
            if (module->types[i] == (WASMType *)type) {
                rtt_type = wasm_rtt_type_new(
                    (WASMType *)type, i, module->rtt_types, module->type_count,
                    &module->rtt_type_lock);
                break;
            }
        }
    }
#endif
#if WASM_ENABLE_AOT != 0
    // TODO
#endif

    bh_assert(rtt_type);
    struct_obj = wasm_struct_obj_new(exec_env, rtt_type);

    return struct_obj;
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
    WASMType **types = NULL;
    uint32 type_count = 0;
    uint32 type_idx = 0;

#if WASM_ENABLE_INTERP != 0
    if (module->module_type == Wasm_Module_Bytecode) {
        WASMModule *wasm_module = (WASMModule *)module;

        type_count = wasm_module->type_count;
        types = wasm_module->types;
    }
#endif
#if WASM_ENABLE_AOT != 0
    // TODO
#endif

    for (type_idx = 0; type_idx < type_count; type_idx++) {
        if (types[type_idx] == defined_type) {
            break;
        }
    }
    bh_assert(type_idx < type_count);

    return wasm_obj_is_instance_of(obj, type_idx, types, type_count);
}

bool
wasm_obj_is_instance_of_type_idx(WASMObjectRef obj, uint32 type_idx,
                                 WASMModuleCommon *const module)
{
    WASMType **types = NULL;
    uint32 type_count = 0;

#if WASM_ENABLE_INTERP != 0
    if (module->module_type == Wasm_Module_Bytecode) {
        WASMModule *wasm_module = (WASMModule *)module;

        types = wasm_module->types;
    }
#endif
#if WASM_ENABLE_AOT != 0
    // TODO
#endif

    return wasm_obj_is_instance_of(obj, type_idx, types, type_count);
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
