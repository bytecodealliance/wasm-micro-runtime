/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "ems_gc_internal.h"

static gc_handle_t
gc_init_internal(gc_heap_t *heap, char *base_addr, gc_size_t heap_max_size)
{
    hmu_tree_node_t *root = NULL, *q = NULL;
    int ret;

    memset(heap, 0, sizeof *heap);
    memset(base_addr, 0, heap_max_size);

    ret = os_mutex_init(&heap->lock);
    if (ret != BHT_OK) {
        os_printf("[GC_ERROR]failed to init lock\n");
        return NULL;
    }

    /* init all data structures*/
    heap->current_size = heap_max_size;
    heap->base_addr = (gc_uint8 *)base_addr;
    heap->heap_id = (gc_handle_t)heap;

    heap->total_free_size = heap->current_size;
    heap->highmark_size = 0;
#if WASM_ENABLE_GC != 0
    heap->gc_threshold_factor = GC_DEFAULT_THRESHOLD_FACTOR;
    gc_update_threshold(heap);
#endif

    root = &heap->kfc_tree_root;
    memset(root, 0, sizeof *root);
    root->size = sizeof *root;
    hmu_set_ut(&root->hmu_header, HMU_FC);
    hmu_set_size(&root->hmu_header, sizeof *root);

    q = (hmu_tree_node_t *)heap->base_addr;
    memset(q, 0, sizeof *q);
    hmu_set_ut(&q->hmu_header, HMU_FC);
    hmu_set_size(&q->hmu_header, heap->current_size);

    hmu_mark_pinuse(&q->hmu_header);
    root->right = q;
    q->parent = root;
    q->size = heap->current_size;

    bh_assert(root->size <= HMU_FC_NORMAL_MAX_SIZE);

    return heap;
}

gc_handle_t
gc_init_with_pool(char *buf, gc_size_t buf_size)
{
    char *buf_end = buf + buf_size;
    char *buf_aligned = (char *)(((uintptr_t)buf + 7) & (uintptr_t)~7);
    char *base_addr = buf_aligned + sizeof(gc_heap_t);
    gc_heap_t *heap = (gc_heap_t *)buf_aligned;
    gc_size_t heap_max_size;

    if (buf_size < APP_HEAP_SIZE_MIN) {
        os_printf("[GC_ERROR]heap init buf size (%" PRIu32 ") < %" PRIu32 "\n",
                  buf_size, (uint32)APP_HEAP_SIZE_MIN);
        return NULL;
    }

    base_addr =
        (char *)(((uintptr_t)base_addr + 7) & (uintptr_t)~7) + GC_HEAD_PADDING;
    heap_max_size = (uint32)(buf_end - base_addr) & (uint32)~7;

#if WASM_ENABLE_MEMORY_TRACING != 0
    os_printf("Heap created, total size: %u\n", buf_size);
    os_printf("   heap struct size: %u\n", sizeof(gc_heap_t));
    os_printf("   actual heap size: %u\n", heap_max_size);
    os_printf("   padding bytes: %u\n",
              buf_size - sizeof(gc_heap_t) - heap_max_size);
#endif
    return gc_init_internal(heap, base_addr, heap_max_size);
}

gc_handle_t
gc_init_with_struct_and_pool(char *struct_buf, gc_size_t struct_buf_size,
                             char *pool_buf, gc_size_t pool_buf_size)
{
    gc_heap_t *heap = (gc_heap_t *)struct_buf;
    char *base_addr = pool_buf + GC_HEAD_PADDING;
    char *pool_buf_end = pool_buf + pool_buf_size;
    gc_size_t heap_max_size;

    if ((((uintptr_t)struct_buf) & 7) != 0) {
        os_printf("[GC_ERROR]heap init struct buf not 8-byte aligned\n");
        return NULL;
    }

    if (struct_buf_size < sizeof(gc_handle_t)) {
        os_printf("[GC_ERROR]heap init struct buf size (%" PRIu32 ") < %zu\n",
                  struct_buf_size, sizeof(gc_handle_t));
        return NULL;
    }

    if ((((uintptr_t)pool_buf) & 7) != 0) {
        os_printf("[GC_ERROR]heap init pool buf not 8-byte aligned\n");
        return NULL;
    }

    if (pool_buf_size < APP_HEAP_SIZE_MIN) {
        os_printf("[GC_ERROR]heap init buf size (%" PRIu32 ") < %u\n",
                  pool_buf_size, APP_HEAP_SIZE_MIN);
        return NULL;
    }

    heap_max_size = (uint32)(pool_buf_end - base_addr) & (uint32)~7;

#if WASM_ENABLE_MEMORY_TRACING != 0
    os_printf("Heap created, total size: %u\n",
              struct_buf_size + pool_buf_size);
    os_printf("   heap struct size: %u\n", sizeof(gc_heap_t));
    os_printf("   actual heap size: %u\n", heap_max_size);
    os_printf("   padding bytes: %u\n", pool_buf_size - heap_max_size);
#endif
    return gc_init_internal(heap, base_addr, heap_max_size);
}

int
gc_destroy_with_pool(gc_handle_t handle)
{
    gc_heap_t *heap = (gc_heap_t *)handle;
#if BH_ENABLE_GC_VERIFY != 0
    hmu_t *cur = (hmu_t *)heap->base_addr;
    hmu_t *end = (hmu_t *)((char *)heap->base_addr + heap->current_size);
    if (!heap->is_heap_corrupted
        && (hmu_t *)((char *)cur + hmu_get_size(cur)) != end) {
        os_printf("Memory leak detected:\n");
        gci_dump(heap);
#if WASM_ENABLE_SPEC_TEST != 0
        while (1) {
        }
#endif
    }
#endif
    os_mutex_destroy(&heap->lock);
    memset(heap->base_addr, 0, heap->current_size);
    memset(heap, 0, sizeof(gc_heap_t));
    return GC_SUCCESS;
}

uint32
gc_get_heap_struct_size()
{
    return sizeof(gc_heap_t);
}

static void
adjust_ptr(uint8 **p_ptr, intptr_t offset)
{
    if (*p_ptr)
        *p_ptr += offset;
}

int
gc_migrate(gc_handle_t handle, char *pool_buf_new, gc_size_t pool_buf_size)
{
    gc_heap_t *heap = (gc_heap_t *)handle;
    char *base_addr_new = pool_buf_new + GC_HEAD_PADDING;
    char *pool_buf_end = pool_buf_new + pool_buf_size;
    intptr_t offset = (uint8 *)base_addr_new - (uint8 *)heap->base_addr;
    hmu_t *cur = NULL, *end = NULL;
    hmu_tree_node_t *tree_node;
    gc_size_t heap_max_size, size;

    if ((((uintptr_t)pool_buf_new) & 7) != 0) {
        os_printf("[GC_ERROR]heap migrate pool buf not 8-byte aligned\n");
        return GC_ERROR;
    }

    heap_max_size = (uint32)(pool_buf_end - base_addr_new) & (uint32)~7;

    if (pool_buf_end < base_addr_new || heap_max_size < heap->current_size) {
        os_printf("[GC_ERROR]heap migrate invlaid pool buf size\n");
        return GC_ERROR;
    }

    if (offset == 0)
        return 0;

    if (heap->is_heap_corrupted) {
        os_printf("[GC_ERROR]Heap is corrupted, heap migrate failed.\n");
        return GC_ERROR;
    }

    heap->base_addr = (uint8 *)base_addr_new;
    adjust_ptr((uint8 **)&heap->kfc_tree_root.left, offset);
    adjust_ptr((uint8 **)&heap->kfc_tree_root.right, offset);
    adjust_ptr((uint8 **)&heap->kfc_tree_root.parent, offset);

    cur = (hmu_t *)heap->base_addr;
    end = (hmu_t *)((char *)heap->base_addr + heap->current_size);

    while (cur < end) {
        size = hmu_get_size(cur);

        if (size <= 0 || size > (uint32)((uint8 *)end - (uint8 *)cur)) {
            os_printf("[GC_ERROR]Heap is corrupted, heap migrate failed.\n");
            heap->is_heap_corrupted = true;
            return GC_ERROR;
        }

        if (hmu_get_ut(cur) == HMU_FC && !HMU_IS_FC_NORMAL(size)) {
            tree_node = (hmu_tree_node_t *)cur;
            adjust_ptr((uint8 **)&tree_node->left, offset);
            adjust_ptr((uint8 **)&tree_node->right, offset);
            if (tree_node->parent != &heap->kfc_tree_root)
                /* The root node belongs to heap structure,
                   it is fixed part and isn't changed. */
                adjust_ptr((uint8 **)&tree_node->parent, offset);
        }
        cur = (hmu_t *)((char *)cur + size);
    }

    if (cur != end) {
        os_printf("[GC_ERROR]Heap is corrupted, heap migrate failed.\n");
        heap->is_heap_corrupted = true;
        return GC_ERROR;
    }

    return 0;
}

bool
gc_is_heap_corrupted(gc_handle_t handle)
{
    gc_heap_t *heap = (gc_heap_t *)handle;

    return heap->is_heap_corrupted ? true : false;
}

#if BH_ENABLE_GC_VERIFY != 0
void
gci_verify_heap(gc_heap_t *heap)
{
    hmu_t *cur = NULL, *end = NULL;

    bh_assert(heap && gci_is_heap_valid(heap));
    cur = (hmu_t *)heap->base_addr;
    end = (hmu_t *)(heap->base_addr + heap->current_size);
    while (cur < end) {
        hmu_verify(heap, cur);
        cur = (hmu_t *)((gc_uint8 *)cur + hmu_get_size(cur));
    }
    bh_assert(cur == end);
}
#endif

void
gc_heap_stat(void *heap_ptr, gc_stat_t *stat)
{
    hmu_t *cur = NULL, *end = NULL;
    hmu_type_t ut;
    gc_size_t size;
    gc_heap_t *heap = (gc_heap_t *)heap_ptr;

    memset(stat, 0, sizeof(gc_stat_t));
    cur = (hmu_t *)heap->base_addr;
    end = (hmu_t *)((char *)heap->base_addr + heap->current_size);

    while (cur < end) {
        ut = hmu_get_ut(cur);
        size = hmu_get_size(cur);
        bh_assert(size > 0);

        if (ut == HMU_FC || ut == HMU_FM
            || (ut == HMU_VO && hmu_is_vo_freed(cur))
            || (ut == HMU_WO && !hmu_is_wo_marked(cur))) {
            if (ut == HMU_VO)
                stat->vo_free += size;
            if (ut == HMU_WO)
                stat->wo_free += size;
            stat->free += size;
            stat->free_block++;
            if (size / sizeof(int) < GC_HEAP_STAT_SIZE - 1)
                stat->free_sizes[size / sizeof(int)] += 1;
            else
                stat->free_sizes[GC_HEAP_STAT_SIZE - 1] += 1;
        }
        else {
            if (ut == HMU_VO)
                stat->vo_usage += size;
            if (ut == HMU_WO)
                stat->wo_usage += size;
            stat->usage += size;
            stat->usage_block++;
            if (size / sizeof(int) < GC_HEAP_STAT_SIZE - 1)
                stat->usage_sizes[size / sizeof(int)] += 1;
            else
                stat->usage_sizes[GC_HEAP_STAT_SIZE - 1] += 1;
        }

        cur = (hmu_t *)((char *)cur + size);
    }
}

void
gc_print_stat(void *heap_ptr, int verbose)
{
    gc_stat_t stat;
    int i;

    bh_assert(heap_ptr != NULL);
    gc_heap_t *heap = (gc_heap_t *)(heap_ptr);

    gc_heap_stat(heap, &stat);

    os_printf("# stat %s %x use %d free %d \n", "instance", heap, stat.usage,
              stat.free);
    os_printf("# stat %s %x wo_usage %d vo_usage %d \n", "instance", heap,
              stat.wo_usage, stat.vo_usage);
    os_printf("# stat %s %x wo_free %d vo_free %d \n", "instance", heap,
              stat.wo_free, stat.vo_free);
    os_printf("# stat gc %d size %d high %d\n", heap->total_gc_count,
              heap->total_free_size, heap->highmark_size);
    if (verbose) {
        os_printf("usage sizes: \n");
        for (i = 0; i < GC_HEAP_STAT_SIZE; i++)
            if (stat.usage_sizes[i])
                os_printf(" %d: %d; ", i * 4, stat.usage_sizes[i]);
        os_printf(" \n");
        os_printf("free sizes: \n");
        for (i = 0; i < GC_HEAP_STAT_SIZE; i++)
            if (stat.free_sizes[i])
                os_printf(" %d: %d; ", i * 4, stat.free_sizes[i]);
    }
}

void *
gc_heap_stats(void *heap_arg, uint32 *stats, int size)
{
    int i;
    gc_heap_t *heap = (gc_heap_t *)heap_arg;

    if (!gci_is_heap_valid(heap)) {
        for (i = 0; i < size; i++)
            stats[i] = 0;
        return NULL;
    }

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
            case GC_STAT_COUNT:
                stats[i] = heap->total_gc_count;
                break;
            case GC_STAT_TIME:
                stats[i] = heap->total_gc_time;
                break;
            default:
                break;
        }
    }

    return heap;
}

void
gc_traverse_tree(hmu_tree_node_t *node, gc_size_t *stats, int *n)
{
    if (!node)
        return;

    if (*n > 0)
        gc_traverse_tree(node->right, stats, n);

    if (*n > 0) {
        (*n)--;
        stats[*n] = node->size;
    }

    if (*n > 0)
        gc_traverse_tree(node->left, stats, n);
}

void
gc_show_stat(void *heap)
{

    uint32 stats[GC_STAT_MAX];

    heap = gc_heap_stats(heap, stats, GC_STAT_MAX);

    os_printf("\n[GC stats %x] %d %d %d %d %d\n", heap, stats[0], stats[1],
              stats[2], stats[3], stats[4]);
}

void
gc_show_fragment(void *heap_arg)
{
    int stats[3];
    int n = 3;
    gc_heap_t *heap = (gc_heap_t *)heap_arg;

    memset(stats, 0, n * sizeof(int));
    gct_vm_mutex_lock(&heap->lock);
    gc_traverse_tree(&(heap->kfc_tree_root), (gc_size_t *)stats, &n);
    gct_vm_mutex_unlock(&heap->lock);
    os_printf("\n[GC %x top sizes] %d %d %d\n", heap, stats[0], stats[1],
              stats[2]);
}
