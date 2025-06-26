/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#if defined(__aarch64__)
#if WASM_ENABLE_SIMD == 0
#include "invokeNative_aarch64.s"
#else
#include "invokeNative_aarch64_simd.s"
#endif
#else
#if WASM_ENABLE_SIMD == 0
#include "invokeNative_em64.s"
#else
#include "invokeNative_em64_simd.s"
#endif
#endif#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
