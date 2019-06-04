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

#include "bh_platform.h"
#include "bh_assert.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef BH_TEST
#include <setjmp.h>
#endif

#ifdef BH_TEST
/* for exception throwing */
jmp_buf bh_test_jb;
#endif

void bh_assert_internal(int v, const char *file_name, int line_number, const char *expr_string)
{
  if(v) return;

  if(!file_name) file_name = "NULL FILENAME";
  if(!expr_string) expr_string = "NULL EXPR_STRING";

  printf("\nASSERTION FAILED: %s, at FILE=%s, LINE=%d\n", expr_string, file_name, line_number);

#ifdef BH_TEST
  longjmp(bh_test_jb, 1);
#endif

  aos_reboot();
}

void bh_debug_internal(const char *file_name, int line_number, const char *fmt, ...)
{
#ifndef JEFF_TEST_VERIFIER
  va_list args;

  va_start(args, fmt);
  bh_assert(file_name);

  printf("\nDebug info FILE=%s, LINE=%d: ", file_name, line_number);
  vprintf(fmt, args);

  va_end(args);
  printf("\n");
#endif
}

