/*
 * Copyright (C) 2024 Grenoble INP - ESISAR.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <errno.h>

// Zephyr
#define CWD "/lfs"
#define FOLDER_PATH CWD "/folder"
#define FILE_PATH CWD "folder/test.txt"

int
main(int argc, char **argv)
{
    int rc;
    const int zero = 0;
    printf("Hello WebAssembly Module !\n");

    rc = mkdir(FOLDER_PATH, 0777);
    if (rc < 0) {
        rc = errno;
        printf("mkdir failed with error %d\n", rc);
        return -1;
    }
    printf("mkdir returned %d\n", rc);

    FILE *file = fopen(FILE_PATH, "w+");
    if (!file) {
        printf("fopen Failed to open\n");
        return -1;
    }
    printf("fopen Succeed\n");

    const char *data = "Hello, World!";
    size_t len = 13;
    size_t nitems = fwrite(data, sizeof(char), 13, file);
    printf("fwrite returned %d\n", (int)nitems);

    rc = fseek(file, 0, SEEK_SET);
    printf("fseek returned %d\n", rc);

    char buffer[32];
    nitems = fread(buffer, sizeof(char), 32, file);
    printf("fread returned %d\n", (int)nitems);
    printf("buffer read = %s\n", buffer);

    fclose(file);

    return 0;
}
