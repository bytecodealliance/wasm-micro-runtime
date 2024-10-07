#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Buffer {
    int size;
    char *data;
};

struct Buffer *
new_buffer(int size)
{
    struct Buffer *buffer = calloc(1, sizeof(struct Buffer));
    buffer->size = size;
    buffer->data = calloc(1, size);
    return buffer;
}

void
release_buffer(struct Buffer *buffer)
{
    buffer->size = 0;
    free(buffer->data);
    free(buffer);
}

__attribute__((export_name("goodhart_law"))) void
goodhart_law()
{
    struct Buffer *buffer = new_buffer(64);
    // not 0xEE as pre-allocated buffer
    assert(buffer->data[30] == 0);

    snprintf(buffer->data, 60, "%s",
             "When a measure becomes a target, it ceases to be a good measure");
    fprintf(stderr, "%s\n", buffer->data);
    assert(buffer->data[10] == 's');

    release_buffer(buffer);

    // alloc a huge buffer
    struct Buffer *huge_buffer = new_buffer(60000);
    assert(huge_buffer->data != NULL);

    memset(huge_buffer->data, 0xEE, 60000);
    assert((uint8_t)(huge_buffer->data[50000]) == 0xEE);

    release_buffer(huge_buffer);

    // grow 1 page
    bool grow = false;
    for (unsigned i = 0; i < 65535; i++) {
        int buffer_size = 65535 + i * 1000;
        struct Buffer *buffer = new_buffer(buffer_size);
        if (buffer->data == NULL) {
            if (!grow) {
                fprintf(stderr, "Failed to allocate buffer at %u. Let's grow\n",
                        buffer_size);
                grow = true;
                int32_t ret = __builtin_wasm_memory_grow(0, 1);
                assert(ret > 0);
            }
            else {
                break;
            }
        }
        else {
            release_buffer(buffer);
        }
    }
}

int
main()
{
    goodhart_law();
    return EXIT_SUCCESS;
}