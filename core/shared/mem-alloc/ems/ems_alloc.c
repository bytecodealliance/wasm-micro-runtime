/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "ems_gc_internal.h"


static int
hmu_is_in_heap(gc_heap_t* heap, hmu_t* hmu)
{
    return heap && hmu
           && (gc_uint8*) hmu >= heap->base_addr
           && (gc_uint8*) hmu < heap->base_addr + heap->current_size;
}

/**
 * Remove a node from the tree it belongs to
 *
 * @param p the node to remove, can not be NULL, can not be the ROOT node
 *        the node will be removed from the tree, and the left, right and
 *        parent pointers of the node @p will be set to be NULL. Other fields
 *        won't be touched. The tree will be re-organized so that the order
 *        conditions are still satisified.
 */
static void
remove_tree_node(hmu_tree_node_t *p)
{
    hmu_tree_node_t *q = NULL, **slot = NULL;

    bh_assert(p);
    bh_assert(p->parent); /* @p can not be the ROOT node*/

    /* get the slot which holds pointer to node p*/
    if (p == p->parent->right) {
        slot = &p->parent->right;
    } else {
        bh_assert(p == p->parent->left); /* @p should be a child of its parent*/
        slot = &p->parent->left;
    }

    /**
     * algorithms used to remove node p
     * case 1: if p has no left child, replace p with its right child
     * case 2: if p has no right child, replace p with its left child
     * case 3: otherwise, find p's predecessor, remove it from the tree
     *         and replace p with it.
     * use predecessor can keep the left <= root < right condition.
     */

    if (!p->left) {
        /* move right child up*/
        *slot = p->right;
        if (p->right)
            p->right->parent = p->parent;

        p->left = p->right = p->parent = NULL;
        return;
    }

    if (!p->right) {
        /* move left child up*/
        *slot = p->left;
        p->left->parent = p->parent; /* p->left can never be NULL.*/

        p->left = p->right = p->parent = NULL;
        return;
    }

    /* both left & right exist, find p's predecessor at first*/
    q = p->left;
    while (q->right)
        q = q->right;
    /* remove from the tree*/
    remove_tree_node(q);

    *slot = q;
    q->parent = p->parent;
    q->left = p->left;
    q->right = p->right;
    if (q->left)
        q->left->parent = q;
    if (q->right)
        q->right->parent = q;

    p->left = p->right = p->parent = NULL;
}

static void
unlink_hmu(gc_heap_t *heap, hmu_t *hmu)
{
    gc_size_t size;

    bh_assert(gci_is_heap_valid(heap));
    bh_assert(hmu && (gc_uint8*) hmu >= heap->base_addr
              && (gc_uint8*) hmu < heap->base_addr + heap->current_size);
    bh_assert(hmu_get_ut(hmu) == HMU_FC);

    size = hmu_get_size(hmu);

    if (HMU_IS_FC_NORMAL(size)) {
        uint32 node_idx = size >> 3;
        hmu_normal_node_t *node_prev = &heap->kfc_normal_list[node_idx];
        hmu_normal_node_t *node =
            get_hmu_normal_node_next(&heap->kfc_normal_list[node_idx]);
        while (node) {
            if ((hmu_t*) node == hmu) {
                set_hmu_normal_node_next(node_prev, get_hmu_normal_node_next(node));
                break;
            }
            node_prev = node;
            node = get_hmu_normal_node_next(node);
        }

        if (!node) {
            os_printf("[GC_ERROR]couldn't find the node in the normal list\n");
        }
    }
    else {
        remove_tree_node((hmu_tree_node_t *) hmu);
    }
}

static void
hmu_set_free_size(hmu_t *hmu)
{
    gc_size_t size;
    bh_assert(hmu && hmu_get_ut(hmu) == HMU_FC);

    size = hmu_get_size(hmu);
    *((uint32*)((char*) hmu + size) - 1) = size;
}

/**
 * Add free chunk back to KFC
 *
 * @param heap should not be NULL and it should be a valid heap
 * @param hmu should not be NULL and it should be a HMU of length @size inside @heap
 *        hmu should be 8-bytes aligned
 * @param size should be positive and multiple of 8
 *        hmu with size @size will be added into KFC as a new FC.
 */
void
gci_add_fc(gc_heap_t *heap, hmu_t *hmu, gc_size_t size)
{
    hmu_normal_node_t *np = NULL;
    hmu_tree_node_t *root = NULL, *tp = NULL, *node = NULL;
    uint32 node_idx;

    bh_assert(gci_is_heap_valid(heap));
    bh_assert(hmu && (gc_uint8*) hmu >= heap->base_addr
              && (gc_uint8*) hmu < heap->base_addr + heap->current_size);
    bh_assert(((gc_uint32)(uintptr_t)hmu_to_obj(hmu) & 7) == 0);
    bh_assert(size > 0
              && ((gc_uint8*) hmu) + size <= heap->base_addr + heap->current_size);
    bh_assert(!(size & 7));

    hmu_set_ut(hmu, HMU_FC);
    hmu_set_size(hmu, size);
    hmu_set_free_size(hmu);

    if (HMU_IS_FC_NORMAL(size)) {
        np = (hmu_normal_node_t*) hmu;

        node_idx = size >> 3;
        set_hmu_normal_node_next(np, get_hmu_normal_node_next
                                        (&heap->kfc_normal_list[node_idx]));
        set_hmu_normal_node_next(&heap->kfc_normal_list[node_idx], np);
        return;
    }

    /* big block*/
    node = (hmu_tree_node_t*) hmu;
    node->size = size;
    node->left = node->right = node->parent = NULL;

    /* find proper node to link this new node to*/
    root = &heap->kfc_tree_root;
    tp = root;
    bh_assert(tp->size < size);
    while (1) {
        if (tp->size < size) {
            if (!tp->right) {
                tp->right = node;
                node->parent = tp;
                break;
            }
            tp = tp->right;
        }
        else { /* tp->size >= size*/
            if (!tp->left) {
                tp->left = node;
                node->parent = tp;
                break;
            }
            tp = tp->left;
        }
    }
}

/**
 * Find a proper hmu for required memory size
 *
 * @param heap should not be NULL and should be a valid heap
 * @param size should cover the header and should be 8 bytes aligned
 *        GC will not be performed here.
 *        Heap extension will not be performed here.
 *
 * @return hmu allocated if success, which will be aligned to 8 bytes,
 *         NULL otherwise
 */
static hmu_t *
alloc_hmu(gc_heap_t *heap, gc_size_t size)
{
    hmu_normal_node_t *node = NULL, *p = NULL;
    uint32 node_idx = 0, init_node_idx = 0;
    hmu_tree_node_t *root = NULL, *tp = NULL, *last_tp = NULL;
    hmu_t *next, *rest;

    bh_assert(gci_is_heap_valid(heap));
    bh_assert(size > 0 && !(size & 7));

    if (size < GC_SMALLEST_SIZE)
        size = GC_SMALLEST_SIZE;

    /* check normal list at first*/
    if (HMU_IS_FC_NORMAL(size)) {
        /* find a non-empty slot in normal_node_list with good size*/
        init_node_idx = (size >> 3);
        for (node_idx = init_node_idx; node_idx < HMU_NORMAL_NODE_CNT;
                node_idx++) {
            node = heap->kfc_normal_list + node_idx;
            if (get_hmu_normal_node_next(node))
                break;
            node = NULL;
        }

        /* not found in normal list*/
        if (node) {
            bh_assert(node_idx >= init_node_idx);

            p = get_hmu_normal_node_next(node);
            set_hmu_normal_node_next(node, get_hmu_normal_node_next(p));
            bh_assert(((gc_int32)(uintptr_t)hmu_to_obj(p) & 7) == 0);

            if ((gc_size_t)node_idx != (uint32)init_node_idx
                /* with bigger size*/
                && ((gc_size_t)node_idx << 3) >= size + GC_SMALLEST_SIZE) {
                rest = (hmu_t*) (((char *) p) + size);
                gci_add_fc(heap, rest, (node_idx << 3) - size);
                hmu_mark_pinuse(rest);
            }
            else {
                size = node_idx << 3;
                next = (hmu_t*) ((char*) p + size);
                if (hmu_is_in_heap(heap, next))
                    hmu_mark_pinuse(next);
            }

            heap->total_free_size -= size;
            if ((heap->current_size - heap->total_free_size)
                > heap->highmark_size)
                heap->highmark_size = heap->current_size
                                      - heap->total_free_size;

            hmu_set_size((hmu_t* )p, size);
            return (hmu_t*)p;
        }
    }

    /* need to find a node in tree*/
    root = &heap->kfc_tree_root;

    /* find the best node*/
    bh_assert(root);
    tp = root->right;
    while (tp) {
        if (tp->size < size) {
            tp = tp->right;
            continue;
        }

        /* record the last node with size equal to or bigger than given size*/
        last_tp = tp;
        tp = tp->left;
    }

    if (last_tp) {
        bh_assert(last_tp->size >= size);

        /* alloc in last_p*/

        /* remove node last_p from tree*/
        remove_tree_node(last_tp);

        if (last_tp->size >= size + GC_SMALLEST_SIZE) {
            rest = (hmu_t*) ((char*) last_tp + size);
            gci_add_fc(heap, rest, last_tp->size - size);
            hmu_mark_pinuse(rest);
        }
        else {
            size = last_tp->size;
            next = (hmu_t*) ((char*) last_tp + size);
            if (hmu_is_in_heap(heap, next))
                hmu_mark_pinuse(next);
        }

        heap->total_free_size -= size;
        if ((heap->current_size - heap->total_free_size) > heap->highmark_size)
            heap->highmark_size = heap->current_size - heap->total_free_size;

        hmu_set_size((hmu_t* ) last_tp, size);
        return (hmu_t*) last_tp;
    }

    return NULL;
}

/**
 * Find a proper HMU with given size
 *
 * @param heap should not be NULL and should be a valid heap
 * @param size should cover the header and should be 8 bytes aligned
 *
 * Note: This function will try several ways to satisfy the allocation request:
 *   1. Find a proper on available HMUs.
 *   2. GC will be triggered if 1 failed.
 *   3. Find a proper on available HMUS.
 *   4. Return NULL if 3 failed
 *
 * @return hmu allocated if success, which will be aligned to 8 bytes,
 *         NULL otherwise
 */
static hmu_t *
alloc_hmu_ex(gc_heap_t *heap, gc_size_t size)
{
    bh_assert(gci_is_heap_valid(heap));
    bh_assert(size > 0 && !(size & 7));

    return alloc_hmu(heap, size);
}

static unsigned long g_total_malloc = 0;
static unsigned long g_total_free = 0;

#if BH_ENABLE_GC_VERIFY == 0
gc_object_t
gc_alloc_vo(void *vheap, gc_size_t size)
#else
gc_object_t
gc_alloc_vo_internal(void *vheap, gc_size_t size,
                     const char *file, int line)
#endif
{
    gc_heap_t* heap = (gc_heap_t*) vheap;
    hmu_t *hmu = NULL;
    gc_object_t ret = (gc_object_t) NULL;
    gc_size_t tot_size = 0, tot_size_unaligned;

    /* hmu header + prefix + obj + suffix */
    tot_size_unaligned = HMU_SIZE + OBJ_PREFIX_SIZE + size + OBJ_SUFFIX_SIZE;
    /* aligned size*/
    tot_size = GC_ALIGN_8(tot_size_unaligned);
    if (tot_size < size)
        /* integer overflow */
        return NULL;

    os_mutex_lock(&heap->lock);

    hmu = alloc_hmu_ex(heap, tot_size);
    if (!hmu)
        goto finish;

    g_total_malloc += tot_size;

    hmu_set_ut(hmu, HMU_VO);
    hmu_unfree_vo(hmu);

#if BH_ENABLE_GC_VERIFY != 0
    hmu_init_prefix_and_suffix(hmu, tot_size, file, line);
#endif

    ret = hmu_to_obj(hmu);
    if (tot_size > tot_size_unaligned)
        /* clear buffer appended by GC_ALIGN_8() */
        memset((uint8*)ret + size, 0, tot_size - tot_size_unaligned);

finish:
    os_mutex_unlock(&heap->lock);
    return ret;
}

#if BH_ENABLE_GC_VERIFY == 0
gc_object_t
gc_realloc_vo(void *vheap, void *ptr, gc_size_t size)
#else
gc_object_t
gc_realloc_vo_internal(void *vheap, void *ptr, gc_size_t size,
                       const char *file, int line)
#endif
{
    gc_heap_t* heap = (gc_heap_t*) vheap;
    hmu_t *hmu = NULL, *hmu_old = NULL, *hmu_next;
    gc_object_t ret = (gc_object_t) NULL, obj_old = (gc_object_t)ptr;
    gc_size_t tot_size, tot_size_unaligned, tot_size_old = 0, tot_size_next;
    gc_size_t obj_size, obj_size_old;
    hmu_type_t ut;

    /* hmu header + prefix + obj + suffix */
    tot_size_unaligned = HMU_SIZE + OBJ_PREFIX_SIZE + size + OBJ_SUFFIX_SIZE;
    /* aligned size*/
    tot_size = GC_ALIGN_8(tot_size_unaligned);
    if (tot_size < size)
        /* integer overflow */
        return NULL;

    if (obj_old) {
        hmu_old = obj_to_hmu(obj_old);
        tot_size_old = hmu_get_size(hmu_old);
        if (tot_size <= tot_size_old)
            /* current node alreay meets requirement */
            return obj_old;
    }

    os_mutex_lock(&heap->lock);

    if (hmu_old) {
        hmu_next = (hmu_t*)((char *)hmu_old + tot_size_old);
        if (hmu_is_in_heap(heap, hmu_next)) {
            ut = hmu_get_ut(hmu_next);
            tot_size_next = hmu_get_size(hmu_next);
            if (ut == HMU_FC
                && tot_size <= tot_size_old + tot_size_next) {
                /* current node and next node meets requirement */
                unlink_hmu(heap, hmu_next);
                hmu_set_size(hmu_old, tot_size);
                memset((char*)hmu_old + tot_size_old, 0, tot_size - tot_size_old);
#if BH_ENABLE_GC_VERIFY != 0
                hmu_init_prefix_and_suffix(hmu_old, tot_size, file, line);
#endif
                if (tot_size < tot_size_old + tot_size_next) {
                    hmu_next = (hmu_t*)((char*)hmu_old + tot_size);
                    tot_size_next = tot_size_old + tot_size_next - tot_size;
                    gci_add_fc(heap, hmu_next, tot_size_next);
                }
                os_mutex_unlock(&heap->lock);
                return obj_old;
            }
        }
    }


    hmu = alloc_hmu_ex(heap, tot_size);
    if (!hmu)
        goto finish;

    g_total_malloc += tot_size;

    hmu_set_ut(hmu, HMU_VO);
    hmu_unfree_vo(hmu);

#if BH_ENABLE_GC_VERIFY != 0
    hmu_init_prefix_and_suffix(hmu, tot_size, file, line);
#endif

    ret = hmu_to_obj(hmu);

finish:
    os_mutex_unlock(&heap->lock);

    if (ret) {
        obj_size = tot_size - HMU_SIZE - OBJ_PREFIX_SIZE - OBJ_SUFFIX_SIZE;
        memset(ret, 0, obj_size);
        if (obj_old) {
            obj_size_old = tot_size_old - HMU_SIZE
                           - OBJ_PREFIX_SIZE - OBJ_SUFFIX_SIZE;
            bh_memcpy_s(ret, obj_size, obj_old, obj_size_old);
            gc_free_vo(vheap, obj_old);
        }
    }

    return ret;
}

/**
 * Do some checking to see if given pointer is a possible valid heap
 * @return GC_TRUE if all checking passed, GC_FALSE otherwise
 */
int
gci_is_heap_valid(gc_heap_t *heap)
{
    if (!heap)
        return GC_FALSE;
    if (heap->heap_id != (gc_handle_t) heap)
        return GC_FALSE;

    return GC_TRUE;
}

#if BH_ENABLE_GC_VERIFY == 0
int
gc_free_vo(void *vheap, gc_object_t obj)
#else
int
gc_free_vo_internal(void *vheap, gc_object_t obj,
                    const char *file, int line)
#endif
{
    gc_heap_t* heap = (gc_heap_t*) vheap;
    hmu_t *hmu = NULL;
    hmu_t *prev = NULL;
    hmu_t *next = NULL;
    gc_size_t size = 0;
    hmu_type_t ut;
    int ret = GC_SUCCESS;

    if (!obj) {
        return GC_SUCCESS;
    }

    hmu = obj_to_hmu(obj);

    os_mutex_lock(&heap->lock);

    if ((gc_uint8 *)hmu >= heap->base_addr
        && (gc_uint8 *)hmu < heap->base_addr + heap->current_size) {
#if BH_ENABLE_GC_VERIFY != 0
        hmu_verify(hmu);
#endif
        ut = hmu_get_ut(hmu);
        if (ut == HMU_VO) {
            if (hmu_is_vo_freed(hmu)) {
                bh_assert(0);
                ret = GC_ERROR;
                goto out;
            }

            size = hmu_get_size(hmu);

            g_total_free += size;

            heap->total_free_size += size;

            if (!hmu_get_pinuse(hmu)) {
                prev = (hmu_t*) ((char*) hmu - *((int*) hmu - 1));

                if (hmu_is_in_heap(heap, prev) && hmu_get_ut(prev) == HMU_FC) {
                    size += hmu_get_size(prev);
                    hmu = prev;
                    unlink_hmu(heap, prev);
                }
            }

            next = (hmu_t*) ((char*) hmu + size);
            if (hmu_is_in_heap(heap, next)) {
                if (hmu_get_ut(next) == HMU_FC) {
                    size += hmu_get_size(next);
                    unlink_hmu(heap, next);
                    next = (hmu_t*) ((char*) hmu + size);
                }
            }

            gci_add_fc(heap, hmu, size);

            if (hmu_is_in_heap(heap, next)) {
                hmu_unmark_pinuse(next);
            }

        } else {
            ret = GC_ERROR;
            goto out;
        }
        ret = GC_SUCCESS;
        goto out;
    }

out:
    os_mutex_unlock(&heap->lock);
    return ret;
}

void
gc_dump_heap_stats(gc_heap_t *heap)
{
    os_printf("heap: %p, heap start: %p\n", heap, heap->base_addr);
    os_printf("total free: %u, current: %u, highmark: %u\n",
              heap->total_free_size, heap->current_size, heap->highmark_size);
    os_printf("g_total_malloc=%lu, g_total_free=%lu, occupied=%lu\n",
              g_total_malloc, g_total_free, g_total_malloc - g_total_free);
}

uint32
gc_get_heap_highmark_size(gc_heap_t *heap)
{
    return heap->highmark_size;
}

void
gci_dump(gc_heap_t *heap)
{
    hmu_t *cur = NULL, *end = NULL;
    hmu_type_t ut;
    gc_size_t size;
    int i = 0, p, mark;
    char inuse = 'U';

    cur = (hmu_t*)heap->base_addr;
    end = (hmu_t*)((char*)heap->base_addr + heap->current_size);

    while(cur < end) {
        ut = hmu_get_ut(cur);
        size = hmu_get_size(cur);
        p = hmu_get_pinuse(cur);
        mark = hmu_is_jo_marked (cur);

        if (ut == HMU_VO)
            inuse = 'V';
        else if (ut == HMU_JO)
            inuse = hmu_is_jo_marked(cur) ? 'J' : 'j';
        else if (ut == HMU_FC)
            inuse = 'F';

        bh_assert(size > 0);

        os_printf("#%d %08x %x %x %d %c %d\n",
                  i, (int32)((char*) cur - (char*) heap->base_addr),
                  ut, p, mark, inuse, (int32)hmu_obj_size(size));
#if BH_ENABLE_GC_VERIFY != 0
        if (inuse == 'V') {
            gc_object_prefix_t *prefix = (gc_object_prefix_t *)(cur + 1);
            os_printf("#%s:%d\n", prefix->file_name, prefix->line_no);
        }
#endif

        cur = (hmu_t*)((char *)cur + size);
        i++;
    }

    bh_assert(cur == end);
}

