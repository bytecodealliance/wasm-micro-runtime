/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _CONSTSTRDESC_H
#define _CONSTSTRDESC_H

#include "bh_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ConstStrDescription {
    const char * str;
    bool is_sys_symbol:1;
    uint32 len:31;
    uint32 hash;
    struct ConstStrDescription * next;
} ConstStrDescription;
//#define CONST_STR_HASHMAP_KEY_HEAD_LEN (sizeof(void * ) + 2 * sizeof(uint32))

#define DEF_CONST_STRING(name, string) WAMR_CSP_##name,
#define DEF_CONST_STRING2(name) WAMR_CSP_##name,
enum WAMR_CONST_STRING_IDENT {
    #include "wasm_symbols.h"
    WAMR_CSP_SYMBOLS_end,
};
#undef DEF_CONST_STRING2
#undef DEF_CONST_STRING

#define DEF_CONST_STRING(name, string) string "\0"
#define DEF_CONST_STRING2(name) #name "\0"
static const char wasm_init_symbols[] = {
    #include "wasm_symbols.h"
};
#undef DEF_CONST_STRING2
#undef DEF_CONST_STRING

#define CONST_STR_POOL_DESC(runtime, id) (&runtime->global_const_str_index_array[id])
#define CONST_STR_POOL_STR(runtime, id) (runtime->global_const_str_index_array[id].str)

#ifdef __cplusplus
}
#endif

#endif
