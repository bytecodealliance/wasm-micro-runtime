/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _AOT_EMIT_AOT_FILE_H_
#define _AOT_EMIT_AOT_FILE_H_

#include "aot_compiler.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Internal function in object file */
typedef struct AOTObjectFunc {
    char *func_name;
    /* text offset of aot_func#n */
    uint64 text_offset;
    /* text offset of aot_func_internal#n */
    uint64 text_offset_of_aot_func_internal;
} AOTObjectFunc;

/* Symbol table list node */
typedef struct AOTSymbolNode {
    struct AOTSymbolNode *next;
    uint32 str_len;
    char *symbol;
} AOTSymbolNode;

typedef struct AOTSymbolList {
    AOTSymbolNode *head;
    AOTSymbolNode *end;
    uint32 len;
} AOTSymbolList;

/* AOT object data */
typedef struct AOTObjectData {
    AOTCompContext *comp_ctx;

    LLVMMemoryBufferRef mem_buf;
    LLVMBinaryRef binary;

    AOTTargetInfo target_info;

    void *text;
    uint32 text_size;

    void *text_unlikely;
    uint32 text_unlikely_size;

    void *text_hot;
    uint32 text_hot_size;

    /* literal data and size */
    void *literal;
    uint32 literal_size;

    AOTObjectDataSection *data_sections;
    uint32 data_sections_count;

    AOTObjectFunc *funcs;
    uint32 func_count;

    AOTSymbolList symbol_list;
    AOTRelocationGroup *relocation_groups;
    uint32 relocation_group_count;

    const char *stack_sizes_section_name;
    uint32 stack_sizes_offset;
    uint32 *stack_sizes;
} AOTObjectData;


AOTObjectData *
aot_obj_data_create(AOTCompContext *comp_ctx);

uint32
get_aot_file_size(AOTCompContext *comp_ctx, AOTCompData *comp_data,
                  AOTObjectData *obj_data);

bool
aot_emit_aot_file(AOTCompContext *comp_ctx, AOTCompData *comp_data,
                  const char *file_name);

uint8 *
aot_emit_aot_file_buf(AOTCompContext *comp_ctx, AOTCompData *comp_data,
                      uint32 *p_aot_file_size);

bool
aot_emit_aot_file_buf_ex(AOTCompContext *comp_ctx, AOTCompData *comp_data,
                         AOTObjectData *obj_data,
                         uint8 *aot_file_buf, uint32 aot_file_size);

void
aot_obj_data_destroy(AOTObjectData *obj_data);


#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* end of _AOT_EMIT_AOT_FILE_H_ */
