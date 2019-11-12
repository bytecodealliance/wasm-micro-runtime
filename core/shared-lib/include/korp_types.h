/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _KORP_TYPES_H
#define _KORP_TYPES_H

#include "bh_platform.h"
/* all types used in kORP should be explicit sized */
typedef struct _korp_object korp_object;

typedef unsigned int obj_info;
typedef korp_object* ref;

#define BYTES_OF_OBJ_INFO 4
#define BYTES_OF_REF     4

/* don't change the number, it's hardcoded in kORP */
enum _korp_array_type {
    ARRAY_UINT8 = 0, /* bytes_of_uint8 = 1 << ARRAY_UINT8 */
    ARRAY_UINT16 = 1, /* bytes_of_uint16 = 1 << ARRAY_UINT16 */
    ARRAY_UINT32 = 2, /* bytes_of_uint32 = 1 << ARRAY_UINT32 */
    ARRAY_UINT64 = 3, /* bytes_of_uint64 = 1 << ARRAY_UINT64 */
    ARRAY_BOOLEAN = 4,
    ARRAY_CHAR = 5,
    ARRAY_FLOAT = 6,
    ARRAY_DOUBLE = 7,
    ARRAY_BYTE = 8,
    ARRAY_SHORT = 9,
    ARRAY_INT = 10,
    ARRAY_LONG = 11,
    ARRAY_REF = 12 /* for calculation */
};

enum _korp_java_type {
    JAVA_TYPE_WRONG = 0,
    JAVA_TYPE_BYTE = 'B',
    JAVA_TYPE_CHAR = 'C',
    JAVA_TYPE_DOUBLE = 'D',
    JAVA_TYPE_FLOAT = 'F',
    JAVA_TYPE_INT = 'I',
    JAVA_TYPE_LONG = 'J',
    JAVA_TYPE_SHORT = 'S',
    JAVA_TYPE_BOOLEAN = 'Z',
    JAVA_TYPE_CLASS = 'L',
    JAVA_TYPE_ARRAY = '[',
    JAVA_TYPE_VOID = 'V',
    JAVA_TYPE_STRING = '$' /* for TAG_String const value */
};

enum korp_modifier_type {
    MOD_PUBLIC = 0x0001, /* Class	Field	Method  */
    MOD_PRIVATE = 0x0002, /*			Field	Method  */
    MOD_PROTECTED = 0x0004, /*			Field	Method  */
    MOD_STATIC = 0x0008, /*			Field	Method  */
    MOD_FINAL = 0x0010, /* Class	Field	Method  */
    MOD_SUPER = 0x0020, /* Class                    */
    MOD_SYNCHRONIZED = 0x0020, /*					Method  */
    MOD_VOLATILE = 0x0040, /*			Field           */
    MOD_TRANSIENT = 0x0080, /*			Field           */
    MOD_NATIVE = 0x0100, /*					Method  */
    MOD_INTERFACE = 0x0200, /* Class                    */
    MOD_ABSTRACT = 0x0400, /* Class			Method  */
    MOD_STRICT = 0x0800 /*                  Method  */
};

/* object header, used to access object info */
struct _korp_object {
    obj_info header; /* object header (I) */
};

#define HASH_TABLE_SIZE 359

#ifndef NULL
#define NULL (void*)0
#endif

#define KORP_ERROR (-1)

#ifndef __cplusplus
#define true 1
#define false 0
#define inline __inline
#endif

/* forwarded declarations */
typedef struct _korp_string_pool korp_string_pool;
typedef struct _korp_class_table korp_class_table;

typedef enum _korp_loader_exception {
    LD_OK = 0,
    LD_NoClassDefFoundError,
    LD_ClassFormatError,
    LD_ClassCircularityError,
    LD_IncompatibleClassChangeError,
    LD_AbstractMethodError, /* occurs during preparation */
    LD_IllegalAccessError,
    LD_InstantiationError,
    LD_NoSuchFieldError,
    LD_NoSuchMethodError,
    LD_UnsatisfiedLinkError,
    LD_VerifyError
} korp_loader_exception;

typedef enum _korp_java_type korp_java_type;
typedef enum _korp_array_type korp_array_type;

/* typedef struct _korp_thread korp_thread; */
typedef struct _korp_method korp_method;
typedef struct _korp_field korp_field;
typedef struct _korp_class korp_class;
typedef struct _korp_string korp_string;
typedef struct _korp_package korp_package;
typedef struct _korp_class_loader korp_class_loader;
typedef struct _korp_ref_array korp_ref_array;

typedef struct _korp_entry korp_entry;
typedef struct _korp_preloaded korp_preloaded;
typedef struct _korp_env korp_env;

typedef struct _korp_java_array korp_java_array;
typedef struct _korp_uint8_array korp_uint8_array;

typedef struct _korp_vm_thread_list korp_vm_thread_list;

#define korp_uint8      korp_uint32
#define korp_uint16     korp_uint32
#define korp_boolean    korp_uint32
#define korp_char       korp_uint32
#define korp_short      korp_uint32
#define korp_int        korp_uint32
#define korp_float      korp_uint32

#define korp_long       korp_uint64
#define korp_double     korp_uint64

#define korp_boolean_array      korp_uint8_array
#define korp_char_array         korp_uint8_array
#define korp_short_array        korp_uint16_array
#define korp_int_array          korp_uint32_array
#define korp_float_array        korp_uint32_array
#define korp_double_array       korp_uint64_array
#define korp_long_array         korp_uint64_array

#define korp_code       korp_uint8_array

#endif /* #ifndef _KORP_TYPES_H */

