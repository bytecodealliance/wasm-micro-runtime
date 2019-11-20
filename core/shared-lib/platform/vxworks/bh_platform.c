/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"
#include "bh_common.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>


char *bh_strdup(const char *s)
{
    uint32 size;
    char *s1 = NULL;

    if (s) {
        size = (uint32)(strlen(s) + 1);
        if ((s1 = bh_malloc(size)))
            bh_memcpy_s(s1, size, s, size);
    }
    return s1;
}

int bh_platform_init()
{
    return 0;
}

char*
bh_read_file_to_buffer(const char *filename, uint32 *ret_size)
{
    char *buffer;
    int file;
    uint32 file_size, read_size;
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

    file_size = (uint32)stat_buf.st_size;

    if (!(buffer = bh_malloc(file_size))) {
        printf("Read file to buffer failed: alloc memory failed.\n");
        close(file);
        return NULL;
    }

    read_size = (uint32)read(file, buffer, file_size);
    close(file);

    if (read_size < file_size) {
        printf("Read file to buffer failed: read file content failed.\n");
        bh_free(buffer);
        return NULL;
    }

    *ret_size = file_size;
    return buffer;
}

