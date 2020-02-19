/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "bh_common.h"
#include "bh_platform.h"

#include <unistd.h>
#include "sgx_rsrv_mem_mngr.h"

#define FIXED_BUFFER_SIZE (1<<9)
static bh_print_function_t print_function = NULL;

char *bh_strdup(const char *s)
{
    uint32 size;
    char *s1 = NULL;

    if (s) {
        size = (uint32)(strlen(s) + 1);
        if ((s1 = bh_malloc(size)))
            bh_memcpy_s(s1, size, s, size);
    }
    return s1;
}

int bh_platform_init()
{
    return 0;
}

int putchar(int c)
{
    return 0;
}

int puts(const char *s)
{
    return 0;
}

void bh_set_print_function(bh_print_function_t pf)
{
    print_function = pf;
}

int bh_printf_sgx(const char *message, ...)
{
    if (print_function != NULL) {
        char msg[FIXED_BUFFER_SIZE] = { '\0' };
        va_list ap;
        va_start(ap, message);
        vsnprintf(msg, FIXED_BUFFER_SIZE, message, ap);
        va_end(ap);
        print_function(msg);
    }

    return 0;
}

int bh_vprintf_sgx(const char * format, va_list arg)
{
    if (print_function != NULL) {
        char msg[FIXED_BUFFER_SIZE] = { '\0' };
        vsnprintf(msg, FIXED_BUFFER_SIZE, format, arg);
        print_function(msg);
    }

    return 0;
}

void* bh_mmap(void *hint, unsigned int size, int prot, int flags)
{
    int mprot = 0;
    unsigned alignedSize = (size+4095) & (unsigned)~4095; //Page aligned
    void* ret = NULL;
    sgx_status_t st = 0;

    ret = sgx_alloc_rsrv_mem(alignedSize);
    if (ret == NULL) {
        bh_printf_sgx("bh_mmap(size=%d, alignedSize=%d, prot=0x%x) failed.",size, alignedSize, prot);
        return NULL;
    }
    if (prot & MMAP_PROT_READ)
        mprot |= SGX_PROT_READ;
    if (prot & MMAP_PROT_WRITE)
        mprot |= SGX_PROT_WRITE;
    if (prot & MMAP_PROT_EXEC)
        mprot |= SGX_PROT_EXEC;
    st = sgx_tprotect_rsrv_mem(ret, alignedSize, mprot);
    if (st != SGX_SUCCESS){
	bh_printf_sgx("bh_mmap(size=%d,prot=0x%x) failed to set protect.",size, prot);
        sgx_free_rsrv_mem(ret, alignedSize);
        return NULL;
    }

    return ret;
}

void bh_munmap(void *addr, uint32 size)
{
    sgx_free_rsrv_mem(addr, size);
}

int bh_mprotect(void *addr, uint32 size, int prot)
{
    int mprot = 0;
    sgx_status_t st = 0;

    if (prot & MMAP_PROT_READ)
        mprot |= SGX_PROT_READ;
    if (prot & MMAP_PROT_WRITE)
        mprot |= SGX_PROT_WRITE;
    if (prot & MMAP_PROT_EXEC)
        mprot |= SGX_PROT_EXEC;
    st = sgx_tprotect_rsrv_mem(addr, size, mprot);
    if (st != SGX_SUCCESS) bh_printf_sgx("bh_mprotect(addr=0x%lx,size=%d,prot=0x%x) failed.", addr, size, prot);

    return (st == SGX_SUCCESS? 0:-1);
}
