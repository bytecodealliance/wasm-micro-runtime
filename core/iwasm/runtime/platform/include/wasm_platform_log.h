/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _WASM_PLATFORM_LOG
#define _WASM_PLATFORM_LOG

#include "bh_platform.h"

#define wasm_printf bh_printf

#define wasm_vprintf vprintf

#endif /* _WASM_PLATFORM_LOG */
