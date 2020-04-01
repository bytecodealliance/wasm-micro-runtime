/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "platform_api_vmcore.h"
#include "platform_api_extension.h"

/* function pointers for executable memory management */
static exec_mem_alloc_func_t exec_mem_alloc_func = NULL;
static exec_mem_free_func_t exec_mem_free_func = NULL;

#if WASM_ENABLE_AOT != 0
#ifdef CONFIG_ARM_MPU
/**
 * This function will allow execute from sram region.
 * This is needed for AOT code because by default all soc will
 * disable the execute from SRAM.
 */
static void
disable_mpu_rasr_xn(void)
{
    u32_t index;
    /* Kept the max index as 8 (irrespective of soc) because the sram
       would most likely be set at index 2. */
    for (index = 0U; index < 8; index++) {
        MPU->RNR = index;
        if (MPU->RASR & MPU_RASR_XN_Msk) {
            MPU->RASR |= ~MPU_RASR_XN_Msk;
        }
    }

}
#endif  /* end of CONFIG_ARM_MPU */
#endif

static int
_stdout_hook_iwasm(int c)
{
    printk("%c", (char)c);
    return 1;
}

int
os_thread_sys_init();

void
os_thread_sys_destroy();

int
bh_platform_init()
{
    extern void __stdout_hook_install(int (*hook)(int));
    /* Enable printf() in Zephyr */
    __stdout_hook_install(_stdout_hook_iwasm);

#if WASM_ENABLE_AOT != 0
#ifdef CONFIG_ARM_MPU
    /* Enable executable memory support */
    disable_mpu_rasr_xn();
#endif
#endif

    return os_thread_sys_init();
}

void
bh_platform_destroy()
{
    os_thread_sys_destroy();
}

void *
os_malloc(unsigned size)
{
    return NULL;
}

void *
os_realloc(void *ptr, unsigned size)
{
    return NULL;
}

void
os_free(void *ptr)
{
}

struct out_context {
    int count;
};

typedef int (*out_func_t)(int c, void *ctx);

static int
char_out(int c, void *ctx)
{
    struct out_context *out_ctx = (struct out_context*)ctx;
    out_ctx->count++;
    return _stdout_hook_iwasm(c);
}

int
os_vprintf(const char *fmt, va_list ap)
{
    struct out_context ctx = { 0 };
    z_vprintk(char_out, &ctx, fmt, ap);
    return ctx.count;
}

void *
os_mmap(void *hint, unsigned int size, int prot, int flags)
{
    if (exec_mem_alloc_func)
        return exec_mem_alloc_func(size);
    else
        return BH_MALLOC(size);
}

void
os_munmap(void *addr, uint32 size)
{
    if (exec_mem_free_func)
        exec_mem_free_func(addr);
    else
        BH_FREE(addr);
}

int
os_mprotect(void *addr, uint32 size, int prot)
{
    return 0;
}

void
os_dcache_flush()
{
#if defined(CONFIG_CPU_CORTEX_M7) && defined(CONFIG_ARM_MPU)
    uint32 key;
    key = irq_lock();
    SCB_CleanDCache();
    irq_unlock(key);
#endif
}

void set_exec_mem_alloc_func(exec_mem_alloc_func_t alloc_func,
                             exec_mem_free_func_t free_func)
{
    exec_mem_alloc_func = alloc_func;
    exec_mem_free_func = free_func;
}

