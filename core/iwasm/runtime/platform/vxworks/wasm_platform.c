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

#include "wasm_log.h"
#include "wasm_platform.h"
#include "wasm_memory.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dlfcn.h>

bool is_little_endian = false;

bool __is_little_endian()
{
    union w
    {
        int a;
        char b;
    }c;

    c.a = 1;
    return (c.b == 1);
}

int
wasm_platform_init()
{
    if (__is_little_endian())
        is_little_endian = true;

    return 0;
}

char*
wasm_read_file_to_buffer(const char *filename, int *ret_size)
{
    char *buffer;
    int file;
    int file_size, read_size;
    struct stat stat_buf;

    if (!filename || !ret_size) {
        LOG_ERROR("Read file to buffer failed: invalid filename or ret size.\n");
        return NULL;
    }

    if ((file = open(filename, O_RDONLY, 0)) == -1) {
        LOG_ERROR("Read file to buffer failed: open file %s failed.\n",
                  filename);
        return NULL;
    }

    if (fstat(file, &stat_buf) != 0) {
        LOG_ERROR("Read file to buffer failed: fstat file %s failed.\n",
                  filename);
        close(file);
        return NULL;
    }

    file_size = stat_buf.st_size;

    if (!(buffer = wasm_malloc(file_size))) {
        LOG_ERROR("Read file to buffer failed: alloc memory failed.\n");
        close(file);
        return NULL;
    }

    read_size = read(file, buffer, file_size);
    close(file);

    if (read_size < file_size) {
        LOG_ERROR("Read file to buffer failed: read file content failed.\n");
        wasm_free(buffer);
        return NULL;
    }

    *ret_size = file_size;
    return buffer;
}

