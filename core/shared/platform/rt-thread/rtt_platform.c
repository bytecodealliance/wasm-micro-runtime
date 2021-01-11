//
// Created by alvis on 2020/12/14.
//

#include <platform_api_vmcore.h>
#include <platform_api_extension.h>

typedef struct os_malloc_list {
    void* real;
    void* used;
    rt_list_t node;
}os_malloc_list_t;

static rt_list_t mem_list;

int bh_platform_init(void)
{
    rt_list_init(&mem_list);
    return 0;
}

void bh_platform_destroy(void)
{
    while(!rt_list_isempty(&mem_list))
    {
        os_malloc_list_t *ptr = rt_list_entry(mem_list.next, os_malloc_list_t, node);
        rt_list_remove(&ptr->node);
        rt_free(ptr->real);
        rt_free(ptr);
    }
}


void *os_malloc(unsigned size)
{
    void* buf = rt_malloc(size + 8);
    if (((rt_ubase_t)buf)&7)
    {
        os_malloc_list_t *mem_item = rt_malloc(sizeof(os_malloc_list_t));
        if (!mem_item)
        {
            rt_free(buf);
            return RT_NULL;
        }
        mem_item->real = buf;
        buf = (void*)((rt_ubase_t)(buf+8)&(~7));
        mem_item->used = buf;
        rt_list_insert_after(&mem_list, &mem_item->node);
    }
    return buf;
}

void *os_realloc(void *ptr, unsigned size)
{
    return rt_realloc(ptr, size);
}

void os_free(void *ptr)
{
    os_malloc_list_t *mem;
    for(rt_list_t* node = mem_list.next; node != &mem_list; node = node->next)
    {
        mem = rt_list_entry(node, os_malloc_list_t, node);
        if (mem->used == ptr)
        {
            rt_list_remove(node);
            rt_free(mem->real);
            rt_free(mem);
            return;
        }
    }
    rt_free(ptr);
}


static char wamr_vprint_buf[RT_CONSOLEBUF_SIZE * 2];
int os_printf(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    rt_size_t len = vsnprintf(wamr_vprint_buf, sizeof(wamr_vprint_buf)-1, format, ap);
    wamr_vprint_buf[len] = 0x00;
    rt_kputs(wamr_vprint_buf);
    va_end(ap);
    return 0;
}

int os_vprintf(const char *format, va_list ap)
{
    rt_size_t len = vsnprintf(wamr_vprint_buf, sizeof(wamr_vprint_buf)-1, format, ap);
    wamr_vprint_buf[len] = 0;
    rt_kputs(wamr_vprint_buf);
    return 0;
}

uint64 os_time_get_boot_microsecond(void)
{
    uint64 ret = rt_tick_get()*1000;
    ret /= RT_TICK_PER_SECOND;
    return ret;
}

korp_tid os_self_thread(void)
{
    return rt_thread_self();
}

uint8 *os_thread_get_stack_boundary(void)
{
    rt_thread_t tid = rt_thread_self();
    return tid->stack_addr;
}

int os_mutex_init(korp_mutex *mutex)
{
    return rt_mutex_init(mutex, "wamr0", RT_IPC_FLAG_FIFO);
}

int os_mutex_destroy(korp_mutex *mutex)
{
    return rt_mutex_detach(mutex);
}

int os_mutex_lock(korp_mutex *mutex)
{
    return rt_mutex_take(mutex, RT_WAITING_FOREVER);
}

int os_mutex_unlock(korp_mutex *mutex)
{
    return rt_mutex_release(mutex);
}


/*
 * functions below was not implement
 */

int os_cond_init(korp_cond *cond)
{
    return 0;
}

int os_cond_destroy(korp_cond *cond)
{
    return 0;
}

int os_cond_wait(korp_cond *cond, korp_mutex *mutex)
{
    return 0;
}

void *os_mmap(void *hint, size_t size, int prot, int flags)
{
    return rt_malloc(size);
}

void os_munmap(void *addr, size_t size)
{
    rt_free(addr);
}

#ifdef OS_ENABLE_HW_BOUND_CHECK
int os_mprotect(void *addr, size_t size, int prot)
{
    return 0;
}
#endif

void os_dcache_flush(void)
{
}
