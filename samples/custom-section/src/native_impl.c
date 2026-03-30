/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <string.h>

#include "wasm_export.h"

#define MAX_CUSTOM_SECTION_HANDLES 8

typedef struct CustomSectionHandle {
    wasm_module_t module;
    const uint8_t *content;
    uint32_t length;
} CustomSectionHandle;

static CustomSectionHandle custom_section_handles[MAX_CUSTOM_SECTION_HANDLES];

void
reset_custom_section_handles(void)
{
    memset(custom_section_handles, 0, sizeof(custom_section_handles));
}

int32_t
get_custom_section_handle(wasm_exec_env_t exec_env, const char *section_name)
{
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    wasm_module_t module = wasm_runtime_get_module(module_inst);
    const uint8_t *content = NULL;
    uint32_t length = 0;
    uint32_t i;

    if (!section_name || section_name[0] == '\0') {
        printf("custom section name is empty\n");
        return -1;
    }

    content = wasm_runtime_get_custom_section(module, section_name, &length);
    if (!content) {
        printf("custom section [%s] not found\n", section_name);
        return -1;
    }

    for (i = 0; i < MAX_CUSTOM_SECTION_HANDLES; i++) {
        if (custom_section_handles[i].content == content
            && custom_section_handles[i].module == module
            && custom_section_handles[i].length == length) {
            return (int32_t)i;
        }
    }

    for (i = 0; i < MAX_CUSTOM_SECTION_HANDLES; i++) {
        if (!custom_section_handles[i].content) {
            custom_section_handles[i].module = module;
            custom_section_handles[i].content = content;
            custom_section_handles[i].length = length;
            printf("resolved custom section [%s] to handle %u (%u bytes)\n",
                   section_name, i, length);
            return (int32_t)i;
        }
    }

    printf("no free custom section handle slots remain\n");
    return -1;
}

void
print_custom_section(wasm_exec_env_t exec_env, int32_t handle)
{
    wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(exec_env);
    wasm_module_t module = wasm_runtime_get_module(module_inst);
    CustomSectionHandle *section_handle = NULL;

    if (handle < 0 || handle >= MAX_CUSTOM_SECTION_HANDLES) {
        printf("invalid custom section handle %d\n", handle);
        return;
    }

    section_handle = &custom_section_handles[handle];
    if (!section_handle->content || section_handle->module != module) {
        printf("custom section handle %d is not valid for this module\n",
               handle);
        return;
    }

    printf("custom section payload (%u bytes):\n", section_handle->length);
    fwrite(section_handle->content, 1, section_handle->length, stdout);
    if (section_handle->length == 0
        || section_handle->content[section_handle->length - 1] != '\n') {
        putchar('\n');
    }
}
