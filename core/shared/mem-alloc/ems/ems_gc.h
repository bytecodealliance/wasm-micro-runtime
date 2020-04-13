/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/**
 * @file   ems_gc.h
 * @date   Wed Aug  3 10:46:38 2011
 *
 * @brief  This file defines GC modules types and interfaces.
 *
 *
 */

#ifndef _EMS_GC_H
#define _EMS_GC_H

#include "bh_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GC_HEAD_PADDING 4

#define NULL_REF ((gc_object_t)NULL)

#define GC_SUCCESS (0)
#define GC_ERROR (-1)

#define GC_TRUE (1)
#define GC_FALSE (0)

#define GC_MAX_HEAP_SIZE (256 * BH_KB)

typedef void * gc_handle_t;
typedef void * gc_object_t;
typedef int64  gc_int64;
typedef uint32 gc_uint32;
typedef int32  gc_int32;
typedef uint16 gc_uint16;
typedef int16  gc_int16;
typedef uint8  gc_uint8;
typedef int8   gc_int8;
typedef uint32 gc_size_t;

typedef enum {
    GC_STAT_TOTAL = 0,
    GC_STAT_FREE,
    GC_STAT_HIGHMARK,
} GC_STAT_INDEX;

/**
 * GC initialization from a buffer
 *
 * @param buf the buffer to be initialized to a heap
 * @param buf_size the size of buffer
 *
 * @return gc handle if success, NULL otherwise
 */
gc_handle_t
gc_init_with_pool(char *buf, gc_size_t buf_size);

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

/**
 * Migrate heap from one place to another place
 *
 * @param handle handle of the new heap
 * @param handle_old handle of the old heap
 *
 * @return GC_SUCCESS if success, GC_ERROR otherwise
 */
int
gc_migrate(gc_handle_t handle, gc_handle_t handle_old);

/**
 * Re-initialize lock of heap
 *
 * @param handle the heap handle
 *
 * @return GC_SUCCESS if success, GC_ERROR otherwise
 */
int
gc_reinit_lock(gc_handle_t handle);

/**
 * Destroy lock of heap
 *
 * @param handle the heap handle
 */
void
gc_destroy_lock(gc_handle_t handle);

/**
 * Get Heap Stats
 *
 * @param stats [out] integer array to save heap stats
 * @param size [in] the size of stats
 * @param mmt [in] type of heap, MMT_SHARED or MMT_INSTANCE
 */
void *
gc_heap_stats(void *heap, uint32* stats, int size);

#if BH_ENABLE_GC_VERIFY == 0

gc_object_t
gc_alloc_vo(void *heap, gc_size_t size);

gc_object_t
gc_realloc_vo(void *heap, void *ptr, gc_size_t size);

int
gc_free_vo(void *heap, gc_object_t obj);

#else /* else of BH_ENABLE_GC_VERIFY */

gc_object_t
gc_alloc_vo_internal(void *heap, gc_size_t size,
                     const char *file, int line);

gc_object_t
gc_realloc_vo_internal(void *heap, void *ptr, gc_size_t size,
                       const char *file, int line);

int
gc_free_vo_internal(void *heap, gc_object_t obj,
                    const char *file, int line);

#define gc_alloc_vo(heap, size) \
    gc_alloc_vo_internal(heap, size, __FILE__, __LINE__)

#define gc_realloc_vo(heap, ptr, size) \
    gc_realloc_vo_internal(heap, ptr, size, __FILE__, __LINE__)

#define gc_free_vo(heap, obj) \
    gc_free_vo_internal(heap, obj, __FILE__, __LINE__)

#endif /* end of BH_ENABLE_GC_VERIFY */

#ifdef __cplusplus
}
#endif

#endif

