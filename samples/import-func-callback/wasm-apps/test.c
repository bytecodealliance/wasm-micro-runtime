/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

extern int
import_func1(int a, int b);
extern int
import_func2(int a);

int
test()
{
    int a = import_func1(1, 2);
    int b = import_func2(3);
    return a + b;
}
