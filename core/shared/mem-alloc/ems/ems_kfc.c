/*
 * Copyright (C) 2022 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#include "ems_gc_internal.h"

#define HEAP_INC_FACTOR 1

/* Check if current platform is compatible with current GC design*/

/* Return GC_ERROR if not;*/
/* Return GC_SUCCESS otherwise.*/
int
gci_check_platform()
{
#define CHECK(x, y)                                                      \
    do {                                                                 \
        if ((x) != (y)) {                                                \
            LOG_ERROR("Platform checking failed on LINE %d at FILE %s.", \
                      __LINE__, __FILE__);                               \
            return GC_ERROR;                                             \
        }                                                                \
    } while (0)

    CHECK(8, sizeof(gc_int64));
    CHECK(4, sizeof(gc_uint32));
    CHECK(4, sizeof(gc_int32));
    CHECK(2, sizeof(gc_uint16));
    CHECK(2, sizeof(gc_int16));
    CHECK(1, sizeof(gc_int8));
    CHECK(1, sizeof(gc_uint8));
    CHECK(4, sizeof(gc_size_t));
    CHECK(4, sizeof(void *));

    return GC_SUCCESS;
}

static void
adjust_ptr(uint8 **p_ptr, intptr_t offset)
{
    if (*p_ptr)
        *p_ptr += offset;
}

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

#if 0

/* Initialize a heap*/

/* @heap can not be NULL*/
/* @heap_max_size can not exceed GC_MAX_HEAP_SIZE and it should not euqal to or smaller than HMU_FC_NORMAL_MAX_SIZE.*/

/* @heap_max_size will be rounded down to page size at first.*/

/* This function will alloc resource for given heap and initalize all data structures.*/

/* Return GC_ERROR if any errors occur.*/
/* Return GC_SUCCESS otherwise.*/
static int init_heap(gc_heap_t *heap, gc_size_t heap_max_size)
{
	void *base_addr = NULL;
	hmu_normal_node_t *p = NULL;
	hmu_tree_node_t *root = NULL, *q = NULL;
	int i = 0;
	int ret = 0;

	bh_assert(heap);

	if(heap_max_size < 1024) {
		LOG_ERROR("[GC_ERROR]heap_init_size(%d) < 1024 ", heap_max_size);
		return GC_ERROR;
	}

	memset(heap, 0, sizeof *heap);

	ret = gct_vm_mutex_init(&heap->lock);
	if (ret != BHT_OK) {
		LOG_ERROR("[GC_ERROR]failed to init lock ");
		return GC_ERROR;
	}

	heap_max_size = (heap_max_size + 7) & ~(unsigned int)7;

	/* alloc memory for this heap*/
	base_addr = os_malloc(heap_max_size + GC_HEAD_PADDING);
	if(!base_addr)
	{
		LOG_ERROR("[GC_ERROR]reserve heap with size(%u) failed", heap_max_size);
		(void) gct_vm_mutex_destroy(&heap->lock);
		return GC_ERROR;
	}

	base_addr = (char*) base_addr + GC_HEAD_PADDING;

#ifdef BH_FOOTPRINT
	printf("\nINIT HEAP 0x%08x %d\n", base_addr, heap_max_size);
#endif

        bh_assert(((int) base_addr & 7) == 4);

	/* init all data structures*/
	heap->max_size = heap_max_size;
	heap->current_size = heap_max_size;
	heap->base_addr = (gc_uint8*)base_addr;
	heap->heap_id = (gc_handle_t)heap;

#if GC_STAT_DATA != 0
	heap->total_free_size = heap->current_size;
	heap->highmark_size = 0;
	heap->total_gc_count = 0;
	heap->total_gc_time = 0;
	heap->gc_threshold_factor = GC_DEFAULT_THRESHOLD_FACTOR;
	gc_update_threshold(heap);
#endif

	for(i = 0; i < HMU_NORMAL_NODE_CNT;i++)
	{
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

	q = (hmu_tree_node_t *)heap->base_addr;
	memset(q, 0, sizeof *q);
	hmu_set_ut(&q->hmu_header, HMU_FC);
	hmu_set_size(&q->hmu_header, heap->current_size);

	hmu_mark_pinuse(&q->hmu_header);
	root->right = q;
	q->parent = root;
	q->size = heap->current_size;

/* #if !defined(NVALGRIND) */
/* 	VALGRIND_MAKE_MEM_NOACCESS (base_addr, heap_max_size); */
/* #endif */

	bh_assert(root->size <= HMU_FC_NORMAL_MAX_SIZE && HMU_FC_NORMAL_MAX_SIZE < q->size); /*@NOTIFY*/

	return GC_SUCCESS;
}
#endif

#if GC_STAT_DATA != 0
/**
 * Set GC threshold factor
 *
 * @param heap [in] the heap to set
 * @param factor [in] the threshold size is free_size * factor / 1000
 *
 * @return GC_SUCCESS if success.
 */
int
gc_set_threshold_factor(void *instance_heap, unsigned int factor)
{
    gc_heap_t *heap = (gc_heap_t *)instance_heap;

    if (!gci_is_heap_valid(heap)) {
        LOG_ERROR("gc_set_threshold_factor with incorrect private heap");
        return GC_ERROR;
    }

    heap->gc_threshold_factor = factor;
    gc_update_threshold(heap);
    return GC_SUCCESS;
}

#endif

#if BH_ENABLE_GC_VERIFY != 0
/* Verify heap integrity*/
/* @heap should not be NULL and it should be a valid heap*/
void
gci_verify_heap(gc_heap_t *heap)
{
    hmu_t *cur = NULL, *end = NULL;

    bh_assert(heap && gci_is_heap_valid(heap));
    cur = (hmu_t *)heap->base_addr;
    end = (hmu_t *)(heap->base_addr + heap->current_size);
    while (cur < end) {
        hmu_verify(cur);
        cur = (hmu_t *)((gc_uint8 *)cur + hmu_get_size(cur));
    }
    bh_assert(cur == end);
}
#endif
