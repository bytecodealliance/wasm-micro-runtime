/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "ems_gc_internal.h"


#define HEAP_INC_FACTOR 1

/* Check if current platform is compatible with current GC design*/

/* Return GC_ERROR if not;*/
/* Return GC_SUCCESS otherwise.*/
int gci_check_platform()
{
#define CHECK(x, y)  do {                                       \
  if((x) != (y)) {                                              \
    os_printf("Platform checking failed on LINE %d at FILE %s.",\
           __LINE__, __FILE__);                                 \
    return GC_ERROR;                                            \
  }                                                             \
} while(0)

    CHECK(8, sizeof(gc_int64));
    CHECK(4, sizeof(gc_uint32));
    CHECK(4, sizeof(gc_int32));
    CHECK(2, sizeof(gc_uint16));
    CHECK(2, sizeof(gc_int16));
    CHECK(1, sizeof(gc_int8));
    CHECK(1, sizeof(gc_uint8));
    CHECK(4, sizeof(gc_size_t));
    /*CHECK(4, sizeof(void *));*/

    return GC_SUCCESS;
}

gc_handle_t gc_init_with_pool(char *buf, gc_size_t buf_size)
{
    char *buf_end = buf + buf_size;
    char *buf_aligned = (char*) (((uintptr_t) buf + 7) & (uintptr_t)~7);
    char *base_addr = buf_aligned + sizeof(gc_heap_t);
    gc_heap_t *heap = (gc_heap_t*) buf_aligned;
    gc_size_t heap_max_size;
    hmu_normal_node_t *p = NULL;
    hmu_tree_node_t *root = NULL, *q = NULL;
    int i = 0, ret;

    /* check system compatibility*/
    if (gci_check_platform() == GC_ERROR) {
        os_printf("Check platform compatibility failed");
        return NULL;
    }

    if (buf_size < 1024) {
        os_printf("[GC_ERROR]heap_init_size(%d) < 1024", buf_size);
        return NULL;
    }

    base_addr = (char*) (((uintptr_t) base_addr + 7) & (uintptr_t)~7) + GC_HEAD_PADDING;
    heap_max_size = (uint32)(buf_end - base_addr) & (uint32)~7;

    memset(heap, 0, sizeof *heap);
    memset(base_addr, 0, heap_max_size);

    ret = gct_vm_mutex_init(&heap->lock);
    if (ret != BHT_OK) {
        os_printf("[GC_ERROR]failed to init lock ");
        return NULL;
    }

#ifdef BH_FOOTPRINT
    os_printf("\nINIT HEAP 0x%08x %d\n", base_addr, heap_max_size);
#endif

    /* init all data structures*/
    heap->max_size = heap_max_size;
    heap->current_size = heap_max_size;
    heap->base_addr = (gc_uint8*) base_addr;
    heap->heap_id = (gc_handle_t) heap;

#if GC_STAT_DATA != 0
    heap->total_free_size = heap->current_size;
    heap->highmark_size = 0;
    heap->total_gc_count = 0;
    heap->total_gc_time = 0;
    heap->gc_threshold_factor = GC_DEFAULT_THRESHOLD_FACTOR;
    gc_update_threshold(heap);
#endif

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

    bh_assert(
            root->size <= HMU_FC_NORMAL_MAX_SIZE
                    && HMU_FC_NORMAL_MAX_SIZE < q->size); /*@NOTIFY*/

#if BH_ENABLE_MEMORY_PROFILING != 0
    os_printf("heap is successfully initialized with max_size=%u.",
              heap_max_size);
#endif
    return heap;
}

int gc_destroy_with_pool(gc_handle_t handle)
{
    gc_heap_t *heap = (gc_heap_t *) handle;
    gct_vm_mutex_destroy(&heap->lock);
    memset(heap->base_addr, 0, heap->max_size);
    memset(heap, 0, sizeof(gc_heap_t));
    return GC_SUCCESS;
}

#if defined(GC_VERIFY)
/* Verify heap integrity*/
/* @heap should not be NULL and it should be a valid heap*/
void gci_verify_heap(gc_heap_t *heap)
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

void* gc_heap_stats(void *heap_arg, uint32* stats, int size, gc_mm_t mmt)
{
    (void) mmt;
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
        case GC_STAT_COUNT:
            stats[i] = heap->total_gc_count;
            break;
        case GC_STAT_TIME:
            stats[i] = (uint32)heap->total_gc_time;
            break;
        default:
            break;
        }
    }
    return heap;
}
