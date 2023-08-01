/*
 * Copyright (C) 2023 Amazon Inc.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"

#ifndef _BH_TIME_H
#define _BH_TIME_H

#ifdef __cplusplus
extern "C" {
#endif

uint64
convert_timespec(const struct timespec *ts);

#ifdef __cplusplus
}
#endif
#endif /* end of _BH_TIME_H */