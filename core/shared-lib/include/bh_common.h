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

#ifndef _BH_COMMON_H
#define _BH_COMMON_H

#include "bh_assert.h"
#include "bh_definition.h"
#include "bh_platform.h"
#include "bh_log.h"
#include "bh_list.h"

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
