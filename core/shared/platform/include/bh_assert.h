/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _BH_ASSERT_H
#define _BH_ASSERT_H

#include "bh_config.h"
#include "bh_platform.h"

#ifdef BH_TEST
#    ifndef BH_DEBUG
#        error "BH_TEST should be defined under BH_DEBUG"
#    endif
#endif

#ifdef BH_TEST
#    if defined(WIN32) || defined(__linux__)
#    else
#        error "Test case can not run on the current platform"
#    endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BH_DEBUG

void bh_assert_internal(int v, const char *file_name, int line_number,
                        const char *expr_string);
#define bh_assert(expr) bh_assert_internal((int)(expr), __FILE__, __LINE__, #expr)

void bh_debug_internal(const char *file_name, int line_number,
                       const char *fmt, ...);
#if defined(WIN32)
  #define bh_debug(fmt, ...) bh_debug_internal(__FILE__, __LINE__, fmt, __VA_ARGS__)
#elif defined(__linux__)
  #define bh_debug bh_debug_internal(__FILE__, __LINE__, "");printf
#else
  #error "Unsupported platform"
#endif

#else /* else of BH_DEBUG */

#define bh_debug if(0)printf

#endif /* end of BH_DEBUG */

#ifdef BH_TEST
  #define BH_STATIC
#else
  #define BH_STATIC static
#endif /* end of BH_TEST */

#ifdef __cplusplus
}
#endif

#endif

