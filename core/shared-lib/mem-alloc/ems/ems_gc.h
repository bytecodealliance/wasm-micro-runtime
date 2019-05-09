/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

/*Pre-compile configuration can be done here or on Makefiles*/
/*#define GC_EMBEDDED or GC_STANDALONE*/
/*#define GC_DEBUG*/
/*#define GC_TEST // TEST mode is a sub-mode of STANDALONE*/
/* #define GC_ALLOC_TRACE */
/* #define GC_STAT */
#ifndef GC_STAT_DATA
#define GC_STAT_DATA 1
#endif

#define GC_HEAD_PADDING 4

/* Standalone GC is used for testing.*/
#ifndef GC_EMBEDDED
#    ifndef GC_STANDALONE
#        define GC_STANDALONE
#    endif
#endif

#if defined(GC_EMBEDDED) && defined(GC_STANDALONE)
#    error "Can not define GC_EMBEDDED and GC_STANDALONE at the same time"
#endif

#ifdef BH_TEST
#    ifndef GC_TEST
#        define GC_TEST
#    endif
#endif

#ifdef BH_DEBUG
/*instrument mode ignore GC_DEBUG feature, for instrument testing gc_alloc_vo_i_heap only has func_name parameter*/
#if !defined INSTRUMENT_TEST_ENABLED && !defined GC_DEBUG
#        define GC_DEBUG
#endif
#endif

#if defined(GC_EMBEDDED) && defined(GC_TEST)
#    error "Can not defined GC_EMBEDDED and GC_TEST at the same time"
#endif

typedef void *gc_handle_t;
typedef void *gc_object_t;

#define NULL_REF ((gc_object_t)NULL)

#define GC_SUCCESS (0)
#define GC_ERROR (-1)

#define GC_TRUE (1)
#define GC_FALSE (0)

#define GC_MAX_HEAP_SIZE (256 * BH_KB)

typedef int64 gc_int64;

typedef unsigned int gc_uint32;
typedef signed int gc_int32;
typedef unsigned short gc_uint16;
typedef signed short gc_int16;
typedef unsigned char gc_uint8;
typedef signed char gc_int8;
typedef gc_uint32 gc_size_t;

typedef enum {
    MMT_SHARED = 0,
    MMT_INSTANCE = 1,
    MMT_APPMANAGER = MMT_SHARED,
    MMT_VERIFIER = MMT_SHARED,
    MMT_JHI = MMT_SHARED,
    MMT_LOADER = MMT_SHARED,
    MMT_APPLET = MMT_INSTANCE,
    MMT_INTERPRETER = MMT_INSTANCE
} gc_mm_t;

#ifdef GC_STAT
#define GC_HEAP_STAT_SIZE (128 / 4)

typedef struct {
    int usage;
    int usage_block;
    int vo_usage;
    int jo_usage;
    int free;
    int free_block;
    int vo_free;
    int jo_free;
    int usage_sizes[GC_HEAP_STAT_SIZE];
    int free_sizes[GC_HEAP_STAT_SIZE];
}gc_stat_t;

extern void gc_heap_stat(void* heap, gc_stat_t* gc_stat);
extern void __gc_print_stat(void *heap, int verbose);

#define gc_print_stat __gc_print_stat

#else

#define gc_print_stat(heap, verbose)

#endif

#if GC_STAT_DATA != 0

typedef enum {
    GC_STAT_TOTAL = 0,
    GC_STAT_FREE,
    GC_STAT_HIGHMARK,
    GC_STAT_COUNT,
    GC_STAT_TIME,
    GC_STAT_MAX_1,
    GC_STAT_MAX_2,
    GC_STAT_MAX_3,
    GC_STAT_MAX
} GC_STAT_INDEX;

#endif

/*////////////// Exported APIs*/

/**
 * GC initialization from a buffer
 *
 * @param buf the buffer to be initialized to a heap
 * @param buf_size the size of buffer
 *
 * @return gc handle if success, NULL otherwise
 */
extern gc_handle_t gc_init_with_pool(char *buf, gc_size_t buf_size);

/**
 * Destroy heap which is initilized from a buffer
 *
 * @param handle handle to heap needed destory
 *
 * @return GC_SUCCESS if success
 *         GC_ERROR for bad parameters or failed system resource freeing.
 */
extern int gc_destroy_with_pool(gc_handle_t handle);

#if GC_STAT_DATA != 0

/**
 * Get Heap Stats
 *
 * @param stats [out] integer array to save heap stats
 * @param size [in] the size of stats
 * @param mmt [in] type of heap, MMT_SHARED or MMT_INSTANCE
 */
extern void* gc_heap_stats(void *heap, int* stats, int size, gc_mm_t mmt);

/**
 * Set GC threshold factor
 *
 * @param heap [in] the heap to set
 * @param factor [in] the threshold size is free_size * factor / 1000
 *
 * @return GC_SUCCESS if success.
 */
extern int gc_set_threshold_factor(void *heap, unsigned int factor);

#endif

/*////// Allocate heap object*/

/* There are two versions of allocate functions. The functions with _i suffix should be only used*/
/*  internally. Functions without _i suffix are just wrappers with the corresponded functions with*/
/*  _i suffix. Allocation operation code position are record under DEBUG model for debugging.*/
#ifdef GC_DEBUG
#    define ALLOC_EXTRA_PARAMETERS ,const char*file_name,int line_number
#    define ALLOC_EXTRA_ARGUMENTS , __FILE__, __LINE__
#    define ALLOC_PASSDOWN_EXTRA_ARGUMENTS , file_name, line_number
#    define gc_alloc_vo_h(heap, size) gc_alloc_vo_i_heap(heap, size, __FILE__, __LINE__)
#    define gc_free_h(heap, obj) gc_free_i_heap(heap, obj, __FILE__, __LINE__)
#else
#    define ALLOC_EXTRA_PARAMETERS
#    define ALLOC_EXTRA_ARGUMENTS
#    define ALLOC_PASSDOWN_EXTRA_ARGUMENTS
#    define gc_alloc_vo_h                    gc_alloc_vo_i_heap
#    define gc_free_h                        gc_free_i_heap
#endif

/**
 * Invoke a GC
 *
 * @param heap
 *
 * @return GC_SUCCESS if success
 */
extern int gci_gc_heap(void *heap);

/**
 * Allocate VM Object in specific heap.
 *
 * @param heap heap to allocate.
 * @param size bytes to allocate.
 *
 * @return pointer to VM object allocated
 *         NULL if failed.
 */
extern gc_object_t _gc_alloc_vo_i_heap(void *heap,
        gc_size_t size ALLOC_EXTRA_PARAMETERS);
extern gc_object_t _gc_alloc_jo_i_heap(void *heap,
        gc_size_t size ALLOC_EXTRA_PARAMETERS);
#ifdef INSTRUMENT_TEST_ENABLED
extern gc_object_t gc_alloc_vo_i_heap_instr(void *heap, gc_size_t size, const char* func_name );
extern gc_object_t gc_alloc_jo_i_heap_instr(void *heap, gc_size_t size, const char* func_name);
#    define gc_alloc_vo_i_heap(heap, size) gc_alloc_vo_i_heap_instr(heap, size, __FUNCTION__)
#    define gc_alloc_jo_i_heap(heap, size) gc_alloc_jo_i_heap_instr(heap, size, __FUNCTION__)
#else
#    define gc_alloc_vo_i_heap _gc_alloc_vo_i_heap
#    define gc_alloc_jo_i_heap _gc_alloc_jo_i_heap
#endif

/**
 * Allocate Java object in specific heap.
 *
 * @param heap heap to allocate.
 * @param size bytes to allocate.
 *
 * @return pointer to Java object allocated
 *         NULL if failed.
 */
extern gc_object_t _gc_alloc_jo_i_heap(void *heap,
        gc_size_t size ALLOC_EXTRA_PARAMETERS);

/**
 * Free VM object
 *
 * @param heap heap to free.
 * @param obj pointer to object need free.
 *
 * @return GC_SUCCESS if success
 */
extern int gc_free_i_heap(void *heap, gc_object_t obj ALLOC_EXTRA_PARAMETERS);

/**
 * Add ref to rootset of gc for current instance.
 *
 * @param obj pointer to real load of a valid Java object managed by gc for current instance.
 *
 * @return GC_SUCCESS if success.
 *         GC_ERROR for invalid parameters.
 */
extern int gc_add_root(void* heap, gc_object_t obj);

/*////////////// Imported APIs which should be implemented in other components*/

/*////// Java object layout related APIs*/

/**
 * Get Java object size from corresponding VM module
 *
 * @param obj pointer to the real load of a Java object.
 *
 * @return size of java object.
 */
extern gc_size_t vm_get_java_object_size(gc_object_t obj);

/**
 * Get reference list of this object
 *
 * @param obj [in] pointer to java object.
 * @param is_compact_mode [in] indicate the java object mode. GC_TRUE or GC_FALSE.
 * @param ref_num [out] the size of ref_list.
 * @param ref_list [out] if is_compact_mode is GC_FALSE, this parameter will be set to a list of offset.
 * @param ref_start_offset [out] If is_compact_mode is GC_TRUE, this parameter will be set to the start offset of the references in this object.
 *
 * @return GC_SUCCESS if success.
 *         GC_ERROR when error occurs.
 */
extern int vm_get_java_object_ref_list(gc_object_t obj, int *is_compact_mode,
        gc_size_t *ref_num, gc_uint16 **ref_list, gc_uint32 *ref_start_offset);

/**
 * Get gc handle for current instance
 *
 *
 * @return instance heap handle.
 */
extern gc_handle_t app_manager_get_cur_applet_heap(void);

/**
 * Begin current instance heap rootset enumeration
 *
 *
 * @return GC_SUCCESS if success.
 *         GC_ERROR when error occurs.
 */
extern int vm_begin_rootset_enumeration(void *heap);

#ifdef _INSTRUMENT_TEST_ENABLED
extern int vm_begin_rootset_enumeration_instr(void *heap, const char*func_name);
#define vm_begin_rootset_enumeration(heap) vm_begin_rootset_enumeration_instr(heap, __FUNCTION__)
#else
#define vm_begin_rootset_enumeration _vm_begin_rootset_enumeration
#endif    /* INSTUMENT_TEST_ENABLED*/

#ifndef offsetof
#define offsetof(Type, field) ((size_t)(&((Type *)0)->field))
#endif

#ifdef __cplusplus
}
#endif

#endif

