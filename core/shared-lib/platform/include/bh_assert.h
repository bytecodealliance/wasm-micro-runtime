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

extern void bh_assert_internal(int v, const char *file_name, int line_number, const char *expr_string);
#define bh_assert(expr) bh_assert_internal((int)(expr), __FILE__, __LINE__, # expr)
extern void bh_debug_internal(const char *file_name, int line_number, const char *fmt, ...);

#if defined(WIN32) || defined(EMU)
#    define bh_debug(fmt, ...) bh_debug_internal(__FILE__, __LINE__, fmt, __VA_ARGS__)
#elif defined(__linux__)
/*#    define bh_debug(...) bh_debug_internal(__FILE__, __LINE__, ## __VA_ARGS__)*/
# define bh_debug bh_debug_internal(__FILE__, __LINE__, "");printf
#elif defined(PLATFORM_SEC)
#    define bh_debug(fmt, ...) bh_debug_internal(__FILE__, __LINE__, fmt, __VA_ARGS__)
#else
#    error "Unsupported platform"
#endif

#else

#define bh_debug if(0)printf

#endif

#define bh_assert_abort(x) do {                 \
        if (!x)                                 \
            abort();                            \
    } while (0)

#ifdef BH_TEST
#    define BH_STATIC
#else
#    define BH_STATIC static
#endif

#ifdef __cplusplus
}
#endif

#endif

/* Local Variables: */
/* mode:c           */
/* c-basic-offset: 4 */
/* indent-tabs-mode: nil */
/* End:             */
