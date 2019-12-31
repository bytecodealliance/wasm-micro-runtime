/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "wasm_app.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <sys/stat.h>
#include <assert.h>

/* User global variable */
static int num = 0;

int main(int argc, char *argv[])
{
    /* The "_start" function will call the main function
     * and leave it empty is enough. */
    return 0;
}

int wasi_test()
{
    int fd, n, ret;
    char *file_name = "fadvise_test_file.txt";
    struct stat stat_buf;

    fd = open(file_name, O_RDWR|O_CREAT|O_TRUNC);
    assert(fd != 0);
    assert(errno == 0);

    char *test_string = "wasi fadvise test";
    n = write(fd, test_string, strlen(test_string));
    assert(n == strlen(test_string));
    assert(errno == 0);

    ret = fstat(fd, &stat_buf);
    assert(ret == 0);

    ret = posix_fallocate(fd, n, 1024);
    assert(ret == 0);

    ret = fstat(fd, &stat_buf);
    assert(ret == 0);
    assert(stat_buf.st_size == strlen(test_string) + 1024);

    ret = remove(file_name);
    assert(ret == 0);

    return 0;
}

/* Timer callback */
void timer1_update(user_timer_t timer)
{
    printf("Timer update %d\n", num++);
    if (wasi_test() != 0)
        printf("wasi test fail.\n");
    else
        printf("wasi test ok.\n");
}

void on_init()
{
    user_timer_t timer;

    /* set up a timer */
    timer = api_timer_create(1000, true, false, timer1_update);
    api_timer_restart(timer, 1000);
}

void on_destroy()
{
    /* real destroy work including killing timer and closing sensor is
       accomplished in wasm app library version of on_destroy() */
}
