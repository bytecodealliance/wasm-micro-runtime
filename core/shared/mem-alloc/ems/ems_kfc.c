/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "ems_gc_internal.h"

gc_handle_t
gc_init_with_pool(char *buf, gc_size_t buf_size)
{
    char *buf_end = buf + buf_size;
    char *buf_aligned = (char*)(((uintptr_t) buf + 7) & (uintptr_t)~7);
    char *base_addr = buf_aligned + sizeof(gc_heap_t);
    gc_heap_t *heap = (gc_heap_t*)buf_aligned;
    gc_size_t heap_max_size;
    hmu_normal_node_t *p = NULL;
    hmu_tree_node_t *root = NULL, *q = NULL;
    int i = 0, ret;

    if (buf_size < APP_HEAP_SIZE_MIN) {
        os_printf("[GC_ERROR]heap init buf size (%u) < %u\n",
                  buf_size, APP_HEAP_SIZE_MIN);
        return NULL;
    }

    base_addr = (char*) (((uintptr_t) base_addr + 7) & (uintptr_t)~7) + GC_HEAD_PADDING;
    heap_max_size = (uint32)(buf_end - base_addr) & (uint32)~7;

    memset(heap, 0, sizeof *heap);
    memset(base_addr, 0, heap_max_size);

    ret = os_mutex_init(&heap->lock);
    if (ret != BHT_OK) {
        os_printf("[GC_ERROR]failed to init lock\n");
        return NULL;
    }

    /* init all data structures*/
    heap->current_size = heap_max_size;
    heap->base_addr = (gc_uint8*)base_addr;
    heap->heap_id = (gc_handle_t)heap;

    heap->total_free_size = heap->current_size;
    heap->highmark_size = 0;

    for (i = 0; i < HMU_NORMAL_NODE_CNT; i++) {
        /* make normal node look like a FC*/
        p = &heap->kfc_normal_list[i];
        memset(p, 0, sizeof *p);
        hmu_set_ut(&p->hmu_header, HMU_FC);
        hmu_set_size(&p->hmu_header, sizeof *p);
    }

    root = &heap->kfc_tree_root;
    memset(root, 0, sizeof *root);
    root->size = sizeof *root;
    hmu_set_ut(&root->hmu_header, HMU_FC);
    hmu_set_size(&root->hmu_header, sizeof *root);

    q = (hmu_tree_node_t *) heap->base_addr;
    memset(q, 0, sizeof *q);
    hmu_set_ut(&q->hmu_header, HMU_FC);
    hmu_set_size(&q->hmu_header, heap->current_size);

    hmu_mark_pinuse(&q->hmu_header);
    root->right = q;
    q->parent = root;
    q->size = heap->current_size;

    bh_assert(root->size <= HMU_FC_NORMAL_MAX_SIZE);

#if WASM_ENABLE_MEMORY_TRACING != 0
    os_printf("Heap created, total size: %u\n", buf_size);
    os_printf("   heap struct size: %u\n", sizeof(gc_heap_t));
    os_printf("   actual heap size: %u\n", heap_max_size);
    os_printf("   padding bytes: %u\n",
              buf_size - sizeof(gc_heap_t) - heap_max_size);
#endif
    return heap;
}

int
gc_destroy_with_pool(gc_handle_t handle)
{
    gc_heap_t *heap = (gc_heap_t *) handle;
#if BH_ENABLE_GC_VERIFY != 0
    hmu_t *cur = (hmu_t*)heap->base_addr;
    hmu_t *end = (hmu_t*)((char*)heap->base_addr + heap->current_size);
    if ((hmu_t*)((char *)cur + hmu_get_size(cur)) != end) {
        os_printf("Memory leak detected:\n");
        gci_dump(heap);
#if WASM_ENABLE_SPEC_TEST != 0
        while (1);
#endif
    }
#endif
    os_mutex_destroy(&heap->lock);
    memset(heap->base_addr, 0, heap->current_size);
    memset(heap, 0, sizeof(gc_heap_t));
    return GC_SUCCESS;
}

static void
adjust_ptr(uint8 **p_ptr, intptr_t offset)
{
    if (*p_ptr)
        *p_ptr += offset;
}

int
gc_migrate(gc_handle_t handle, gc_handle_t handle_old)
{
    gc_heap_t *heap = (gc_heap_t *) handle;
    intptr_t offset = (uint8*)handle - (uint8*)handle_old;
    hmu_t *cur = NULL, *end = NULL;
    hmu_tree_node_t *tree_node;
    gc_size_t size;

    os_mutex_init(&heap->lock);

    if (offset == 0)
        return 0;

    heap->heap_id = (gc_handle_t)heap;
    heap->base_addr += offset;
    adjust_ptr((uint8**)&heap->kfc_tree_root.left, offset);
    adjust_ptr((uint8**)&heap->kfc_tree_root.right, offset);
    adjust_ptr((uint8**)&heap->kfc_tree_root.parent, offset);

    cur = (hmu_t*)heap->base_addr;
    end = (hmu_t*)((char*)heap->base_addr + heap->current_size);

    while (cur < end) {
        size = hmu_get_size(cur);
        bh_assert(size > 0);

        if (hmu_get_ut(cur) == HMU_FC && !HMU_IS_FC_NORMAL(size)) {
            tree_node = (hmu_tree_node_t *)cur;
            adjust_ptr((uint8**)&tree_node->left, offset);
            adjust_ptr((uint8**)&tree_node->right, offset);
            adjust_ptr((uint8**)&tree_node->parent, offset);
        }
        cur = (hmu_t*)((char *)cur + size);
    }

    bh_assert(cur == end);
    return 0;
}

int
gc_reinit_lock(gc_handle_t handle)
{
    gc_heap_t *heap = (gc_heap_t *) handle;
    return os_mutex_init(&heap->lock);
}

void
gc_destroy_lock(gc_handle_t handle)
{
    gc_heap_t *heap = (gc_heap_t *) handle;
    os_mutex_destroy(&heap->lock);
}

#if BH_ENABLE_GC_VERIFY != 0
void
gci_verify_heap(gc_heap_t *heap)
{
    hmu_t *cur = NULL, *end = NULL;

    bh_assert(heap && gci_is_heap_valid(heap));
    cur = (hmu_t *)heap->base_addr;
    end = (hmu_t *)(heap->base_addr + heap->current_size);
    while(cur < end)
    {
        hmu_verify(cur);
        cur = (hmu_t *)((gc_uint8*)cur + hmu_get_size(cur));
    }
    bh_assert(cur == end);
}
#endif

void *
gc_heap_stats(void *heap_arg, uint32* stats, int size)
{
    int i;
    gc_heap_t *heap = (gc_heap_t *) heap_arg;

    for (i = 0; i < size; i++) {
        switch (i) {
        case GC_STAT_TOTAL:
            stats[i] = heap->current_size;
            break;
        case GC_STAT_FREE:
            stats[i] = heap->total_free_size;
            break;
        case GC_STAT_HIGHMARK:
            stats[i] = heap->highmark_size;
            break;
        default:
            break;
        }
    }
    return heap;
}

