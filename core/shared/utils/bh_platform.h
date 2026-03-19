/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _BH_PLATFORM_H
#define _BH_PLATFORM_H

#include "../platform/include/platform_common.h"
#include "../platform/include/platform_api_vmcore.h"
#include "../platform/include/platform_api_extension.h"
#include "bh_assert.h"
#include "bh_common.h"
#include "bh_hashmap.h"
#include "bh_list.h"
#include "bh_log.h"
#include "bh_queue.h"
#include "bh_vector.h"
#include "runtime_timer.h"

/**
 * API visibility macros for WAMR internal functions
 *
 * WASM_RUNTIME_API_EXTERN - Public exported APIs (defined in wasm_export.h)
 * WASM_RUNTIME_API_INTERN - Internal APIs visible across WAMR components
 *
 * In test builds (WAMR_BUILD_TEST=1), internal APIs are exposed for unit
 * testing. In production builds, internal APIs are static (file-scoped) for
 * encapsulation.
 */
#ifndef WASM_RUNTIME_API_INTERN
#ifdef WAMR_BUILD_TEST
#define WASM_RUNTIME_API_INTERN
#else
#define WASM_RUNTIME_API_INTERN static
#endif
#endif

/**
 * WA_MALLOC/WA_FREE need to be redefined for both
 * runtime native and WASM app respectively.
 *
 * Some source files are shared for building native and WASM,
 * and this the mem allocator API for these files.
 *
 * Here we define it for the native world
 */
#ifndef WA_MALLOC
#define WA_MALLOC wasm_runtime_malloc
#endif

#ifndef WA_FREE
#define WA_FREE wasm_runtime_free
#endif

#endif /* #ifndef _BH_PLATFORM_H */
