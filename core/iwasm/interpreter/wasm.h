/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WASM_H_
#define _WASM_H_

#include "bh_platform.h"
#include "bh_hashmap.h"
#include "bh_assert.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Value Type */
#define VALUE_TYPE_I32 0x7F
#define VALUE_TYPE_I64 0X7E
#define VALUE_TYPE_F32 0x7D
#define VALUE_TYPE_F64 0x7C
#define VALUE_TYPE_VOID 0x40
/* Used by AOT */
#define VALUE_TYPE_I1  0x41

/* Table Element Type */
#define TABLE_ELEM_TYPE_ANY_FUNC 0x70

#define DEFAULT_NUM_BYTES_PER_PAGE 65536

#define INIT_EXPR_TYPE_I32_CONST 0x41
#define INIT_EXPR_TYPE_I64_CONST 0x42
#define INIT_EXPR_TYPE_F32_CONST 0x43
#define INIT_EXPR_TYPE_F64_CONST 0x44
#define INIT_EXPR_TYPE_GET_GLOBAL 0x23
#define INIT_EXPR_TYPE_ERROR 0xff

#define WASM_MAGIC_NUMBER 0x6d736100
#define WASM_CURRENT_VERSION 1

#define SECTION_TYPE_USER 0
#define SECTION_TYPE_TYPE 1
#define SECTION_TYPE_IMPORT 2
#define SECTION_TYPE_FUNC 3
#define SECTION_TYPE_TABLE 4
#define SECTION_TYPE_MEMORY 5
#define SECTION_TYPE_GLOBAL 6
#define SECTION_TYPE_EXPORT 7
#define SECTION_TYPE_START 8
#define SECTION_TYPE_ELEM 9
#define SECTION_TYPE_CODE 10
#define SECTION_TYPE_DATA 11

#define IMPORT_KIND_FUNC 0
#define IMPORT_KIND_TABLE 1
#define IMPORT_KIND_MEMORY 2
#define IMPORT_KIND_GLOBAL 3

#define EXPORT_KIND_FUNC 0
#define EXPORT_KIND_TABLE 1
#define EXPORT_KIND_MEMORY 2
#define EXPORT_KIND_GLOBAL 3

#define BLOCK_TYPE_BLOCK 0
#define BLOCK_TYPE_LOOP 1
#define BLOCK_TYPE_IF 2
#define BLOCK_TYPE_FUNCTION 3

typedef union WASMValue {
    int32 i32;
    uint32 u32;
    int64 i64;
    uint64 u64;
    float32 f32;
    float64 f64;
    uintptr_t addr;
} WASMValue;

typedef struct InitializerExpression {
    /* type of INIT_EXPR_TYPE_XXX */
    uint8 init_expr_type;
    union {
        int32 i32;
        int64 i64;
        float32 f32;
        float64 f64;
        uint32 global_index;
    } u;
} InitializerExpression;

typedef struct WASMType {
    uint32 param_count;
    /* only one result is supported currently */
    uint32 result_count;
    /* types of params and results */
    uint8 types[1];
} WASMType;

typedef struct WASMTable {
    uint8 elem_type;
    uint32 flags;
    uint32 init_size;
    /* specified if (flags & 1), else it is 0x10000 */
    uint32 max_size;
} WASMTable;

typedef struct WASMMemory {
    uint32 flags;
    uint32 num_bytes_per_page;
    uint32 init_page_count;
    uint32 max_page_count;
} WASMMemory;

typedef struct WASMTableImport {
    char *module_name;
    char *field_name;
    uint8 elem_type;
    uint32 flags;
    uint32 init_size;
    /* specified if (flags & 1), else it is 0x10000 */
    uint32 max_size;
} WASMTableImport;

typedef struct WASMMemoryImport {
    char *module_name;
    char *field_name;
    uint32 flags;
    uint32 num_bytes_per_page;
    uint32 init_page_count;
    uint32 max_page_count;
} WASMMemoryImport;

typedef struct WASMFunctionImport {
    char *module_name;
    char *field_name;
    /* function type */
    WASMType *func_type;
    /* function pointer after linked */
    void *func_ptr_linked;
} WASMFunctionImport;

typedef struct WASMGlobalImport {
    char *module_name;
    char *field_name;
    uint8 type;
    bool is_mutable;
    /* global data after linked */
    WASMValue global_data_linked;
} WASMGlobalImport;

typedef struct WASMImport {
    uint8 kind;
    union {
        WASMFunctionImport function;
        WASMTableImport table;
        WASMMemoryImport memory;
        WASMGlobalImport global;
        struct {
            char *module_name;
            char *field_name;
        } names;
    } u;
} WASMImport;

typedef struct WASMFunction {
    /* the type of function */
    WASMType *func_type;
    uint32 local_count;
    uint8 *local_types;

    /* cell num of parameters */
    uint16 param_cell_num;
    /* cell num of return type */
    uint16 ret_cell_num;
    /* cell num of local variables */
    uint16 local_cell_num;
    /* offset of each local, including function paramameters
       and local variables */
    uint16 *local_offsets;

    uint32 max_stack_cell_num;
    uint32 max_block_num;
    /* Whether function has opcode memory.grow */
    bool has_op_memory_grow;
    /* Whether function has opcode call or
       call_indirect */
    bool has_op_func_call;
    uint32 code_size;
    uint8 *code;
} WASMFunction;

typedef struct WASMGlobal {
    uint8 type;
    bool is_mutable;
    InitializerExpression init_expr;
} WASMGlobal;

typedef struct WASMExport {
    char *name;
    uint8 kind;
    uint32 index;
} WASMExport;

typedef struct WASMTableSeg {
    uint32 table_index;
    InitializerExpression base_offset;
    uint32 function_count;
    uint32 *func_indexes;
} WASMTableSeg;

typedef struct WASMDataSeg {
    uint32 memory_index;
    InitializerExpression base_offset;
    uint32 data_length;
    uint8 *data;
} WASMDataSeg;

typedef struct BlockAddr {
    const uint8 *start_addr;
    uint8 *else_addr;
    uint8 *end_addr;
} BlockAddr;

#if WASM_ENABLE_LIBC_WASI != 0
typedef struct WASIArguments {
    const char **dir_list;
    uint32 dir_count;
    const char **map_dir_list;
    uint32 map_dir_count;
    const char **env;
    uint32 env_count;
    const char **argv;
    uint32 argc;
} WASIArguments;
#endif

typedef struct StringNode {
    struct StringNode *next;
    char *str;
} StringNode, *StringList;

typedef struct WASMModule {
    /* Module type, for module loaded from WASM bytecode binary,
       this field is Wasm_Module_Bytecode;
       for module loaded from AOT file, this field is
       Wasm_Module_AoT, and this structure should be treated as
       AOTModule structure. */
    uint32 module_type;

    uint32 type_count;
    uint32 import_count;
    uint32 function_count;
    uint32 table_count;
    uint32 memory_count;
    uint32 global_count;
    uint32 export_count;
    uint32 table_seg_count;
    uint32 data_seg_count;

    uint32 import_function_count;
    uint32 import_table_count;
    uint32 import_memory_count;
    uint32 import_global_count;

    WASMImport *import_functions;
    WASMImport *import_tables;
    WASMImport *import_memories;
    WASMImport *import_globals;

    WASMType **types;
    WASMImport *imports;
    WASMFunction **functions;
    WASMTable *tables;
    WASMMemory *memories;
    WASMGlobal *globals;
    WASMExport *exports;
    WASMTableSeg *table_segments;
    WASMDataSeg **data_segments;
    uint32 start_function;

    /* __data_end global exported by llvm */
    uint32 llvm_aux_data_end;
    /* auxiliary stack bottom, or __heap_base global exported by llvm */
    uint32 llvm_aux_stack_bottom;
    /* auxiliary stack size */
    uint32 llvm_aux_stack_size;
    /* the index of a global exported by llvm, which is
       auxiliary stack top pointer */
    uint32 llvm_aux_stack_global_index;

    /* Whether there is possible memory grow, e.g.
       memory.grow opcode or call enlargeMemory */
    bool possible_memory_grow;

    StringList const_str_list;

#if WASM_ENABLE_LIBC_WASI != 0
    WASIArguments wasi_args;
    bool is_wasi_module;
#endif
} WASMModule;

typedef struct WASMBranchBlock {
    uint8 block_type;
    uint8 return_type;
    uint8 *target_addr;
    uint32 *frame_sp;
} WASMBranchBlock;

/* Execution environment, e.g. stack info */
/**
 * Align an unsigned value on a alignment boundary.
 *
 * @param v the value to be aligned
 * @param b the alignment boundary (2, 4, 8, ...)
 *
 * @return the aligned value
 */
inline static unsigned
align_uint (unsigned v, unsigned b)
{
    unsigned m = b - 1;
    return (v + m) & ~m;
}

/**
 * Return the hash value of c string.
 */
inline static uint32
wasm_string_hash(const char *str)
{
    unsigned h = (unsigned)strlen(str);
    const uint8 *p = (uint8*)str;
    const uint8 *end = p + h;

    while (p != end)
        h = ((h << 5) - h) + *p++;
    return h;
}

/**
 * Whether two c strings are equal.
 */
inline static bool
wasm_string_equal(const char *s1, const char *s2)
{
    return strcmp(s1, s2) == 0 ? true : false;
}

/**
 * Return the byte size of value type.
 *
 */
inline static uint32
wasm_value_type_size(uint8 value_type)
{
    switch (value_type) {
        case VALUE_TYPE_I32:
        case VALUE_TYPE_F32:
            return sizeof(int32);
        case VALUE_TYPE_I64:
        case VALUE_TYPE_F64:
            return sizeof(int64);
        default:
            bh_assert(0);
    }
    return 0;
}

inline static uint16
wasm_value_type_cell_num(uint8 value_type)
{
    switch (value_type) {
        case VALUE_TYPE_I32:
        case VALUE_TYPE_F32:
            return 1;
        case VALUE_TYPE_I64:
        case VALUE_TYPE_F64:
            return 2;
        default:
            bh_assert(0);
    }
    return 0;
}

inline static uint16
wasm_get_cell_num(const uint8 *types, uint32 type_count)
{
    uint32 cell_num = 0;
    uint32 i;
    for (i = 0; i < type_count; i++)
        cell_num += wasm_value_type_cell_num(types[i]);
    return (uint16)cell_num;
}

inline static uint16
wasm_type_param_cell_num(const WASMType *type)
{
    return wasm_get_cell_num(type->types, type->param_count);
}

inline static uint16
wasm_type_return_cell_num(const WASMType *type)
{
    return wasm_get_cell_num(type->types + type->param_count,
                             type->result_count);
}

inline static bool
wasm_type_equal(const WASMType *type1, const WASMType *type2)
{
    return (type1->param_count == type2->param_count
            && type1->result_count == type2->result_count
            && memcmp(type1->types, type2->types,
                      type1->param_count + type1->result_count) == 0)
        ? true : false;
}

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _WASM_H_ */

