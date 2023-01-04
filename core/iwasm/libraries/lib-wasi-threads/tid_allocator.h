/*
 * Copyright (C) 2023 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _TID_ALLOCATOR_H
#define _TID_ALLOCATOR_H

#include "platform_common.h"

#define TID_ALLOCATOR_INIT_SIZE CLUSTER_MAX_THREAD_NUM
enum {
    TID_MIN = 1,
    TID_MAX = 0x1FFFFFFF
}; // Reserved TIDs (WASI specification)

/* Stack data structure to track available thread identifiers */
typedef struct tidAllocator TidAllocator;

TidAllocator *
tid_allocator_init();

void
tid_allocator_destroy(TidAllocator *tid_allocator);

int32
tid_allocator_get(TidAllocator *tid_allocator);

void
tid_allocator_put(TidAllocator *tid_allocator, int32 thread_id);

#endif /* _TID_ALLOCATOR_H */