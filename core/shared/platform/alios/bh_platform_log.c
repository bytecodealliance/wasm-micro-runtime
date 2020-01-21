/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"
#include <stdio.h>


void bh_log_emit(const char *fmt, va_list ap)
{
  vprintf(fmt, ap);
}

int bh_fprintf(FILE *stream, const char *fmt, ...)
{
  (void)stream;
  va_list ap;
  int ret;

  va_start(ap, fmt);
  ret = vprintf(fmt, ap);
  va_end(ap);

  return ret;
}

int bh_fflush(void *stream)
{
  (void)stream;
  return 0;
}

