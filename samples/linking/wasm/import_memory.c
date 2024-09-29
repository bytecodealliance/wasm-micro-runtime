#include <stdio.h>
#include <stdlib.h>

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
    snprintf(buffer->data, 60, "%s",
             "When a measure becomes a target, it ceases to be a good measure");
    fprintf(stderr, "%s\n", buffer->data);
    release_buffer(buffer);
}

int
main()
{
    goodhart_law();
    return EXIT_SUCCESS;
}