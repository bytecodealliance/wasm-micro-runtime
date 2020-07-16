/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#if WASM_ENABLE_SHARED_MEMORY != 0

#include "bh_log.h"
#include "wasm_shared_memory.h"

static bh_list shared_memory_list_head;
static bh_list *const shared_memory_list = &shared_memory_list_head;
static korp_mutex shared_memory_list_lock;

bool
wasm_shared_memory_init()
{
    if (os_mutex_init(&shared_memory_list_lock) != 0)
        return false;
    return true;
}

void
wasm_shared_memory_destroy()
{
    os_mutex_destroy(&shared_memory_list_lock);
}

static WASMSharedMemNode*
search_module(WASMModuleCommon *module)
{
    WASMSharedMemNode *node;

    os_mutex_lock(&shared_memory_list_lock);
    node = bh_list_first_elem(shared_memory_list);

    while (node) {
        if (module == node->module) {
            os_mutex_unlock(&shared_memory_list_lock);
            return node;
        }
        node = bh_list_elem_next(node);
    }

    os_mutex_unlock(&shared_memory_list_lock);
    return NULL;
}

WASMSharedMemNode*
wasm_module_get_shared_memory(WASMModuleCommon *module)
{
    return search_module(module);
}

int32
shared_memory_inc_reference(WASMModuleCommon *module)
{
    WASMSharedMemNode *node = search_module(module);
    if (node) {
        os_mutex_lock(&node->lock);
        node->ref_count++;
        os_mutex_unlock(&node->lock);
        return node->ref_count;
    }
    return -1;
}

int32
shared_memory_dec_reference(WASMModuleCommon *module)
{
    WASMSharedMemNode *node = search_module(module);
    uint32 ref_count = 0;
    if (node) {
        os_mutex_lock(&node->lock);
        ref_count = --node->ref_count;
        os_mutex_unlock(&node->lock);
        if (ref_count == 0) {
            os_mutex_lock(&shared_memory_list_lock);
            bh_list_remove(shared_memory_list, node);
            os_mutex_unlock(&shared_memory_list_lock);

            os_mutex_destroy(&node->lock);
            wasm_runtime_free(node);
        }
        return ref_count;
    }

    return -1;
}

WASMMemoryInstanceCommon*
shared_memory_get_memory_inst(WASMSharedMemNode *node)
{
    return node->memory_inst;
}

WASMSharedMemNode*
shared_memory_set_memory_inst(WASMModuleCommon *module,
                              WASMMemoryInstanceCommon *memory)
{
    WASMSharedMemNode *node;
    bh_list_status ret;

    if (!(node = wasm_runtime_malloc(sizeof(WASMSharedMemNode))))
        return NULL;

    node->module = module;
    node->memory_inst = memory;
    node->ref_count = 1;
    if (os_mutex_init(&node->lock) != 0) {
        wasm_runtime_free(node);
        return NULL;
    }

    os_mutex_lock(&shared_memory_list_lock);
    ret = bh_list_insert(shared_memory_list, node);
    bh_assert(ret == BH_LIST_SUCCESS);
    os_mutex_unlock(&shared_memory_list_lock);

    (void)ret;
    return node;
}

#endif /* end of WASM_ENABLE_SHARED_MEMORY */
