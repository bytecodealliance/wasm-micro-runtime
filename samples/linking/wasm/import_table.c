/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
add(int a, int b)
{
    return a + b;
}

int
subtract(int a, int b)
{
    return a - b;
}

int
main()
{
    int (*operation)(int, int);

    operation = add;
    int result = operation(5, 3);
    printf("Result of addition: %d\n", operation(5, 3));
    assert(result == 8 && "Addition failed");

    operation = subtract;
    result = operation(5, 3);
    printf("Result of subtraction: %d\n", operation(5, 3));
    assert(result == 2 && "Subtraction failed");

    return EXIT_SUCCESS;
}