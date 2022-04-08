/*
 * Copyright (C) 2022 Tencent Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#include "ems_gc_internal.h"
#include "ems_gc.h"

extern int
_vm_begin_rootset_enumeration(void *heap);

#define GB (1 << 30UL)

#ifdef GC_STAT
void
gc_heap_stat(void *heap_ptr, gc_stat_t *stat)
{
    hmu_t *cur = NULL, *end = NULL, *last = NULL;
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
__gc_print_stat(void *heap_ptr, int verbose)
{
    gc_stat_t stat;
    int i;

    bh_assert(heap_ptr != NULL);
    gc_heap_t *heap = (gc_heap_t *)(heap_ptr);

    gc_heap_stat(heap, &stat);

    printf("# stat %s %x use %d free %d \n", "instance", heap, stat.usage,
           stat.free);
    printf("# stat %s %x wo_usage %d vo_usage %d \n", "instance", heap,
           stat.wo_usage, stat.vo_usage);
    printf("# stat %s %x wo_free %d vo_free %d \n", "instance", heap,
           stat.wo_free, stat.vo_free);
    printf("# stat gc %d size %d high %d\n", heap->total_gc_count,
           heap->total_free_size, heap->highmark_size);
    if (verbose) {
        printf("usage sizes: \n");
        for (i = 0; i < GC_HEAP_STAT_SIZE; i++)
            if (stat.usage_sizes[i])
                printf(" %d: %d; ", i * 4, stat.usage_sizes[i]);
        printf(" \n");
        printf("free sizes: \n");
        for (i = 0; i < GC_HEAP_STAT_SIZE; i++)
            if (stat.free_sizes[i])
                printf(" %d: %d; ", i * 4, stat.free_sizes[i]);
    }
}
#endif

#if GC_STAT_DATA != 0

void *
gc_heap_stats(void *heap_arg, int *stats, int size, gc_mm_t mmt)
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
                stats[i] = (int)heap->total_gc_time;
                break;
            default:
                break;
        }
    }

    return heap;
}

#ifdef STAT_SHOW_GC
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

extern void
bh_log_emit(const char *fmt, va_list ap);

static void
gc_log_stat(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    bh_log_emit(fmt, ap);
    va_end(ap);
}

void
gc_show_stat(void *heap)
{

    int stats[GC_STAT_MAX];

    heap = gc_heap_stats(heap, stats, GC_STAT_MAX, MMT_INSTANCE);

    gc_log_stat("\n[GC stats %x] %d %d %d %d %d\n", heap, stats[0], stats[1],
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
    gc_log_stat("\n[GC %x top sizes] %d %d %d\n", heap, stats[0], stats[1],
                stats[2]);
}

#endif
#endif

/* Alloc a mark node from native heap*/

/* Return a valid mark node if successfull*/
/* Return NULL otherwise*/
static mark_node_t *
alloc_mark_node(void)
{
    mark_node_t *ret = (mark_node_t *)malloc(sizeof(mark_node_t));

    if (!ret) {
        LOG_ERROR("alloc a new mark node failed");
        return NULL;
    }
    ret->cnt = sizeof(ret->set) / sizeof(ret->set[0]);
    ret->idx = 0;
    ret->next = NULL;
    return ret;
}

/* Free a mark node*/

/* @node should not be NULL*/
/* @node should be a valid mark node allocated from native heap*/
static void
free_mark_node(mark_node_t *node)
{
    bh_assert(node);
    free((gc_object_t)node);
}

/* Sweep phase of mark_sweep algorithm*/

static void
wasm_runtime_gc_pre_sweep()
{}

/* @heap should be a valid instance heap which has already been marked*/
static void
sweep_instance_heap(gc_heap_t *heap)
{
    hmu_t *cur = NULL, *end = NULL, *last = NULL;
    hmu_type_t ut;
    gc_size_t size;
    int i;

#if GC_STAT_DATA != 0
    gc_size_t tot_free = 0;
#endif

#if WAMR_ENABLE_MEMORY_PROFILING != 0
    gc_size_t gc_freed_size = 0;
#endif

    bh_assert(gci_is_heap_valid(heap));

    cur = (hmu_t *)heap->base_addr;
    last = NULL;
    end = (hmu_t *)((char *)heap->base_addr + heap->current_size);

    /* reset KFC*/
    int lsize =
        (int)(sizeof(heap->kfc_normal_list) / sizeof(heap->kfc_normal_list[0]));
    for (i = 0; i < lsize; i++) {
        heap->kfc_normal_list[i].next = NULL;
    }
    heap->kfc_tree_root.right = NULL;
    heap->root_set = NULL;

    while (cur < end) {
        ut = hmu_get_ut(cur);
        size = hmu_get_size(cur);
        bh_assert(size > 0);

#if WAMR_ENABLE_MEMORY_PROFILING != 0
        if (ut == HMU_WO && !hmu_is_wo_marked(cur))
            gc_freed_size += size;
#endif

        if (ut == HMU_FC || ut == HMU_FM
            || (ut == HMU_VO && hmu_is_vo_freed(cur))
            || (ut == HMU_WO && !hmu_is_wo_marked(cur))) {
            /* merge previous free areas with current one*/
            if (!last)
                last = cur;
        }
        else {
            /* current block is still live*/
            if (last) {
#if GC_STAT_DATA != 0
                tot_free += (char *)cur - (char *)last;
#endif
                gci_add_fc(heap, last, (char *)cur - (char *)last);
                hmu_mark_pinuse(last);
                last = NULL;
            }

            if (ut == HMU_WO) {
                /* unmark it*/
                hmu_unmark_wo(cur);
            }
        }

        cur = (hmu_t *)((char *)cur + size);
    }

    bh_assert(cur == end);

    if (last) {
#if GC_STAT_DATA != 0
        tot_free += (char *)cur - (char *)last;
#endif
        gci_add_fc(heap, last, (char *)cur - (char *)last);
        hmu_mark_pinuse(last);
    }

#if GC_STAT_DATA != 0
    heap->total_gc_count++;
    heap->total_free_size = tot_free;
    if ((heap->current_size - tot_free) > heap->highmark_size)
        heap->highmark_size = heap->current_size - tot_free;

    gc_update_threshold(heap);
#endif

#if WAMR_ENABLE_MEMORY_PROFILING != 0
    LOG_PROFILE_HEAP_GC((unsigned)heap, gc_freed_size);
#endif
}

/* Add to-expand node to the to-expand list*/

/* @heap should be a valid instance heap*/
/* @obj should be a valid wo inside @heap*/

/* GC_ERROR will be returned if no more resource for marking*/
/* GC_SUCCESS will be returned otherwise*/
static int
add_wo_to_expand(gc_heap_t *heap, gc_object_t obj)
{
    mark_node_t *mark_node = NULL, *new_node = NULL;
    hmu_t *hmu = NULL;

    bh_assert(obj);

    hmu = obj_to_hmu(obj);

    bh_assert(gci_is_heap_valid(heap));
    bh_assert((gc_uint8 *)hmu >= heap->base_addr
              && (gc_uint8 *)hmu < heap->base_addr + heap->current_size);
    bh_assert(hmu_get_ut(hmu) == HMU_WO);

    if (hmu_is_wo_marked(hmu))
        return GC_SUCCESS; /* already marked*/

    mark_node = (mark_node_t *)heap->root_set;
    if (!mark_node || mark_node->idx == mark_node->cnt) {
        new_node = alloc_mark_node();
        if (!new_node) {
            LOG_ERROR("can not add obj to mark node because of mark node "
                      "allocation failed");
            return GC_ERROR;
        }
        new_node->next = mark_node;
        heap->root_set = new_node;
        mark_node = new_node;
    }

    mark_node->set[mark_node->idx++] = obj;
    hmu_mark_wo(hmu);
    return GC_SUCCESS;
}

/* Check ems_gc.h for description*/
int
gc_add_root(void *heap_p, gc_object_t obj)
{
    gc_heap_t *heap = (gc_heap_t *)heap_p;
    hmu_t *hmu = NULL;

    if (!obj) {
        LOG_ERROR("gc_add_root with NULL obj");
        return GC_ERROR;
    }

    hmu = obj_to_hmu(obj);

    if (!gci_is_heap_valid(heap)) {
        LOG_ERROR("vm_get_gc_handle_for_current_instance returns invalid heap");
        return GC_ERROR;
    }

    if (!((gc_uint8 *)hmu >= heap->base_addr
          && (gc_uint8 *)hmu < heap->base_addr + heap->current_size)) {
        LOG_ERROR("Obj is not a object in current instance heap");
        return GC_ERROR;
    }

    if (hmu_get_ut(hmu) != HMU_WO) {
        LOG_ERROR("Given objecti s not wo");
        return GC_ERROR;
    }

    if (add_wo_to_expand(heap, obj) != GC_SUCCESS) {
        heap->is_fast_marking_failed = 1;
        return GC_ERROR;
    }

    return GC_SUCCESS;
}

/* Unmark all marked objects to do rollback*/

/* @heap should be a valid instance heap*/
static void
rollback_mark(gc_heap_t *heap)
{
    mark_node_t *mark_node = NULL, *next_mark_node = NULL;
    hmu_t *cur = NULL, *end = NULL;
    hmu_type_t ut;
    gc_size_t size;

    bh_assert(gci_is_heap_valid(heap));

    /* roll back*/
    mark_node = (mark_node_t *)heap->root_set;
    while (mark_node) {
        next_mark_node = mark_node->next;
        free_mark_node(mark_node);
        mark_node = next_mark_node;
    }

    heap->root_set = NULL;

    /* then traverse the heap to unmark all marked wos*/

    cur = (hmu_t *)heap->base_addr;
    end = (hmu_t *)((char *)heap->base_addr + heap->current_size);

    while (cur < end) {
        ut = hmu_get_ut(cur);
        size = hmu_get_size(cur);

        if (ut == HMU_WO && hmu_is_wo_marked(cur)) {
            hmu_unmark_wo(cur);
        }

        cur = (hmu_t *)((char *)cur + size);
    }

    bh_assert(cur == end);
}

/* GC instance heap*/

/* @heap should be a valid instance heap*/

/* GC_SUCCESS will be returned if everything goes well.*/
/* GC_ERROR will be returned otherwise.*/
static int
reclaim_instance_heap(gc_heap_t *heap)
{
    mark_node_t *mark_node = NULL;
    int idx = 0, ret = GC_ERROR, j = 0, is_compact_mode = GC_FALSE;
    gc_object_t obj = NULL, ref = NULL;
    hmu_t *hmu = NULL;
    gc_uint32 ref_num = 0, ref_start_offset = 0, size = 0, offset = 0;
    gc_uint16 *ref_list = NULL;

    bh_assert(gci_is_heap_valid(heap));

    heap->root_set = NULL;
    ret = gct_vm_begin_rootset_enumeration(heap);
    if (ret != GC_SUCCESS)
        return ret;

#if BH_GC_VERIFY != 0
    /* no matter whether the enumeration is successful or not, the data
     * collected should be checked at first*/
    mark_node = (mark_node_t *)heap->root_set;
    while (mark_node) {
        /* all nodes except first should be full filled*/
        bh_assert(mark_node == (mark_node_t *)heap->root_set
                  || mark_node->idx == mark_node->cnt);

        /* all nodes should be non-empty*/
        bh_assert(mark_node->idx > 0);

        for (idx = 0; idx < mark_node->idx; idx++) {
            obj = mark_node->set[idx];
            hmu = obj_to_hmu(obj);
            bh_assert(hmu_is_wo_marked(hmu));
            bh_assert((gc_uint8 *)hmu >= heap->base_addr
                      && (gc_uint8 *)hmu
                             < heap->base_addr + heap->current_size);
        }

        mark_node = mark_node->next;
    }
#endif
    /* TODO: when fast marking failed, we can still do slow
       marking.  Currently just simply roll it back.  */
    if (heap->is_fast_marking_failed) {
        LOG_ERROR("enumerate rootset failed");
        LOG_ERROR("all marked wos will be unmarked to keep heap consistency");

        rollback_mark(heap);
        heap->is_fast_marking_failed = 0;
        return GC_ERROR;
    }

    /* the algorithm we use to mark all objects*/
    /* 1. mark rootset and organize them into a mark_node list (last marked
     * roots at list header, i.e. stack top)*/
    /* 2. in every iteration, we use the top node to expand*/
    /* 3. execute step 2 till no expanding*/
    /* this is a BFS & DFS mixed algorithm, but more like DFS*/
    mark_node = (mark_node_t *)heap->root_set;
    while (mark_node) {
        heap->root_set = mark_node->next;

        /* note that mark_node->idx may change in each loop*/
        for (idx = 0; idx < (int)mark_node->idx; idx++) {
            obj = mark_node->set[idx];
            hmu = obj_to_hmu(obj);
            size = hmu_get_size(hmu);

            if (gct_vm_get_wasm_object_ref_list(obj, &is_compact_mode, &ref_num,
                                                &ref_list, &ref_start_offset)
                == GC_ERROR) {
                LOG_ERROR("mark process failed because failed "
                          "vm_get_wasm_object_ref_list");
                break;
            }

            if (ref_num >= 2U * GB) {
                LOG_ERROR("Invalid ref_num returned");
                break;
            }

            if (is_compact_mode) {
                for (j = 0; j < (int)ref_num; j++) {
                    offset = ref_start_offset + j * 4;
                    bh_assert(offset >= 0 && offset + 4 < size);
                    ref = *(gc_object_t *)(((gc_uint8 *)obj) + offset);
                    if (ref == NULL_REF)
                        continue; /* NULL REF*/
                    if (add_wo_to_expand(heap, ref) == GC_ERROR) {
                        LOG_ERROR("add_wo_to_expand failed");
                        break;
                    }
                }
                if (j < (int)ref_num)
                    break;
            }
            else {
                for (j = 0; j < (int)ref_num; j++) {
                    offset = ref_list[j];
                    bh_assert(offset + 4 < size);

                    ref = *(gc_object_t *)(((gc_uint8 *)obj) + offset);
                    if (ref == NULL_REF)
                        continue; /* NULL REF*/
                    if (add_wo_to_expand(heap, ref) == GC_ERROR) {
                        LOG_ERROR("mark process failed");
                        break;
                    }
                }
                if (j < (int)ref_num)
                    break;
            }
        }
        if (idx < (int)mark_node->idx)
            break; /* not yet done*/

        /* obj's in mark_node are all expanded*/
        free_mark_node(mark_node);
        mark_node = heap->root_set;
    }

    if (mark_node) {
        LOG_ERROR("mark process is not successfully finished");

        free_mark_node(mark_node);
        /* roll back is required*/
        rollback_mark(heap);

        return GC_ERROR;
    }

    /* mark finished*/
    wasm_runtime_gc_pre_sweep();

    /* now sweep*/
    sweep_instance_heap(heap);

    (void)size;

    return GC_SUCCESS;
}

/* Do GC on given heap*/

/* @heap should not be NULL and it should be a valid heap*/

/* GC_ERROR returned for failure*/
/* GC_SUCCESS otherwise*/
int
gci_gc_heap(void *h)
{
    int ret = GC_ERROR;
    gc_heap_t *heap = (gc_heap_t *)h;

    bh_assert(gci_is_heap_valid(heap));

    LOG_VERBOSE("#reclaim instance heap %x", heap);
    // gc_print_stat(heap, 0);
    gct_vm_gc_prepare();
    gct_vm_mutex_lock(&heap->lock);
    heap->is_doing_reclaim = 1;
    ret = reclaim_instance_heap(heap);
    heap->is_doing_reclaim = 0;
    gct_vm_mutex_unlock(&heap->lock);
    gct_vm_gc_finished();
    LOG_VERBOSE("#reclaim instance heap %x done", heap);

#if BH_ENABLE_GC_VERIFY
    gci_verify_heap(heap);
#endif

#ifdef STAT_SHOW_GC
    gc_show_stat(heap);
    gc_show_fragment(heap);
#endif

    return ret;
}

int
gc_is_dead_object(void *obj)
{
    return !hmu_is_wo_marked(obj_to_hmu(obj));
}

#ifdef GC_TEST
gc_size_t (*gct_vm_get_wasm_object_size)(gc_object_t obj);
int (*gct_vm_get_wasm_object_ref_list)(
    gc_object_t obj,
    int *is_compact_mode, /* can be set to GC_TRUE, or GC_FALSE */
    gc_size_t *ref_num, gc_uint16 **ref_list, gc_uint32 *ref_start_offset);
int (*gct_vm_mutex_init)(korp_mutex *mutex);
int (*gct_vm_mutex_destroy)(korp_mutex *mutex);
int (*gct_vm_mutex_lock)(korp_mutex *mutex);
int (*gct_vm_mutex_unlock)(korp_mutex *mutex);
int (*gct_vm_begin_rootset_enumeration)();
int (*gct_vm_gc_prepare)();
int (*gct_vm_gc_finished)();
#else
int
vm_begin_rootset_enumeration(void *heap)
{
    os_printf("Error: Unimplemented vm_begin_rootset_enumeration function!\n");
    return GC_ERROR;
}

int
vm_get_wasm_object_ref_list(
    gc_object_t obj,
    int *is_compact_mode, /* can be set to GC_TRUE, or GC_FALSE */
    gc_size_t *ref_num, gc_uint16 **ref_list, gc_uint32 *ref_start_offset)
{
    os_printf(
        "Error: Unimplemented gct_vm_get_wasm_object_ref_list function!\n");
    return GC_ERROR;
}

void
wasm_runtime_gc_prepare(){};
void
wasm_runtime_gc_finished(){};
#endif
