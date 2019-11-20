/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _EMS_GC_INTERNAL_H
#define _EMS_GC_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bh_platform.h"
#include "bh_thread.h"
#include "bh_memory.h"
#include "bh_assert.h"
#include "ems_gc.h"

/* basic block managed by EMS gc is the so-called HMU (heap memory unit)*/
typedef enum _hmu_type_enum
{
    HMU_TYPE_MIN = 0,
    HMU_TYPE_MAX = 3,
    HMU_JO = 3,
    HMU_VO = 2,
    HMU_FC = 1,
    HMU_FM = 0
}hmu_type_t;

typedef struct _hmu_struct
{
    gc_uint32 header;
}hmu_t;

#if defined(GC_VERIFY)

#define GC_OBJECT_PREFIX_PADDING_CNT 3
#define GC_OBJECT_SUFFIX_PADDING_CNT 4
#define GC_OBJECT_PADDING_VALUE (0x12345678)

typedef struct _gc_object_prefix
{
    const char *file_name;
    gc_int32 line_no;
    gc_int32 size;
    gc_uint32 padding[GC_OBJECT_PREFIX_PADDING_CNT];
}gc_object_prefix_t;

#define OBJ_PREFIX_SIZE (sizeof(gc_object_prefix_t))

typedef struct _gc_object_suffix
{
    gc_uint32 padding[GC_OBJECT_SUFFIX_PADDING_CNT];
}gc_object_suffix_t;

#define OBJ_SUFFIX_SIZE (sizeof(gc_object_suffix_t))

extern void hmu_init_prefix_and_suffix(hmu_t *hmu, gc_size_t tot_size, const char *file_name, int line_no);
extern void hmu_verify(hmu_t *hmu);

#define SKIP_OBJ_PREFIX(p) ((void*)((gc_uint8*)(p) + OBJ_PREFIX_SIZE))
#define SKIP_OBJ_SUFFIX(p) ((void*)((gc_uint8*)(p) + OBJ_SUFFIX_SIZE))

#define OBJ_EXTRA_SIZE (HMU_SIZE + OBJ_PREFIX_SIZE + OBJ_SUFFIX_SIZE)

#else

#define OBJ_PREFIX_SIZE 0
#define OBJ_SUFFIX_SIZE 0

#define SKIP_OBJ_PREFIX(p) ((void*)((gc_uint8*)(p) + OBJ_PREFIX_SIZE))
#define SKIP_OBJ_SUFFIX(p) ((void*)((gc_uint8*)(p) + OBJ_SUFFIX_SIZE))

#define OBJ_EXTRA_SIZE (HMU_SIZE + OBJ_PREFIX_SIZE + OBJ_SUFFIX_SIZE)

#endif /* GC_DEBUG*/

#define hmu_obj_size(s) ((s)-OBJ_EXTRA_SIZE)

#define GC_ALIGN_8(s) (((uint32)(s) + 7) & (uint32)~7)

#define GC_SMALLEST_SIZE GC_ALIGN_8(HMU_SIZE + OBJ_PREFIX_SIZE + OBJ_SUFFIX_SIZE + 8)
#define GC_GET_REAL_SIZE(x) GC_ALIGN_8(HMU_SIZE + OBJ_PREFIX_SIZE + OBJ_SUFFIX_SIZE + (((x) > 8) ? (x): 8))

/*////// functions for bit operation*/

#define SETBIT(v, offset) (v) |= (1 << (offset))
#define GETBIT(v, offset) ((v) & (1 << (offset)) ? 1 : 0)
#define CLRBIT(v, offset) (v) &= (uint32)(~(1 << (offset)))

#define SETBITS(v, offset, size, value) do {        \
    (v) &= (uint32)(~(((1 << size) - 1) << offset));\
    (v) |= (uint32)(value << offset);               \
  } while(0)
#define CLRBITS(v, offset, size) (v) &= ~(((1 << size) - 1) << offset)
#define GETBITS(v, offset, size) (((v) & ((uint32)(((1 << size) - 1) << offset))) >> offset)

/*////// gc object layout definition*/

#define HMU_SIZE (sizeof(hmu_t))

#define hmu_to_obj(hmu) (gc_object_t)(SKIP_OBJ_PREFIX((hmu_t*) (hmu) + 1))
#define obj_to_hmu(obj) ((hmu_t *)((gc_uint8*)(obj) - OBJ_PREFIX_SIZE) - 1)

#define HMU_UT_SIZE      2
#define HMU_UT_OFFSET    30

#define hmu_get_ut(hmu) GETBITS ((hmu)->header, HMU_UT_OFFSET, HMU_UT_SIZE)
#define hmu_set_ut(hmu, type) SETBITS ((hmu)->header, HMU_UT_OFFSET, HMU_UT_SIZE, type)
#define hmu_is_ut_valid(tp) (tp >= HMU_TYPE_MIN && tp <= HMU_TYPE_MAX)

/* P in use bit means the previous chunk is in use */
#define HMU_P_OFFSET 29

#define hmu_mark_pinuse(hmu) SETBIT ((hmu)->header, HMU_P_OFFSET)
#define hmu_unmark_pinuse(hmu) CLRBIT ((hmu)->header, HMU_P_OFFSET)
#define hmu_get_pinuse(hmu) GETBIT ((hmu)->header, HMU_P_OFFSET)

#define HMU_JO_VT_SIZE   27
#define HMU_JO_VT_OFFSET 0
#define HMU_JO_MB_OFFSET 28

#define hmu_mark_jo(hmu) SETBIT ((hmu)->header, HMU_JO_MB_OFFSET)
#define hmu_unmark_jo(hmu) CLRBIT ((hmu)->header, HMU_JO_MB_OFFSET)
#define hmu_is_jo_marked(hmu) GETBIT ((hmu)->header, HMU_JO_MB_OFFSET)

#define HMU_SIZE_SIZE 27
#define HMU_SIZE_OFFSET 0

#define HMU_VO_FB_OFFSET 28

#define hmu_is_vo_freed(hmu) GETBIT ((hmu)->header, HMU_VO_FB_OFFSET)
#define hmu_unfree_vo(hmu) CLRBIT ((hmu)->header, HMU_VO_FB_OFFSET)

#define hmu_get_size(hmu) GETBITS ((hmu)->header, HMU_SIZE_OFFSET, HMU_SIZE_SIZE)
#define hmu_set_size(hmu, size) SETBITS ((hmu)->header, HMU_SIZE_OFFSET, HMU_SIZE_SIZE, size)

/*////// HMU free chunk management*/

#define HMU_NORMAL_NODE_CNT 32
#define HMU_FC_NORMAL_MAX_SIZE ((HMU_NORMAL_NODE_CNT - 1) << 3)
#define HMU_IS_FC_NORMAL(size) ((size) < HMU_FC_NORMAL_MAX_SIZE)
#if HMU_FC_NORMAL_MAX_SIZE >= GC_MAX_HEAP_SIZE
#error "Too small GC_MAX_HEAP_SIZE"
#endif

typedef struct _hmu_normal_node
{
    hmu_t hmu_header;
    struct _hmu_normal_node *next;
}hmu_normal_node_t;

typedef struct _hmu_tree_node
{
    hmu_t hmu_header;
    gc_size_t size;
    struct _hmu_tree_node *left;
    struct _hmu_tree_node *right;
    struct _hmu_tree_node *parent;
}hmu_tree_node_t;

typedef struct _gc_heap_struct
{
    gc_handle_t heap_id; /* for double checking*/

    gc_uint8 *base_addr;
    gc_size_t current_size;
    gc_size_t max_size;

    korp_mutex lock;

    hmu_normal_node_t kfc_normal_list[HMU_NORMAL_NODE_CNT];

    /* order in kfc_tree is: size[left] <= size[cur] < size[right]*/
    hmu_tree_node_t kfc_tree_root;

    /* for rootset enumeration of private heap*/
    void *root_set;

    /* whether the fast mode of marking process that requires
     additional memory fails.  When the fast mode fails, the
     marking process can still be done in the slow mode, which
     doesn't need additional memory (by walking through all
     blocks and marking sucessors of marked nodes until no new
     node is marked).  TODO: slow mode is not implemented.  */
    unsigned is_fast_marking_failed : 1;

#if GC_STAT_DATA != 0
    gc_size_t highmark_size;
    gc_size_t init_size;
    gc_size_t total_gc_count;
    gc_size_t total_free_size;
    gc_size_t gc_threshold;
    gc_size_t gc_threshold_factor;
    gc_int64 total_gc_time;
#endif
}gc_heap_t;

/*////// MISC internal used APIs*/

extern void gci_add_fc(gc_heap_t *heap, hmu_t *hmu, gc_size_t size);
extern int gci_is_heap_valid(gc_heap_t *heap);

#ifdef GC_DEBUG
extern void gci_verify_heap(gc_heap_t *heap);
extern void gci_dump(char* buf, gc_heap_t *heap);
#endif

#if GC_STAT_DATA != 0

/* the default GC threshold size is free_size * GC_DEFAULT_THRESHOLD_FACTOR / 1000 */
#define GC_DEFAULT_THRESHOLD_FACTOR 400

static inline void gc_update_threshold(gc_heap_t *heap)
{
    heap->gc_threshold = heap->total_free_size * heap->gc_threshold_factor / 1000;
}
#endif

/*////// MISC data structures*/

#define MARK_NODE_OBJ_CNT 256

/* mark node is used for gc marker*/
typedef struct _mark_node_struct
{
    /* number of to-expand objects can be saved in this node*/
    gc_size_t cnt;

    /* the first unused index*/
    int idx;

    /* next node on the node list*/
    struct _mark_node_struct *next;

    /* the actual to-expand objects list*/
    gc_object_t set[MARK_NODE_OBJ_CNT];
}mark_node_t;

/*////// Imported APIs wrappers under TEST mode*/

#ifdef GC_TEST
extern int (*gct_vm_get_java_object_ref_list)(
        gc_object_t obj,
        int *is_compact_mode, /* can be set to GC_TRUE, or GC_FALSE */
        gc_size_t *ref_num,
        gc_uint16 **ref_list,
        gc_uint32 *ref_start_offset);
extern int (*gct_vm_mutex_init)(korp_mutex *mutex);
extern int (*gct_vm_mutex_destroy)(korp_mutex *mutex);
extern int (*gct_vm_mutex_lock)(korp_mutex *mutex);
extern int (*gct_vm_mutex_unlock)(korp_mutex *mutex);
extern gc_handle_t (*gct_vm_get_gc_handle_for_current_instance)(void);
extern int (*gct_vm_begin_rootset_enumeration)(void* heap);
extern int (*gct_vm_gc_finished)(void);
#else
#define gct_vm_get_java_object_ref_list             bh_get_java_object_ref_list
#define gct_vm_mutex_init                           vm_mutex_init
#define gct_vm_mutex_destroy                        vm_mutex_destroy
#define gct_vm_mutex_lock                           vm_mutex_lock
#define gct_vm_mutex_unlock                         vm_mutex_unlock
#define gct_vm_get_gc_handle_for_current_instance   app_manager_get_cur_applet_heap
#define gct_vm_begin_rootset_enumeration            vm_begin_rootset_enumeration
#define gct_vm_gc_finished                          jeff_runtime_gc_finished
#endif

#ifdef __cplusplus
}
#endif

#endif
