/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _GC_OBJECT_H_
#define _GC_OBJECT_H_

#include "gc_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Object header of a WASM object, as the adddress of allocated memory
 * must be 8-byte aligned, the lowest 3 bits are zero, we use them to
 * mark the object:
 *   bits[0] is 1: the object is weak referenced
 *   bits[1] is 1: it is an rtt object
 *   bits[2] is 1: it is an externref object
 *   if both bits[1] and bits[2] are 0, then this object header must
 *   be a pointer of a WASMRttObject, denotes that the object is a
 *   struct object, or an array object, or a function object
 */
typedef uintptr_t WASMObjectHeader;

#define WASM_OBJ_HEADER_MASK (~((uintptr_t)7))

#define WASM_OBJ_WEAK_REF_FLAG (((uintptr_t)1) << 0)

#define WASM_OBJ_RTT_OBJ_FLAG (((uintptr_t)1) << 1)

#define WASM_OBJ_EXTERNREF_OBJ_FLAG (((uintptr_t)1) << 2)

/* Representation of WASM objects */
typedef struct WASMObject {
    WASMObjectHeader header;
} WASMObject, *WASMObjectRef;

/* Representation of WASM rtt objects */
typedef struct WASMRttObject {
    /* bits[1] must be 1, denotes an rtt object */
    WASMObjectHeader header;
    /* type_flag must be WASM_TYPE_FUNC/STRUCT/ARRAY to
       denote an object of func, struct or array */
    uint16 type_flag;
    /* The inheritance depth */
    uint16 n;
    uint32 defined_type_idx;
    WASMType *defined_type;
    /* The root of the super rtt objects, whose parent is NULL */
    struct WASMRttObject *root;
    struct WASMRttObject *parent;
} WASMRttObject, *WASMRttObjectRef;

/* Representation of WASM externref objects */
typedef struct WASMExternrefObject {
    /* bits[2] must be 1, denotes an externref object */
    WASMObjectHeader header;
    /* The foreign object passed from host */
    void *foreign_obj;
} WASMExternrefObject, *WASMExternrefObjectRef;

/**
 * Representation of WASM i31 objects, the lowest bit is 1:
 * for a pointer of WASMObject, if the lowest bit is 1, then it is an
 * i31 object and bits[1..31] stores the actual i31 value, otherwise
 * it is a normal object of rtt/externref/struct/array/func */
typedef uintptr_t WASMI31ObjectRef;

/* Representation of WASM struct objects */
typedef struct WASMStructObject {
    /* Must be pointer of WASMRttObject of struct type */
    WASMObjectHeader header;
    uint8 field_data[1];
} WASMStructObject, *WASMStructObjectRef;

/* Representation of WASM array objects */
typedef struct WASMArrayObject {
    /* Must be pointer of WASMRttObject of array type */
    WASMObjectHeader header;
    /* (<array length> << 2) | <array element size>,
     * elem_count = lenght >> 2
     * elem_size = 2 ^ (length & 0x3)
     */
    uint32 length;
    uint8 elem_data[1];
} WASMArrayObject, *WASMArrayObjectRef;

#define WASM_ARRAY_LENGTH_SHIFT 2
#define WASM_ARRAY_ELEM_SIZE_MASK 3

/* Representation of WASM function objects */
typedef struct WASMFuncObject {
    /* must be pointer of WASMRttObject of array type */
    WASMObjectHeader header;
    uint32 func_idx;
    uint8 *param_data[1];
} WASMFuncObject, *WASMFuncObjectRef;

inline static WASMObjectHeader
wasm_object_header(const WASMObjectRef obj)
{
    return (obj->header & WASM_OBJ_HEADER_MASK);
}

HashMap *
wasm_rttobj_set_create(uint32 size);

WASMRttObjectRef
wasm_rtt_obj_new(HashMap *rtt_obj_set, WASMRttObject *parent,
                 WASMType *defined_type, uint32 defined_type_idx);

inline static WASMRttObjectRef
wasm_rtt_obj_get_parent(const WASMRttObjectRef rtt_obj)
{
    return rtt_obj->parent;
}

inline static WASMType *
wasm_rtt_obj_get_defined_type(const WASMRttObjectRef rtt_obj)
{
    return rtt_obj->defined_type;
}

inline static uint32
wasm_rtt_obj_get_defined_type_idx(const WASMRttObjectRef rtt_obj)
{
    return rtt_obj->defined_type_idx;
}

inline static uint32
wasm_rtt_obj_get_inherit_depth(const WASMRttObjectRef rtt_obj)
{
    return rtt_obj->n;
}

inline static bool
wasm_rtt_obj_is_subtype_of(const WASMRttObjectRef rtt_obj1,
                           const WASMRttObjectRef rtt_obj2)
{
    return (rtt_obj1->root == rtt_obj2->root
           && rtt_obj1->n >= rtt_obj2->n) ? true : false;
}

WASMStructObjectRef
wasm_struct_obj_new(void *heap_handle, WASMRttObjectRef rtt_obj);

void
wasm_struct_obj_set_field(WASMStructObjectRef struct_obj, uint32 field_idx,
                          WASMValue *value);

void
wasm_struct_obj_get_field(const WASMStructObjectRef struct_obj,
                          uint32 field_idx, bool sign_extend, WASMValue *value);

WASMArrayObjectRef
wasm_array_obj_new(void *heap_handle, WASMRttObjectRef rtt_obj, uint32 length,
                   WASMValue *init_value);

void
wasm_array_obj_set_elem(WASMArrayObjectRef array_obj, uint32 elem_idx,
                        WASMValue *value);

void
wasm_array_obj_get_elem(WASMArrayObjectRef array_obj, uint32 elem_idx,
                        bool sign_extend, WASMValue *value);

/**
 * Return the logarithm of the size of array element.
 *
 * @param array the WASM array object
 *
 * @return log(size of the array element)
 */
inline static uint32
wasm_array_obj_elem_size_log(const WASMArrayObjectRef array_obj)
{
    return (array_obj->length & WASM_ARRAY_ELEM_SIZE_MASK);
}

/**
 * Return the length of the array.
 *
 * @param array_obj the WASM array object
 *
 * @return the length of the array
 */
inline static uint32
wasm_array_obj_length(const WASMArrayObjectRef array_obj)
{
    return array_obj->length >> WASM_ARRAY_LENGTH_SHIFT;
}

/**
 * Return the address of the first element of an array object.
 *
 * @param array_obj the WASM array object
 *
 * @return the address of the first element of the array object
 */
inline static void *
wasm_array_obj_first_elem_addr(const WASMArrayObjectRef array_obj)
{
    return array_obj->elem_data;
}

/**
 * Return the address of the i-th element of an array object.
 *
 * @param array_obj the WASM array object
 * @param index the index of the element
 *
 * @return the address of the i-th element of the array object
 */
inline static void *
wasm_array_obj_elem_addr(const WASMArrayObjectRef array_obj, uint32 elem_idx)
{
    return array_obj->elem_data
           + (elem_idx << wasm_array_obj_elem_size_log(array_obj));
}

WASMFuncObjectRef
wasm_func_obj_new(void *heap_handle, WASMRttObjectRef rtt_obj, uint32 func_idx);

void
wasm_func_obj_set_param(WASMFuncObjectRef func_obj, uint32 param_idx,
                        WASMValue *value);

WASMValue *
wasm_func_obj_get_param(const WASMFuncObjectRef func_obj, uint32 param_idx);

WASMExternrefObjectRef
wasm_externref_obj_new(void *heap_handle, void *foreign_obj);

inline static void *
wasm_externref_obj_get_value(const WASMExternrefObjectRef externref_obj)
{
    return externref_obj->foreign_obj;
}

inline static WASMI31ObjectRef
wasm_i31_obj_new(uint32 i31_value)
{
    return (((uintptr_t)i31_value) << 1) | 1;
}

inline static uint32
wasm_i31_obj_get_value(WASMI31ObjectRef i31_obj, bool sign_extend)
{
    uint32 i31_value = (uint32)(((uintptr_t)i31_obj) >> 1);
    if (sign_extend && (i31_value & 0x40000000))
        i31_value |= 0x80000000;
    return i31_value;
}

inline static bool
wasm_obj_is_i31_obj(WASMObjectRef obj)
{
    bh_assert(obj);
    return (((uintptr_t)obj) & 1) ? true : false;
}

inline static bool
wasm_obj_is_rtt_obj(WASMObjectRef obj)
{
    bh_assert(obj);
    return (!wasm_obj_is_i31_obj(obj) && (obj->header & WASM_OBJ_RTT_OBJ_FLAG))
               ? true
               : false;
}

inline static bool
wasm_obj_is_externref_obj(WASMObjectRef obj)
{
    bh_assert(obj);
    return (!wasm_obj_is_i31_obj(obj)
            && (obj->header & WASM_OBJ_EXTERNREF_OBJ_FLAG))
               ? true
               : false;
}

inline static bool
wasm_obj_is_struct_obj(WASMObjectRef obj)
{
    WASMRttObjectRef rtt_obj;

    bh_assert(obj);

    if (wasm_obj_is_i31_obj(obj)
        || (obj->header
            & (WASM_OBJ_RTT_OBJ_FLAG | WASM_OBJ_EXTERNREF_OBJ_FLAG)))
        return false;

    rtt_obj = (WASMRttObjectRef)wasm_object_header(obj);
    return rtt_obj->type_flag == WASM_TYPE_STRUCT ? true : false;
}

inline static bool
wasm_obj_is_array_obj(WASMObjectRef obj)
{
    WASMRttObjectRef rtt_obj;

    bh_assert(obj);

    if (wasm_obj_is_i31_obj(obj)
        || (obj->header
            & (WASM_OBJ_RTT_OBJ_FLAG | WASM_OBJ_EXTERNREF_OBJ_FLAG)))
        return false;

    rtt_obj = (WASMRttObjectRef)wasm_object_header(obj);
    return rtt_obj->type_flag == WASM_TYPE_ARRAY ? true : false;
}

inline static bool
wasm_obj_is_func_obj(WASMObjectRef obj)
{
    WASMRttObjectRef rtt_obj;

    bh_assert(obj);

    if (wasm_obj_is_i31_obj(obj)
        || (obj->header
            & (WASM_OBJ_RTT_OBJ_FLAG | WASM_OBJ_EXTERNREF_OBJ_FLAG)))
        return false;

    rtt_obj = (WASMRttObjectRef)wasm_object_header(obj);
    return rtt_obj->type_flag == WASM_TYPE_FUNC ? true : false;
}

inline static bool
wasm_obj_is_data_obj(WASMObjectRef obj)
{
    WASMRttObjectRef rtt_obj;

    bh_assert(obj);

    if (wasm_obj_is_i31_obj(obj)
        || (obj->header
            & (WASM_OBJ_RTT_OBJ_FLAG | WASM_OBJ_EXTERNREF_OBJ_FLAG)))
        return false;

    rtt_obj = (WASMRttObjectRef)wasm_object_header(obj);
    return (rtt_obj->type_flag == WASM_TYPE_ARRAY
            || rtt_obj->type_flag == WASM_TYPE_STRUCT) ? true : false;
}

inline static bool
wasm_obj_is_null_obj(WASMObjectRef obj)
{
    return obj == NULL ? true : false;
}

bool
wasm_obj_is_instance_of(WASMObjectRef obj, WASMRttObjectRef rtt_obj);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _GC_OBJECT_H_ */
