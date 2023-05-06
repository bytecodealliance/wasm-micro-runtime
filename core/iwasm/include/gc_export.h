/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _GC_EXPORT_H
#define _GC_EXPORT_H

#include "wasm_export.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t wasm_value_type_t;

typedef enum wasm_value_type_enum {
    VALUE_TYPE_I32 = 0x7F,
    VALUE_TYPE_I64 = 0x7E,
    VALUE_TYPE_F32 = 0x7D,
    VALUE_TYPE_F64 = 0x7C,
    VALUE_TYPE_V128 = 0x7B,
    VALUE_TYPE_FUNCREF = 0x70,
    VALUE_TYPE_EXTERNREF = 0x6F,
    VALUE_TYPE_ANYREF = 0x6E,
    VALUE_TYPE_EQREF = 0x6D,
    VALUE_TYPE_HT_NULLABLE_REF = 0x6C,
    VALUE_TYPE_HT_NON_NULLABLE_REF = 0x6B,
    VALUE_TYPE_I31REF = 0x6A,
    VALUE_TYPE_NULLFUNCREF = 0x69,
    VALUE_TYPE_NULLEXTERNREF = 0x68,
    VALUE_TYPE_STRUCTREF = 0x67,
    VALUE_TYPE_ARRAYREF = 0x66,
    VALUE_TYPE_NULLREF = 0x65,
} wasm_value_type_enum;

typedef int32_t wasm_heap_type_t;

typedef enum wasm_heap_type_enum {
    HEAP_TYPE_FUNC = -0x10,
    HEAP_TYPE_EXTERN = -0x11,
    HEAP_TYPE_ANY = -0x12,
    HEAP_TYPE_EQ = -0x13,
    HEAP_TYPE_I31 = -0x16,
    HEAP_TYPE_NOFUNC = -0x17,
    HEAP_TYPE_NOEXTERN = -0x18,
    HEAP_TYPE_STRUCT = -0x19,
    HEAP_TYPE_ARRAY = -0x1A,
    HEAP_TYPE_NONE = -0x1B
} wasm_heap_type_enum;

struct WASMObject;
typedef struct WASMObject *wasm_obj_t;

#ifndef WASM_VALUE_DEFINED
#define WASM_VALUE_DEFINED
typedef union V128 {
    int8_t i8x16[16];
    int16_t i16x8[8];
    int32_t i32x8[4];
    int64_t i64x2[2];
    float f32x4[4];
    double f64x2[2];
} V128;

typedef union WASMValue {
    int32_t i32;
    uint32_t u32;
    uint32_t global_index;
    uint32_t ref_index;
    int64_t i64;
    uint64_t u64;
    float f32;
    double f64;
    V128 v128;
    wasm_obj_t gc_obj;
    uint32_t type_index;
    struct {
        uint32_t type_index;
        uint32_t N;
    } array_new_canon_fixed;
} WASMValue;
#endif /* end of WASM_VALUE_DEFINED */

typedef union WASMValue wasm_value_t;

/* Reference type, the layout is same as WasmRefType in wasm.h */
typedef struct wasm_ref_type_t {
    wasm_value_type_t value_type;
    bool nullable;
    int32_t heap_type;
} wasm_ref_type_t;

/**
 * Local object reference that can be traced when GC occurs. All
 * native functions that need to hold WASM objects which may not be
 * referenced from other elements of GC root set may be hold with
 * this type of variable so that they can be traced when GC occurs.
 * Before using such a variable, it must be pushed onto the stack
 * (implemented as a chain) of such variables, and before leaving the
 * frame of the variables, they must be popped from the stack.
 */
typedef struct WASMLocalObjectRef {
    /* Previous local object reference variable on the stack */
    struct WASMLocalObjectRef *prev;
    /* The reference of WASM object hold by this variable */
    wasm_obj_t val;
} WASMLocalObjectRef, wasm_local_obj_ref_t;

struct WASMType;
struct WASMFuncType;
struct WASMStructType;
struct WASMArrayType;

typedef struct WASMType *wasm_defined_type_t;
typedef struct WASMFuncType *wasm_func_type_t;
typedef struct WASMStructType *wasm_struct_type_t;
typedef struct WASMArrayType *wasm_array_type_t;

struct WASMExternrefObject;
struct WASMAnyrefObject;
struct WASMStructObject;
struct WASMArrayObject;
struct WASMFuncObject;

typedef struct WASMExternrefObject *wasm_externref_obj_t;
typedef struct WASMAnyrefObject *wasm_anyref_obj_t;
typedef struct WASMStructObject *wasm_struct_obj_t;
typedef struct WASMArrayObject *wasm_array_obj_t;
typedef struct WASMFuncObject *wasm_func_obj_t;
typedef uintptr_t wasm_i31_obj_t;

/* Defined type related operations */

/**
 * Get the defined type count of a WASM module
 */
WASM_RUNTIME_API_EXTERN uint32_t
wasm_get_defined_type_count(const wasm_module_t module);

/**
 * Get the specified defined type of a WASM module
 */
WASM_RUNTIME_API_EXTERN wasm_defined_type_t
wasm_get_defined_type(const wasm_module_t module, uint32_t index);

/**
 * Get defined type of the GC managed object, the object must be struct,
 * array or func.
 *
 * @param obj the object
 *
 * @return defined type of the object.
 */
WASM_RUNTIME_API_EXTERN wasm_defined_type_t
wasm_obj_get_defined_type(const wasm_obj_t obj);

/**
 * Get defined type index of the GC managed object, the object must be struct,
 * array or func.
 *
 * @param obj the object
 *
 * @return defined type index of the object.
 */
WASM_RUNTIME_API_EXTERN int32_t
wasm_obj_get_defined_type_idx(const wasm_module_t module, const wasm_obj_t obj);

/**
 * Check whether a defined type is a function type
 *
 * @param def_type the defined type to be checked
 *
 * @return return true if the defined type is function type, false otherwise
 */
WASM_RUNTIME_API_EXTERN bool
wasm_defined_type_is_func_type(const wasm_defined_type_t def_type);

/**
 * Check whether a defined type is a struct type
 */
WASM_RUNTIME_API_EXTERN bool
wasm_defined_type_is_struct_type(const wasm_defined_type_t def_type);

/**
 * Check whether a defined type is an array type
 *
 * @param def_type the defined type to be checked
 *
 * @return return true if the defined type is array type, false otherwise
 */
WASM_RUNTIME_API_EXTERN bool
wasm_defined_type_is_array_type(const wasm_defined_type_t def_type);

/**
 * Get parameter count of a function type
 *
 * @param func_type the specified function type
 *
 * @return the param count of the specified function type
 */
WASM_RUNTIME_API_EXTERN uint32_t
wasm_func_type_get_param_count(const wasm_func_type_t func_type);

/**
 * Normalize reference type
 */
WASM_RUNTIME_API_EXTERN bool
wasm_ref_type_normalize(wasm_ref_type_t *ref_type);

/**
 * Get type of a specified parameter of a function type
 *
 * @param func_type the specified function type
 * @param param_idx the specified param index
 *
 * @return the param type at the specified param index of the specified func
 * type
 */
WASM_RUNTIME_API_EXTERN wasm_ref_type_t
wasm_func_type_get_param_type(const wasm_func_type_t func_type,
                              uint32_t param_idx);

/**
 * Get result count of a function type
 *
 * @param func_type the specified function type
 *
 * @return the result count of the specified function type
 */
WASM_RUNTIME_API_EXTERN uint32_t
wasm_func_type_get_result_count(const wasm_func_type_t func_type);

/**
 * Get type of a specified result of a function type
 *
 * @param func_type the specified function type
 * @param param_idx the specified result index
 *
 * @return the result type at the specified result index of the specified func
 * type
 */
WASM_RUNTIME_API_EXTERN wasm_ref_type_t
wasm_func_type_get_result_type(const wasm_func_type_t func_type,
                               uint32_t result_idx);

/**
 * Get field count of a struct type
 */
WASM_RUNTIME_API_EXTERN uint32_t
wasm_struct_type_get_field_count(const wasm_struct_type_t struct_type);

/**
 * Get type of a specified field of a struct type
 */
WASM_RUNTIME_API_EXTERN wasm_ref_type_t
wasm_struct_type_get_field_type(const wasm_struct_type_t struct_type,
                                uint32_t field_idx, bool *p_is_mutable);

/**
 * Get element type of an array type
 *
 * @param array_type the specified array type
 * @param p_is_mutable the pointer passed by invoker, record if the elem is
 * mutable
 *
 * @return the ref type of array's elem type
 */
WASM_RUNTIME_API_EXTERN wasm_ref_type_t
wasm_array_type_get_elem_type(const wasm_array_type_t array_type,
                              bool *p_is_mutable);

/**
 * Check whether two defined types are equal
 */
WASM_RUNTIME_API_EXTERN bool
wasm_defined_type_equal(const wasm_defined_type_t def_type1,
                        const wasm_defined_type_t def_type2,
                        const wasm_module_t module);

/**
 * Check whether def_type1 is subtype of def_type2
 *
 * @param def_type1 the specified defined type1
 * @param def_type2 the specified defined type2
 * @param module current wasm module
 *
 * @return return true if the defined type1 is subtype of the defined type2,
 * false otherwise
 */
WASM_RUNTIME_API_EXTERN bool
wasm_defined_type_is_subtype_of(const wasm_defined_type_t def_type1,
                                const wasm_defined_type_t def_type2,
                                const wasm_module_t module);

/* ref type related operations */

/**
 * Set the ref_type to be (ref null? type_idx)
 */
WASM_RUNTIME_API_EXTERN void
wasm_ref_type_set_type_idx(wasm_ref_type_t *ref_type, bool nullable,
                           int32_t type_idx);

/**
 * Set the ref_type to be (ref null? func/extern/any/eq/i31/struct/array/..)
 */
WASM_RUNTIME_API_EXTERN void
wasm_ref_type_set_heap_type(wasm_ref_type_t *ref_type, bool nullable,
                            int32_t heap_type);

/**
 * Check whether two ref types are equal
 */
WASM_RUNTIME_API_EXTERN bool
wasm_ref_type_equal(const wasm_ref_type_t *ref_type1,
                    const wasm_ref_type_t *ref_type2,
                    const wasm_module_t module);

/**
 * Check whether ref_type1 is subtype of ref_type2
 */
WASM_RUNTIME_API_EXTERN bool
wasm_ref_type_is_subtype_of(const wasm_ref_type_t *ref_type1,
                            const wasm_ref_type_t *ref_type2,
                            const wasm_module_t module);

/* wasm object related operations */

/**
 * Create a struct object with the index of defined type
 */
WASM_RUNTIME_API_EXTERN wasm_struct_obj_t
wasm_struct_obj_new_with_typeidx(wasm_exec_env_t exec_env, uint32_t type_idx);

/**
 * Create a struct object with the struct type
 */
WASM_RUNTIME_API_EXTERN wasm_struct_obj_t
wasm_struct_obj_new_with_type(wasm_exec_env_t exec_env,
                              const wasm_struct_type_t type);

/**
 * Set the field value of a struct object
 */
WASM_RUNTIME_API_EXTERN void
wasm_struct_obj_set_field(wasm_struct_obj_t obj, uint32_t field_idx,
                          const wasm_value_t *value);

/**
 * Get the field value of a struct object
 */
WASM_RUNTIME_API_EXTERN void
wasm_struct_obj_get_field(const wasm_struct_obj_t obj, uint32_t field_idx,
                          bool sign_extend, wasm_value_t *value);

/**
 * Create an array object with the index of defined type, the obj's length is
 * length, init value is init_value
 *
 * @param exec_env the execution environment
 * @param type_idx the index of the specified type
 * @param length the array's length
 * @param init_value the array's init value
 *
 * @return the created array object
 */
WASM_RUNTIME_API_EXTERN wasm_array_obj_t
wasm_array_obj_new_with_typeidx(wasm_exec_env_t exec_env, uint32_t type_idx,
                                uint32_t length, wasm_value_t *init_value);

/**
 * Create an array object with the array type, the obj's length is length, init
 * value is init_value
 *
 * @param exec_env the execution environment
 * @param type the array's specified type
 * @param length the array's length
 * @param init_value the array's init value
 *
 * @return the created array object
 */
WASM_RUNTIME_API_EXTERN wasm_array_obj_t
wasm_array_obj_new_with_type(wasm_exec_env_t exec_env,
                             const wasm_array_type_t type, uint32_t length,
                             wasm_value_t *init_value);

/**
 * Set the specified element's value of an array object
 */
WASM_RUNTIME_API_EXTERN void
wasm_array_obj_set_elem(wasm_array_obj_t array_obj, uint32_t elem_idx,
                        const wasm_value_t *value);

/**
 * Get the specified element's value of an array object
 */
WASM_RUNTIME_API_EXTERN void
wasm_array_obj_get_elem(const wasm_array_obj_t array_obj, uint32_t elem_idx,
                        bool sign_extend, wasm_value_t *value);

/**
 * Copy elements from one array to another
 */
WASM_RUNTIME_API_EXTERN void
wasm_array_obj_copy(wasm_array_obj_t dst_obj, uint32_t dst_idx,
                    const wasm_array_obj_t src_obj, uint32_t src_idx,
                    uint32_t len);

/**
 * Return the length of an array object
 */
WASM_RUNTIME_API_EXTERN uint32_t
wasm_array_obj_length(const wasm_array_obj_t array_obj);

/**
 * Get the address of the first element of an array object
 */
WASM_RUNTIME_API_EXTERN void *
wasm_array_obj_first_elem_addr(const wasm_array_obj_t array_obj);

/**
 * Get the address of the i-th element of an array object
 */
WASM_RUNTIME_API_EXTERN void *
wasm_array_obj_elem_addr(const wasm_array_obj_t array_obj, uint32_t elem_idx);

/**
 * Create a function object with the index of defined type and the index of the
 * function
 *
 * @param exec_env the execution environment
 * @param type_idx the index of the specified type
 * @param func_idx_bound the index of the function
 *
 * @return the created function object
 */
WASM_RUNTIME_API_EXTERN wasm_func_obj_t
wasm_func_obj_new_with_typeidx(wasm_exec_env_t exec_env, uint32_t type_idx,
                               uint32_t func_idx_bound);

/**
 * Create a function object with the function type and the index of the function
 *
 * @param exec_env the execution environment
 * @param type the specified type
 * @param func_idx_bound the index of the function
 *
 * @return the created function object
 */
WASM_RUNTIME_API_EXTERN wasm_func_obj_t
wasm_func_obj_new_with_type(wasm_exec_env_t exec_env, wasm_func_type_t type,
                            uint32_t func_idx_bound);

/**
 * Get the function index bound of a function object
 */
WASM_RUNTIME_API_EXTERN uint32_t
wasm_func_obj_get_func_idx_bound(const wasm_func_obj_t func_obj);

/**
 * Get the function type of a function object
 */
WASM_RUNTIME_API_EXTERN wasm_func_type_t
wasm_func_obj_get_func_type(const wasm_func_obj_t func_obj);

/**
 * Call the given WASM function object with arguments (bytecode and AoT).
 *
 * @param exec_env the execution environment to call the function,
 *   which must be created from wasm_create_exec_env()
 * @param func_obj the function object to call
 * @param argc total cell number that the function parameters occupy,
 *   a cell is a slot of the uint32 array argv[], e.g. i32/f32 argument
 *   occupies one cell, i64/f64 argument occupies two cells, note that
 *   it might be different from the parameter number of the function
 * @param argv the arguments. If the function has return value,
 *   the first (or first two in case 64-bit return value) element of
 *   argv stores the return value of the called WASM function after this
 *   function returns.
 *
 * @return true if success, false otherwise and exception will be thrown,
 *   the caller can call wasm_runtime_get_exception to get the exception
 *   info.
 */
WASM_RUNTIME_API_EXTERN bool
wasm_runtime_call_func_ref(wasm_exec_env_t exec_env,
                           const wasm_func_obj_t func_obj, uint32_t argc,
                           uint32_t argv[]);

/**
 * Call the given WASM function object with provided results space
 * and arguments (bytecode and AoT).
 *
 * @param exec_env the execution environment to call the function,
 *   which must be created from wasm_create_exec_env()
 * @param func_obj the function object to call
 * @param num_results the number of results
 * @param results the pre-alloced pointer to get the results
 * @param num_args the number of arguments
 * @param args the arguments
 *
 * @return true if success, false otherwise and exception will be thrown,
 *   the caller can call wasm_runtime_get_exception to get the exception
 *   info.
 */
WASM_RUNTIME_API_EXTERN bool
wasm_runtime_call_func_ref_a(wasm_exec_env_t exec_env,
                             const wasm_func_obj_t func_obj,
                             uint32_t num_results, wasm_val_t results[],
                             uint32_t num_args, wasm_val_t *args);

/**
 * Call the given WASM function object with provided results space and
 * variant arguments (bytecode and AoT).
 *
 * @param exec_env the execution environment to call the function,
 *   which must be created from wasm_create_exec_env()
 * @param func_obj the function object to call
 * @param num_results the number of results
 * @param results the pre-alloced pointer to get the results
 * @param num_args the number of arguments
 * @param ... the variant arguments
 *
 * @return true if success, false otherwise and exception will be thrown,
 *   the caller can call wasm_runtime_get_exception to get the exception
 *   info.
 */
WASM_RUNTIME_API_EXTERN bool
wasm_runtime_call_func_ref_v(wasm_exec_env_t exec_env,
                             const wasm_func_obj_t func_obj,
                             uint32_t num_results, wasm_val_t results[],
                             uint32_t num_args, ...);

/**
 * Create an externref object with host object
 */
WASM_RUNTIME_API_EXTERN wasm_externref_obj_t
wasm_externref_obj_new(wasm_exec_env_t exec_env, const void *host_obj);

/**
 * Get the host value of an externref object
 */
WASM_RUNTIME_API_EXTERN const void *
wasm_externref_obj_get_value(const wasm_externref_obj_t externref_obj);

/**
 * Create an anyref object with host object
 */
WASM_RUNTIME_API_EXTERN wasm_anyref_obj_t
wasm_anyref_obj_new(wasm_exec_env_t exec_env, const void *host_obj);

/**
 * Get the host object value of an anyref object
 */
WASM_RUNTIME_API_EXTERN const void *
wasm_anyref_obj_get_value(const wasm_anyref_obj_t anyref_obj);

/**
 * Get the internal object inside the externref object, same as
 * the operation of opcode extern.internalize
 */
WASM_RUNTIME_API_EXTERN wasm_obj_t
wasm_externref_obj_to_internal_obj(const wasm_externref_obj_t externref_obj);

/**
 * Create an externref object from an internal object, same as
 * the operation of opcode extern.externalize
 */
WASM_RUNTIME_API_EXTERN wasm_externref_obj_t
wasm_internal_obj_to_externref_obj(wasm_exec_env_t exec_env,
                                   const wasm_obj_t internal_obj);

/**
 * Create an i31 object
 */
WASM_RUNTIME_API_EXTERN wasm_i31_obj_t
wasm_i31_obj_new(uint32_t i31_value);

WASM_RUNTIME_API_EXTERN uint32_t
wasm_i31_obj_get_value(wasm_i31_obj_t i31_obj, bool sign_extend);

/**
 * Pin an object to make it traced during GC
 */
WASM_RUNTIME_API_EXTERN bool
wasm_runtime_pin_object(wasm_exec_env_t exec_env, wasm_obj_t obj);

/**
 * Unpin an object
 */
WASM_RUNTIME_API_EXTERN bool
wasm_runtime_unpin_object(wasm_exec_env_t exec_env, wasm_obj_t obj);

/**
 * Check whether an object is a struct object
 */
WASM_RUNTIME_API_EXTERN bool
wasm_obj_is_struct_obj(const wasm_obj_t obj);

/**
 * Check whether an object is an array object
 */
WASM_RUNTIME_API_EXTERN bool
wasm_obj_is_array_obj(const wasm_obj_t obj);

/**
 * Check whether an object is a function object
 */
WASM_RUNTIME_API_EXTERN bool
wasm_obj_is_func_obj(const wasm_obj_t obj);

/**
 * Check whether an object is an i31 object
 */
WASM_RUNTIME_API_EXTERN bool
wasm_obj_is_i31_obj(const wasm_obj_t obj);

/**
 * Check whether an object is an externref object
 */
WASM_RUNTIME_API_EXTERN bool
wasm_obj_is_externref_obj(const wasm_obj_t obj);

/**
 * Check whether an object is an anyref object
 */
WASM_RUNTIME_API_EXTERN bool
wasm_obj_is_anyref_obj(const wasm_obj_t obj);

/**
 * Check whether an object is a struct object, or, an i31/struct/array object
 */
WASM_RUNTIME_API_EXTERN bool
wasm_obj_is_internal_obj(const wasm_obj_t obj);

/**
 * Check whether an object is an eq object
 */
WASM_RUNTIME_API_EXTERN bool
wasm_obj_is_eq_obj(const wasm_obj_t obj);

/**
 * Check whether an object is an instance of a defined type
 */
WASM_RUNTIME_API_EXTERN bool
wasm_obj_is_instance_of_defined_type(const wasm_obj_t obj,
                                     const wasm_defined_type_t defined_type,
                                     const wasm_module_t module);

/**
 * Check whether an object is an instance of a defined type with
 * index type_idx
 */
WASM_RUNTIME_API_EXTERN bool
wasm_obj_is_instance_of_type_idx(const wasm_obj_t obj, uint32_t type_idx,
                                 const wasm_module_t module);

/**
 * Check whether an object is an instance of a ref type
 */
WASM_RUNTIME_API_EXTERN bool
wasm_obj_is_instance_of_ref_type(const wasm_obj_t obj,
                                 const wasm_ref_type_t *ref_type);

/**
 * Push a local object ref into stack, note that we should set its value
 * after pushing to retain it during GC, and should pop it from stack
 * before returning from the current function
 */
WASM_RUNTIME_API_EXTERN void
wasm_runtime_push_local_object_ref(wasm_exec_env_t exec_env,
                                   wasm_local_obj_ref_t *local_obj_ref);

/**
 * Pop a local object ref from stack
 */
WASM_RUNTIME_API_EXTERN wasm_local_obj_ref_t *
wasm_runtime_pop_local_object_ref(wasm_exec_env_t exec_env);

/**
 * Pop n local object refs from stack
 */
WASM_RUNTIME_API_EXTERN void
wasm_runtime_pop_local_object_refs(wasm_exec_env_t exec_env, uint32_t n);

#ifdef __cplusplus
}
#endif

#endif /* end of _GC_EXPORT_H */
