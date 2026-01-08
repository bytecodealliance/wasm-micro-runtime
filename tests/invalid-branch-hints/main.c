#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wasm_export.h"

static uint8_t *
read_file(const char *path, uint32_t *out_size)
{
    FILE *fp = fopen(path, "rb");
    long size;
    uint8_t *buf;

    if (!fp) {
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }

    size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return NULL;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return NULL;
    }

    if (size == 0) {
        fclose(fp);
        return NULL;
    }

    buf = (uint8_t *)malloc((size_t)size);
    if (!buf) {
        fclose(fp);
        return NULL;
    }

    if (fread(buf, 1, (size_t)size, fp) != (size_t)size) {
        free(buf);
        fclose(fp);
        return NULL;
    }

    fclose(fp);
    *out_size = (uint32_t)size;
    return buf;
}

int
main(int argc, char **argv)
{
    RuntimeInitArgs init_args;
    wasm_module_t module = NULL;
    uint8_t *buffer = NULL;
    uint32_t size = 0;
    char error_buf[128];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <wasm-file>\n", argv[0]);
        return 1;
    }

    memset(&init_args, 0, sizeof(init_args));
    init_args.mem_alloc_type = Alloc_With_System_Allocator;

    if (!wasm_runtime_full_init(&init_args)) {
        fprintf(stderr, "wasm_runtime_full_init failed\n");
        return 1;
    }

    buffer = read_file(argv[1], &size);
    if (!buffer) {
        fprintf(stderr, "read_file failed\n");
        wasm_runtime_destroy();
        return 1;
    }

    module = wasm_runtime_load(buffer, size, error_buf, sizeof(error_buf));
    if (!module) {
        fprintf(stderr, "wasm_runtime_load failed: %s\n", error_buf);
    }
    else {
        wasm_runtime_unload(module);
    }

    free(buffer);
    wasm_runtime_destroy();
    return 0;
}