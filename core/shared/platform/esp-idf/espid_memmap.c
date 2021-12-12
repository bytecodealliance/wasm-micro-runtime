#include "platform_api_vmcore.h"
#include "platform_api_extension.h"

void *
os_mmap(void *hint, size_t size, int prot, int flags)
{
    return malloc((int)size);
}

void
os_munmap(void *addr, size_t size)
{
    return free(addr);
}

int
os_mprotect(void *addr, size_t size, int prot)
{
    return 0;
}

void
os_dcache_flush()
{}
