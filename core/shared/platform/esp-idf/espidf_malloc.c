#include "platform_api_vmcore.h"
#include "platform_api_extension.h"

void *
os_malloc(unsigned size)
{
    void *buf_origin;
    void *buf_fixed;
    uint32 *addr_field;

    buf_origin = malloc(size + 8 + sizeof(uint32));
    buf_fixed = buf_origin + sizeof(void *);
    if ((uint32)buf_fixed & 0x7) {
        buf_fixed = (void *)((size_t)(buf_fixed + 8) & (~7));
    }

    addr_field = buf_fixed - sizeof(uint32);
    *addr_field = (uint32)buf_origin;

    return buf_fixed;
}

void *
os_realloc(void *ptr, unsigned size)
{
    void *mem_origin;
    void *mem_new;
    void *mem_new_fixed;
    int *addr_field;

    if (!ptr) {
        return NULL;
    }

    addr_field = ptr - sizeof(uint32);
    mem_origin = (void *)(*addr_field);
    mem_new = realloc(mem_origin, size + 8 + sizeof(uint32));

    if (mem_origin != mem_new) {
        mem_new_fixed = mem_new + sizeof(uint32);
        if ((uint32)mem_new_fixed & 0x7) {
            mem_new_fixed = (void *)((uint32)(mem_new_fixed + 8) & (~7));
        }

        addr_field = mem_new_fixed - sizeof(uint32);
        *addr_field = (uint32)mem_new;

        return mem_new_fixed;
    }

    return ptr;
}

void
os_free(void *ptr)
{
    void *mem_origin;
    uint32 *addr_field;

    if (ptr) {
        addr_field = ptr - sizeof(uint32);
        mem_origin = (void *)(*addr_field);

        free(mem_origin);
    }
}
