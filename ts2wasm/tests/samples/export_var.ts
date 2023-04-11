/*
 * Copyright (C) 2023 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

export const a = 1;

const b = 2;

export { b };

const c = 3; // not exported as "c"

export { c as renamed_c };
