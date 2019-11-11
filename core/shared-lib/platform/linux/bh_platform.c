/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

char *bh_strdup(const char *s)
{
    char *s1 = NULL;
    if (s && (s1 = bh_malloc(strlen(s) + 1)))
        memcpy(s1, s, strlen(s) + 1);
    return s1;
}

int bh_platform_init()
{
    return 0;
}

char*
bh_read_file_to_buffer(const char *filename, int *ret_size)
{
    char *buffer;
    int file;
    int file_size, read_size;
    struct stat stat_buf;

    if (!filename || !ret_size) {
        printf("Read file to buffer failed: invalid filename or ret size.\n");
        return NULL;
    }

    if ((file = open(filename, O_RDONLY, 0)) == -1) {
        printf("Read file to buffer failed: open file %s failed.\n",
               filename);
        return NULL;
    }

    if (fstat(file, &stat_buf) != 0) {
        printf("Read file to buffer failed: fstat file %s failed.\n",
               filename);
        close(file);
        return NULL;
    }

    file_size = stat_buf.st_size;

    if (!(buffer = bh_malloc(file_size))) {
        printf("Read file to buffer failed: alloc memory failed.\n");
        close(file);
        return NULL;
    }

    read_size = read(file, buffer, file_size);
    close(file);

    if (read_size < file_size) {
        printf("Read file to buffer failed: read file content failed.\n");
        bh_free(buffer);
        return NULL;
    }

    *ret_size = file_size;
    return buffer;
}

