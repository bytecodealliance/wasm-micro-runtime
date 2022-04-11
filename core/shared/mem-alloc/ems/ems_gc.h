/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/**
 * @file   ems_gc.h
 * @date   Wed Aug  3 10:46:38 2011
 *
 * @brief  This file defines GC modules types and interfaces.
 */

#ifndef _EMS_GC_H
#define _EMS_GC_H

#include "bh_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GC_STAT_DATA
#define GC_STAT_DATA 0
#endif

#ifndef GC_STAT_SHOW
#define GC_STAT_SHOW 0
#endif

#ifndef GC_IN_EVERY_ALLOCATION
#define GC_IN_EVERY_ALLOCATION 0
#endif

#ifndef GC_MANUALLY
#define GC_MANUALLY 0
#endif

#define GC_HEAD_PADDING 4

#define NULL_REF ((gc_object_t)NULL)

#define GC_SUCCESS (0)
#define GC_ERROR (-1)

#define GC_TRUE (1)
#define GC_FALSE (0)

#define GC_MAX_HEAP_SIZE (256 * BH_KB)

typedef void *gc_handle_t;
typedef void *gc_object_t;
typedef uint64 gc_uint64;
typedef int64 gc_int64;
typedef uint32 gc_uint32;
typedef int32 gc_int32;
typedef uint16 gc_uint16;
typedef int16 gc_int16;
typedef uint8 gc_uint8;
typedef int8 gc_int8;
typedef uint32 gc_size_t;

typedef enum {
    GC_STAT_TOTAL = 0,
    GC_STAT_FREE,
    GC_STAT_HIGHMARK,
    GC_STAT_COUNT,
    GC_STAT_TIME,
    GC_STAT_MAX
} GC_STAT_INDEX;

/**
 * GC initialization from a buffer, which is separated into
 * two parts: the beginning of the buffer is used to create
 * the heap structure, and the left is used to create the
 * actual pool data
 *
 * @param buf the buffer to be initialized to a heap
 * @param buf_size the size of buffer
 *
 * @return gc handle if success, NULL otherwise
 */
gc_handle_t
gc_init_with_pool(char *buf, gc_size_t buf_size);

/**
 * GC initialization from heap struct buffer and pool buffer
 *
 * @param struct_buf the struct buffer to create the heap structure
 * @param struct_buf_size the size of struct buffer
 * @param pool_buf the pool buffer to create pool data
 * @param pool_buf_size the size of poll buffer
 *
 * @return gc handle if success, NULL otherwise
 */
gc_handle_t
gc_init_with_struct_and_pool(char *struct_buf, gc_size_t struct_buf_size,
                             char *pool_buf, gc_size_t pool_buf_size);

/**
 * Destroy heap which is initilized from a buffer
 *
 * @param handle handle to heap needed destroy
 *
 * @return GC_SUCCESS if success
 *         GC_ERROR for bad parameters or failed system resource freeing.
 */
int
gc_destroy_with_pool(gc_handle_t handle);

#if WASM_ENABLE_GC != 0
/**
 * Enable or disable GC reclaim for a heap
 *
 * @param handle handle of the heap
 * @param enabled enable the GC reclaim or not, true/false to enable/disable
 */
void
gc_enable_heap_reclaim(gc_handle_t handle, bool enabled);
#endif

/**
 * Return heap struct size
 */
uint32
gc_get_heap_struct_size(void);

/**
 * Migrate heap from one pool buf to another pool buf
 *
 * @param handle handle of the new heap
 * @param pool_buf_new the new pool buffer
 * @param pool_buf_size the size of new pool buffer
 *
 * @return GC_SUCCESS if success, GC_ERROR otherwise
 */
int
gc_migrate(gc_handle_t handle, char *pool_buf_new, gc_size_t pool_buf_size);

/**
 * Check whether the heap is corrupted
 *
 * @param handle handle of the heap
 *
 * @return true if success, false otherwise
 */
bool
gc_is_heap_corrupted(gc_handle_t handle);

/**
 * Get Heap Stats
 *
 * @param stats [out] integer array to save heap stats
 * @param size [in] the size of stats
 * @param mmt [in] type of heap, MMT_SHARED or MMT_INSTANCE
 */
void *
gc_heap_stats(void *heap, uint32 *stats, int size);

#if BH_ENABLE_GC_VERIFY == 0

gc_object_t
gc_alloc_vo(void *heap, gc_size_t size);

gc_object_t
gc_realloc_vo(void *heap, void *ptr, gc_size_t size);

int
gc_free_vo(void *heap, gc_object_t obj);

#if WASM_ENABLE_GC != 0
gc_object_t
gc_alloc_wo(void *heap, gc_size_t size);

void
gc_free_wo(void *vheap, void *ptr);
#endif

#else /* else of BH_ENABLE_GC_VERIFY */

gc_object_t
gc_alloc_vo_internal(void *heap, gc_size_t size, const char *file, int line);

gc_object_t
gc_realloc_vo_internal(void *heap, void *ptr, gc_size_t size, const char *file,
                       int line);

int
gc_free_vo_internal(void *heap, gc_object_t obj, const char *file, int line);

#if WASM_ENABLE_GC != 0
gc_object_t
gc_alloc_wo(void *heap, gc_size_t size);

void
gc_free_wo(void *vheap, void *ptr);
#endif

/* clang-format off */
#define gc_alloc_vo(heap, size) \
    gc_alloc_vo_internal(heap, size, __FILE__, __LINE__)

#define gc_realloc_vo(heap, ptr, size) \
    gc_realloc_vo_internal(heap, ptr, size, __FILE__, __LINE__)

#define gc_free_vo(heap, obj) \
    gc_free_vo_internal(heap, obj, __FILE__, __LINE__)

#if WASM_ENABLE_GC != 0
#define gc_alloc_wo(heap, size) \
    gc_alloc_wo_internal(heap, size, __FILE__, __LINE__)

#define gc_free_wo(heap, obj) \
    gc_free_wo_internal(heap, obj, __FILE__, __LINE__)
#endif
/* clang-format on */

#endif /* end of BH_ENABLE_GC_VERIFY */

#if WASM_ENABLE_GC != 0
int
gci_gc_heap(void *heap);

/**
 * Root set enumeration.
 * TODO: This need to be implemented in the ems_gc.c when the heap layout and
 * wasm reference is determined.
 */
int
vm_begin_rootset_enumeration(void *heap);

/**
 * Reference iteration
 * TODO: This function need to be implemented in the ems_gc.c when wasm object
 * layout is determined.
 */
int
vm_get_wasm_object_ref_list(gc_object_t obj, bool *p_is_compact_mode,
                            gc_size_t *ref_num, gc_uint16 **ref_list,
                            gc_uint32 *ref_start_offset);

void
wasm_runtime_gc_prepare();

void
wasm_runtime_gc_finalize();
#endif /* end of WASM_ENABLE_GC != 0 */

#define GC_HEAP_STAT_SIZE (128 / 4)

typedef struct {
    int usage;
    int usage_block;
    int vo_usage;
    int wo_usage;
    int free;
    int free_block;
    int vo_free;
    int wo_free;
    int usage_sizes[GC_HEAP_STAT_SIZE];
    int free_sizes[GC_HEAP_STAT_SIZE];
} gc_stat_t;

void
gc_show_stat(gc_handle_t handle);

#if WASM_ENABLE_GC != 0
void
gc_show_fragment(gc_handle_t handle);
#endif

#ifdef __cplusplus
}
#endif

#endif
