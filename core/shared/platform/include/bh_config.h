/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/**
 * @file   bh_config.h
 * @date   Tue Sep 13 14:53:17 2011
 *
 * @brief Configurations for different platforms and targets.  Make
 * sure all source files in Beihai project include this header file
 * directly or indirectly.
 */

#ifndef BH_CONFIG

#include "config.h"

#define BH_KB (1024)
#define BH_MB ((BH_KB)*1024)
#define BH_GB ((BH_MB)*1024)

#ifndef BH_MALLOC
#define BH_MALLOC os_malloc
#endif

#ifndef BH_FREE
#define BH_FREE os_free
#endif

#ifndef WA_MALLOC
#include <stdlib.h>
#define WA_MALLOC malloc
#endif

#ifndef WA_FREE
#include <stdlib.h>
#define WA_FREE free
#endif

#ifdef __cplusplus
extern "C" {
#endif

void *wasm_runtime_malloc(unsigned int size);
void wasm_runtime_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* end of BH_CONFIG */

