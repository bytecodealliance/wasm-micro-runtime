/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "gc_object.h"
#include "../wasm_runtime_common.h"

static uint32
rtt_obj_hash(const void *key)
{
    const WASMRttObjectRef rtt_obj = (const WASMRttObjectRef)key;

    return (uint32)(((uintptr_t)rtt_obj->defined_type)
                    ^ ((uintptr_t)rtt_obj->parent));
}

static bool
rtt_obj_equal(void *key1, void *key2)
{
    const WASMRttObjectRef rtt_obj1 = (const WASMRttObjectRef)key1;
    const WASMRttObjectRef rtt_obj2 = (const WASMRttObjectRef)key2;

    return (rtt_obj1->defined_type == rtt_obj2->defined_type
            && rtt_obj1->parent == rtt_obj2->parent)
               ? true
               : false;
}

HashMap *
wasm_rttobj_set_create(uint32 size)
{
    HashMap *rtt_obj_set = bh_hash_map_create(
        size, true, rtt_obj_hash, rtt_obj_equal, NULL, wasm_runtime_free);

    return rtt_obj_set;
}

WASMRttObjectRef
wasm_rtt_obj_new(HashMap *rtt_obj_set, WASMRttObject *parent,
                 WASMType *defined_type, uint32 defined_type_idx)
{
    WASMRttObject *rtt_obj, *rtt_obj_ret;

    if (!(rtt_obj = wasm_runtime_malloc(sizeof(WASMRttObject))))
        return NULL;

    rtt_obj->header = WASM_OBJ_RTT_OBJ_FLAG;
    rtt_obj->type_flag = defined_type->type_flag;
    rtt_obj->n = parent ? parent->n + 1 : 0;
    rtt_obj->defined_type_idx = defined_type_idx;
    rtt_obj->defined_type = defined_type;
    rtt_obj->root = parent ? parent->root : rtt_obj;
    rtt_obj->parent = parent;

    if ((rtt_obj_ret = bh_hash_map_find(rtt_obj_set, (void *)rtt_obj))) {
        wasm_runtime_free(rtt_obj);
        return rtt_obj_ret;
    }

    if (!bh_hash_map_insert(rtt_obj_set, rtt_obj, rtt_obj)) {
        wasm_runtime_free(rtt_obj);
        return NULL;
    }

    return rtt_obj;
}

static void *
gc_obj_malloc(void *heap_handle, uint64 size)
{
    void *mem;

    /* Allocate from global heap for test,
       TODO: changed to allocate from heap_handle */
    if (size >= UINT32_MAX || !(mem = wasm_runtime_malloc((uint32)size))) {
        return NULL;
    }

    memset(mem, 0, (uint32)size);
    return mem;
}

WASMStructObjectRef
wasm_struct_obj_new(void *heap_handle, WASMRttObjectRef rtt_obj)
{
    WASMStructObjectRef struct_obj;
    WASMStructType *struct_type;

    bh_assert(rtt_obj->type_flag == WASM_TYPE_STRUCT);

    struct_type = (WASMStructType *)rtt_obj->defined_type;
    if (!(struct_obj = gc_obj_malloc(heap_handle, struct_type->total_size))) {
        return NULL;
    }

    struct_obj->header = (WASMObjectHeader)rtt_obj;
    return struct_obj;
}

void
wasm_struct_obj_set_field(WASMStructObjectRef struct_obj, uint32 field_idx,
                          WASMValue *value)
{
    WASMRttObjectRef rtt_obj =
        (WASMRttObjectRef)wasm_object_header((WASMObjectRef)struct_obj);
    WASMStructType *struct_type = (WASMStructType *)rtt_obj->defined_type;
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
    WASMRttObjectRef rtt_obj =
        (WASMRttObjectRef)wasm_object_header((WASMObjectRef)struct_obj);
    WASMStructType *struct_type = (WASMStructType *)rtt_obj->defined_type;
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
wasm_array_obj_new(void *heap_handle, WASMRttObjectRef rtt_obj, uint32 length,
                   WASMValue *init_value)
{
    WASMArrayObjectRef array_obj;
    WASMArrayType *array_type;
    uint64 total_size;
    uint32 elem_size, elem_size_log, i;

    bh_assert(rtt_obj->type_flag == WASM_TYPE_ARRAY);

    if (length >= (1 << 29))
        return NULL;

    array_type = (WASMArrayType *)rtt_obj->defined_type;
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

    array_obj->header = (WASMObjectHeader)rtt_obj;
    array_obj->length = (length << 2) | elem_size_log;
    for (i = 0; i < length; i++) {
        if (wasm_is_type_reftype(array_type->elem_type)) {
            uint32 *elem_addr =
                (uint32 *)array_obj->elem_data + REF_CELL_NUM * i;
            PUT_REF_TO_ADDR(elem_addr, init_value->gc_obj);
        }
        else if (array_type->elem_type == VALUE_TYPE_I32
                 || array_type->elem_type == VALUE_TYPE_F32
                 || array_type->elem_type == PACKED_TYPE_I8
                 || array_type->elem_type == PACKED_TYPE_I16) {
            ((int32 *)array_obj->elem_data)[i] = init_value->i32;
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

WASMFuncObjectRef
wasm_func_obj_new(void *heap_handle, WASMRttObjectRef rtt_obj, uint32 func_idx)
{
    WASMFuncObjectRef func_obj;
    WASMFuncType *func_type;
    uint64 total_size;

    bh_assert(rtt_obj->type_flag == WASM_TYPE_FUNC);

    func_type = (WASMFuncType *)rtt_obj->defined_type;

    total_size =
        offsetof(WASMFuncObject, param_data) + func_type->total_param_size;
    if (!(func_obj = gc_obj_malloc(heap_handle, total_size))) {
        return NULL;
    }

    func_obj->header = (WASMObjectHeader)rtt_obj;
    func_obj->func_idx = func_idx;
    return func_obj;
}

void
wasm_func_obj_set_param(WASMFuncObjectRef func_obj, uint32 param_idx,
                        WASMValue *value)
{}

WASMValue *
wasm_func_obj_get_param(const WASMFuncObjectRef func_obj, uint32 param_idx)
{
    return NULL;
}

WASMExternrefObjectRef
wasm_externref_obj_new(void *heap_handle, void *foreign_obj)
{
    WASMExternrefObjectRef externref_obj;

    if (!(externref_obj =
              gc_obj_malloc(heap_handle, sizeof(WASMExternrefObject)))) {
        return NULL;
    }

    externref_obj->header = WASM_OBJ_EXTERNREF_OBJ_FLAG;
    externref_obj->foreign_obj = foreign_obj;
    return externref_obj;
}

bool
wasm_obj_is_instance_of(WASMObjectRef obj, WASMRttObjectRef rtt_obj)
{
    WASMRttObjectRef rtt_obj_sub;

    bh_assert(obj);

    if (wasm_obj_is_i31_rtt_or_externref_obj(obj))
        return false;

    rtt_obj_sub = (WASMRttObjectRef)wasm_object_header(obj);

    return wasm_rtt_obj_is_subtype_of(rtt_obj_sub, rtt_obj);
}

bool
wasm_obj_equal(WASMObjectRef obj1, WASMObjectRef obj2)
{
    /* TODO: do we need to compare the internal details of the objects */
    return obj1 == obj2 ? true : false;
}
