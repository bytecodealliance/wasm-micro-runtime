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
 * Object header of a WASM object, can be a pointer of
 * defined type (WASMFuncType/WASMStructType or WASMArrayType),
 * or rtt type (WASMRttObject),
 * or ExternRef type (bit_1 = 1).
 * And use bit_0 to mark whether it is an weakref object.
 */
typedef uintptr_t WASMObjectHeader;

/* Representation of WASM objects. */
typedef struct WASMObject {
    WASMObjectHeader header;
} WASMObject, *WASMObjectRef;

/* Representation of WASM struct objects. */
typedef struct WASMStructObject {
    WASMObject super_;
} WASMStructObject;

/* Representation of WASM array objects. */
typedef struct WASMArrayObject {
    WASMObject super_;
    /* (<array length> << 2) | <array element size>,
     * actual_length = lenght >> 2
     * elem_size = 2 ^ (length & 0x3)
     */
    uint32 length;
} WASMArrayObject;

/* Representation of WASM function objects. */
typedef struct WASMFuncObject {
    WASMObject super_;
} WASMFuncObject;

/* Representation of WASM externref objects. */
typedef struct WASMExternrefObject {
    WASMObject super_;
} WASMExternrefObject;

/* Representation of WASM rtt objects. */
typedef struct WASMRttObject {
    /* type must be 2, denotes an Rtt object */
    uintptr_t type;
    /* 0/1/2 means it is rtt object of
      func/struct/array type */
    uint16 flag;
    uint16 n;
    WASMType *defined_type;
    struct WASMRttObject *root;
    struct WASMRttObject *parent;
} WASMRttObject;

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _GC_OBJECT_H_ */
