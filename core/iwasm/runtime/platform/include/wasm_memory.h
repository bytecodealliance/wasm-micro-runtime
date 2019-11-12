/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WASM_MEMORY_H
#define _WASM_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bh_memory.h"

#define wasm_malloc bh_malloc
#define wasm_free bh_free

#ifdef __cplusplus
}
#endif

#endif /* end of _WASM_MEMORY_H */

