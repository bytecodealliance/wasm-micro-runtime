/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_platform.h"
#include "bh_memory.h"
#include "bh_common.h"
#include <stdlib.h>
#include <string.h>
#ifdef CONFIG_ARM_MPU
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#endif

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

static int
_stdout_hook_iwasm(int c)
{
    printk("%c", (char)c);
    return 1;
}

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

    return 0;
}

void *
bh_mmap(void *hint, unsigned int size, int prot, int flags)
{
    return bh_malloc(size);
}

void
bh_munmap(void *addr, uint32 size)
{
    return bh_free(addr);
}

int
bh_mprotect(void *addr, uint32 size, int prot)
{
    return 0;
}

void
bh_dcache_flush()
{
#if defined(CONFIG_CPU_CORTEX_M7) && defined(CONFIG_ARM_MPU)
    uint32 key;
    key = irq_lock();
    SCB_CleanDCache();
    irq_unlock(key);
#endif
}
