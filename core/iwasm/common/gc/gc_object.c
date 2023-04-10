/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "gc_object.h"
#include "mem_alloc.h"
#include "../wasm_runtime_common.h"

WASMRttTypeRef
wasm_rtt_type_new(WASMType *defined_type, uint32 defined_type_idx,
                  WASMRttType **rtt_types, uint32 rtt_type_count,
                  korp_mutex *rtt_type_lock)
{
    WASMRttType *rtt_type;

    bh_assert(defined_type_idx < rtt_type_count);

    os_mutex_lock(rtt_type_lock);

    if (rtt_types[defined_type_idx]) {
        os_mutex_unlock(rtt_type_lock);
        return rtt_types[defined_type_idx];
    }

    if ((rtt_type = wasm_runtime_malloc(sizeof(WASMRttType)))) {
        rtt_type->type_flag = defined_type->type_flag;
        rtt_type->inherit_depth = defined_type->inherit_depth;
        rtt_type->defined_type = defined_type;
        rtt_type->root_type = defined_type->root_type;

        rtt_types[defined_type_idx] = rtt_type;
    }

    os_mutex_unlock(rtt_type_lock);
    return rtt_type;
}

static void *
gc_obj_malloc(void *heap_handle, uint64 size)
{
    void *mem;

    if (size >= UINT32_MAX
        || !(mem = mem_allocator_malloc_with_gc(heap_handle, (uint32)size))) {
        LOG_WARNING("warning: failed to allocate memory for gc object");
        return NULL;
    }

    memset(mem, 0, (uint32)size);
    return mem;
}

WASMStructObjectRef
wasm_struct_obj_new(void *heap_handle, WASMRttTypeRef rtt_type)
{
    WASMStructObjectRef struct_obj;
    WASMStructType *struct_type;

    bh_assert(rtt_type->type_flag == WASM_TYPE_STRUCT);

    struct_type = (WASMStructType *)rtt_type->defined_type;
    if (!(struct_obj = gc_obj_malloc(heap_handle, struct_type->total_size))) {
        return NULL;
    }

    struct_obj->header = (WASMObjectHeader)rtt_type;

    return struct_obj;
}

void
wasm_struct_obj_set_field(WASMStructObjectRef struct_obj, uint32 field_idx,
                          WASMValue *value)
{
    WASMRttTypeRef rtt_type =
        (WASMRttTypeRef)wasm_object_header((WASMObjectRef)struct_obj);
    WASMStructType *struct_type = (WASMStructType *)rtt_type->defined_type;
    WASMStructFieldType *field;
    uint8 field_size, *field_data;

    bh_assert(field_idx < struct_type->field_count);

    field = struct_type->fields + field_idx;
    field_data = (uint8 *)struct_obj + field->field_offset;
    field_size = field->field_size;

    if (field_size == 4) {
        *(int32 *)field_data = value->i32;
    }
    else if (field_size == 8) {
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64) \
    || defined(BUILD_TARGET_X86_32)
        *(int64 *)field_data = value->i64;
#else
        PUT_I64_TO_ADDR((uint32 *)field_data, value->i64);
#endif
    }
    else if (field_size == 1) {
        *(int8 *)field_data = (int8)value->i32;
    }
    else if (field_size == 2) {
        *(int16 *)field_data = (int16)value->i32;
    }
    else {
        bh_assert(0);
    }
}

void
wasm_struct_obj_get_field(const WASMStructObjectRef struct_obj,
                          uint32 field_idx, bool sign_extend, WASMValue *value)
{
    WASMRttTypeRef rtt_type =
        (WASMRttTypeRef)wasm_object_header((WASMObjectRef)struct_obj);
    WASMStructType *struct_type = (WASMStructType *)rtt_type->defined_type;
    WASMStructFieldType *field;
    uint8 *field_data, field_size;

    bh_assert(field_idx < struct_type->field_count);

    field = struct_type->fields + field_idx;
    field_data = (uint8 *)struct_obj + field->field_offset;
    field_size = field->field_size;

    if (field_size == 4) {
        value->i32 = *(int32 *)field_data;
    }
    else if (field_size == 8) {
#if defined(BUILD_TARGET_X86_64) || defined(BUILD_TARGET_AMD_64) \
    || defined(BUILD_TARGET_X86_32)
        value->i64 = *(int64 *)field_data;
#else
        value->i64 = GET_I64_FROM_ADDR((uint32 *)field_data);
#endif
    }
    else if (field_size == 1) {
        if (sign_extend)
            value->i32 = (int32)(*(int8 *)field_data);
        else
            value->u32 = (uint32)(*(uint8 *)field_data);
    }
    else if (field_size == 2) {
        if (sign_extend)
            value->i32 = (int32)(*(int8 *)field_data);
        else
            value->u32 = (uint32)(*(uint8 *)field_data);
    }
    else {
        bh_assert(0);
    }
}

WASMArrayObjectRef
wasm_array_obj_new(void *heap_handle, WASMRttTypeRef rtt_type, uint32 length,
                   WASMValue *init_value)
{
    WASMArrayObjectRef array_obj;
    WASMArrayType *array_type;
    uint64 total_size;
    uint32 elem_size, elem_size_log, i;

    bh_assert(rtt_type->type_flag == WASM_TYPE_ARRAY);

    if (length >= (1 << 29))
        return NULL;

    array_type = (WASMArrayType *)rtt_type->defined_type;
    if (array_type->elem_type == PACKED_TYPE_I8) {
        elem_size = 1;
        elem_size_log = 0;
    }
    else if (array_type->elem_type == PACKED_TYPE_I16) {
        elem_size = 2;
        elem_size_log = 1;
    }
    else {
        elem_size = wasm_value_type_size(array_type->elem_type);
        elem_size_log = (elem_size == 4) ? 2 : 3;
    }

    total_size =
        offsetof(WASMArrayObject, elem_data) + (uint64)elem_size * length;
    if (!(array_obj = gc_obj_malloc(heap_handle, total_size))) {
        return NULL;
    }

    array_obj->header = (WASMObjectHeader)rtt_type;
    array_obj->length = (length << 2) | elem_size_log;
    for (i = 0; i < length; i++) {
        if (wasm_is_type_reftype(array_type->elem_type)) {
            uint32 *elem_addr =
                (uint32 *)array_obj->elem_data + REF_CELL_NUM * i;
            PUT_REF_TO_ADDR(elem_addr, init_value->gc_obj);
        }
        else if (array_type->elem_type == VALUE_TYPE_I32
                 || array_type->elem_type == VALUE_TYPE_F32) {
            ((int32 *)array_obj->elem_data)[i] = init_value->i32;
        }
        else if (array_type->elem_type == PACKED_TYPE_I8) {
            ((int8 *)array_obj->elem_data)[i] = (int8)init_value->i32;
        }
        else if (array_type->elem_type == PACKED_TYPE_I16) {
            ((int16 *)array_obj->elem_data)[i] = (int16)init_value->i32;
        }
        else {
            uint32 *elem_addr = (uint32 *)array_obj->elem_data + 2 * i;
            PUT_I64_TO_ADDR(elem_addr, init_value->i64);
        }
    }

    return array_obj;
}

void
wasm_array_obj_set_elem(WASMArrayObjectRef array_obj, uint32 elem_idx,
                        WASMValue *value)
{
    uint8 *elem_data = wasm_array_obj_elem_addr(array_obj, elem_idx);
    uint32 elem_size = 1 << wasm_array_obj_elem_size_log(array_obj);

    switch (elem_size) {
        case 1:
            *(int8 *)elem_data = (int8)value->i32;
            break;
        case 2:
            *(int16 *)elem_data = (int16)value->i32;
            break;
        case 4:
            *(int32 *)elem_data = value->i32;
            break;
        case 8:
            PUT_I64_TO_ADDR((uint32 *)elem_data, value->i64);
            break;
    }
}

void
wasm_array_obj_get_elem(WASMArrayObjectRef array_obj, uint32 elem_idx,
                        bool sign_extend, WASMValue *value)
{
    uint8 *elem_data = wasm_array_obj_elem_addr(array_obj, elem_idx);
    uint32 elem_size = 1 << wasm_array_obj_elem_size_log(array_obj);

    switch (elem_size) {
        case 1:
            value->i32 = sign_extend ? (int32)(*(int8 *)elem_data)
                                     : (int32)(uint32)(*(uint8 *)elem_data);
            break;
        case 2:
            value->i32 = sign_extend ? (int32)(*(int16 *)elem_data)
                                     : (int32)(uint32)(*(uint16 *)elem_data);
            break;
        case 4:
            value->i32 = *(int32 *)elem_data;
            break;
        case 8:
            value->i64 = GET_I64_FROM_ADDR((uint32 *)elem_data);
            break;
    }
}

void
wasm_array_obj_copy(WASMArrayObjectRef dst_obj, uint32 dst_idx,
                    WASMArrayObjectRef src_obj, uint32 src_idx, uint32 len)
{
    uint8 *dst_data = wasm_array_obj_elem_addr(dst_obj, dst_idx);
    uint8 *src_data = wasm_array_obj_elem_addr(src_obj, src_idx);
    uint32 elem_size = 1 << wasm_array_obj_elem_size_log(dst_obj);

    bh_memmove_s(dst_data, elem_size * len, src_data, elem_size * len);
}

WASMFuncObjectRef
wasm_func_obj_new(void *heap_handle, WASMRttTypeRef rtt_type,
                  uint32 func_idx_bound)
{
    WASMFuncObjectRef func_obj;
    uint64 total_size;

    bh_assert(rtt_type->type_flag == WASM_TYPE_FUNC);

    total_size = sizeof(WASMFuncObject);
    if (!(func_obj = gc_obj_malloc(heap_handle, total_size))) {
        return NULL;
    }

    func_obj->header = (WASMObjectHeader)rtt_type;
    func_obj->func_idx_bound = func_idx_bound;

    return func_obj;
}

WASMExternrefObjectRef
wasm_externref_obj_new(WASMExecEnv *exec_env, void *heap_handle, void *host_obj)
{
    WASMAnyrefObjectRef anyref_obj;
    WASMExternrefObjectRef externref_obj;
    WASMLocalObjectRef local_ref;

    if (!(anyref_obj = gc_obj_malloc(heap_handle, sizeof(WASMAnyrefObject)))) {
        return NULL;
    }

    anyref_obj->header = WASM_OBJ_ANYREF_OBJ_FLAG;
    anyref_obj->host_obj = host_obj;

    /* Lock anyref_obj in case it is reclaimed when allocating memory below */
    wasm_runtime_push_local_object_ref(exec_env, &local_ref);
    local_ref.val = (WASMObjectRef)anyref_obj;

    if (!(externref_obj =
              gc_obj_malloc(heap_handle, sizeof(WASMExternrefObject)))) {
        wasm_runtime_pop_local_object_ref(exec_env);
        return NULL;
    }

    externref_obj->header = WASM_OBJ_EXTERNREF_OBJ_FLAG;
    externref_obj->internal_obj = (WASMObjectRef)anyref_obj;

    wasm_runtime_pop_local_object_ref(exec_env);
    return externref_obj;
}

WASMAnyrefObjectRef
wasm_anyref_obj_new(WASMExecEnv *exec_env, void *heap_handle, void *host_obj)
{
    WASMAnyrefObjectRef anyref_obj;

    if (!(anyref_obj = gc_obj_malloc(heap_handle, sizeof(WASMAnyrefObject)))) {
        return NULL;
    }

    anyref_obj->header = WASM_OBJ_ANYREF_OBJ_FLAG;
    anyref_obj->host_obj = host_obj;

    return anyref_obj;
}

WASMObjectRef
wasm_externref_obj_to_internal_obj(WASMExternrefObjectRef externref_obj)
{
    return externref_obj->internal_obj;
}

WASMExternrefObjectRef
wasm_internal_obj_to_externref_obj(void *heap_handle,
                                   WASMObjectRef internal_obj)
{
    WASMExternrefObjectRef externref_obj;

    if (!(externref_obj =
              gc_obj_malloc(heap_handle, sizeof(WASMExternrefObject)))) {
        return NULL;
    }

    externref_obj->header = WASM_OBJ_EXTERNREF_OBJ_FLAG;
    externref_obj->internal_obj = internal_obj;

    return externref_obj;
}

bool
wasm_obj_is_created_from_heap(WASMObjectRef obj)
{
    if (obj == NULL)
        return false;

    if (wasm_obj_is_i31_obj(obj))
        return false;

    /* struct/array/func/externref/anyref object */
    return true;
}

bool
wasm_obj_is_instance_of(WASMObjectRef obj, uint32 type_idx, WASMType **types,
                        uint32 type_count)
{
    WASMRttTypeRef rtt_type_sub;
    WASMType *type_sub, *type_parent;
    uint32 distance, i;

    bh_assert(obj);
    bh_assert(type_idx < type_count);

    if (wasm_obj_is_i31_externref_or_anyref_obj(obj))
        return false;

    rtt_type_sub = (WASMRttTypeRef)wasm_object_header(obj);
    type_parent = types[type_idx];

    if (!(rtt_type_sub->root_type == type_parent->root_type
          && rtt_type_sub->inherit_depth >= type_parent->inherit_depth))
        return false;

    type_sub = rtt_type_sub->defined_type;
    distance = type_sub->inherit_depth - type_parent->inherit_depth;

    for (i = 0; i < distance; i++) {
        type_sub = type_sub->parent_type;
    }

    return (type_sub == type_parent) ? true : false;
}

bool
wasm_obj_is_type_of(WASMObjectRef obj, int32 heap_type)
{
    bh_assert(obj);

    switch (heap_type) {
        case HEAP_TYPE_FUNC:
            return wasm_obj_is_func_obj(obj);
        case HEAP_TYPE_EXTERN:
            return wasm_obj_is_externref_obj(obj);
        case HEAP_TYPE_ANY:
            return wasm_obj_is_internal_obj(obj);
        case HEAP_TYPE_EQ:
            return wasm_obj_is_eq_obj(obj);
        case HEAP_TYPE_I31:
            return wasm_obj_is_i31_obj(obj);
        case HEAP_TYPE_STRUCT:
            return wasm_obj_is_struct_obj(obj);
        case HEAP_TYPE_ARRAY:
            return wasm_obj_is_array_obj(obj);
        case HEAP_TYPE_NONE:
        case HEAP_TYPE_NOFUNC:
        case HEAP_TYPE_NOEXTERN:
            return false;
        default:
            bh_assert(0);
            break;
    }
    return false;
}

bool
wasm_obj_equal(WASMObjectRef obj1, WASMObjectRef obj2)
{
    /* TODO: do we need to compare the internal details of the objects */
    return obj1 == obj2 ? true : false;
}

bool
wasm_object_get_ref_list(WASMObjectRef obj, bool *p_is_compact_mode,
                         uint32 *p_ref_num, uint16 **p_ref_list,
                         uint32 *p_ref_start_offset)
{
    WASMRttTypeRef rtt_type;

    bh_assert(wasm_obj_is_created_from_heap(obj));

    rtt_type = (WASMRttTypeRef)wasm_object_header(obj);

    if (obj->header & WASM_OBJ_EXTERNREF_OBJ_FLAG) {
        /* externref object */
        static uint16 externref_obj_ref_list[] = { (uint16)offsetof(
            WASMExternrefObject, internal_obj) };
        *p_is_compact_mode = false;
        *p_ref_num = 0;
        *p_ref_list = externref_obj_ref_list;
        return true;
    }
    else if (obj->header & WASM_OBJ_ANYREF_OBJ_FLAG) {
        /* anyref object */
        *p_is_compact_mode = false;
        *p_ref_num = 0;
        *p_ref_list = NULL;
        return true;
    }
    else if (rtt_type->defined_type->type_flag == WASM_TYPE_FUNC) {
        /* function object */
        *p_is_compact_mode = false;
        *p_ref_num = 0;
        *p_ref_list = NULL;
        return true;
    }
    else if (rtt_type->defined_type->type_flag == WASM_TYPE_STRUCT) {
        /* struct object */
        WASMStructType *type = (WASMStructType *)rtt_type->defined_type;
        *p_is_compact_mode = false;
        *p_ref_num = *type->reference_table;
        *p_ref_list = type->reference_table + 1;
        return true;
    }
    else if (rtt_type->defined_type->type_flag == WASM_TYPE_ARRAY) {
        /* array object */
        *p_is_compact_mode = true;
        *p_ref_num = (uint16)wasm_array_obj_length((WASMArrayObjectRef)obj);
        *p_ref_start_offset = (uint16)offsetof(WASMArrayObject, elem_data);
        return true;
    }
    else {
        bh_assert(0);
        return false;
    }
}
