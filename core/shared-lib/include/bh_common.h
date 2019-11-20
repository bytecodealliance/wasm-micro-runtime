/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _BH_COMMON_H
#define _BH_COMMON_H

#include "bh_assert.h"
#include "bh_platform.h"
#include "bh_log.h"
#include "bh_list.h"

typedef void (*bh_print_function_t)(const char* message);
void bh_set_print_function(bh_print_function_t pf);

#define bh_memcpy_s(dest, dlen, src, slen) do {                         \
    int _ret = slen == 0 ? 0 : b_memcpy_s (dest, dlen, src, slen);      \
    (void)_ret;                                                         \
    bh_assert (_ret == 0);                                              \
  } while (0)

#define bh_strcat_s(dest, dlen, src) do {                               \
    int _ret = b_strcat_s (dest, dlen, src);                            \
    (void)_ret;                                                         \
    bh_assert (_ret == 0);                                              \
  } while (0)

#define bh_strcpy_s(dest, dlen, src) do {                               \
    int _ret = b_strcpy_s (dest, dlen, src);                            \
    (void)_ret;                                                         \
    bh_assert (_ret == 0);                                              \
  } while (0)

#endif
