/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "platform_api_vmcore.h"
#include "platform_api_extension.h"

typedef struct {
    thread_start_routine_t start;
    void* stack;
    uint32 stack_size;
    void* arg;
} thread_wrapper_arg;

static void *os_thread_wrapper(void *arg)
{
    thread_wrapper_arg * targ = arg;
    thread_start_routine_t start_func = targ->start;
    void *thread_arg = targ->arg;
    os_printf("THREAD CREATED %p\n", &targ);
    targ->stack = (void *)((uintptr_t)(&arg) & (uintptr_t)~0xfff);
    BH_FREE(targ);
    start_func(thread_arg);
#ifdef OS_ENABLE_HW_BOUND_CHECK
    os_thread_destroy_stack_guard_pages();
#endif
    return NULL;
}

int os_thread_create_with_prio(korp_tid *tid, thread_start_routine_t start,
                               void *arg, unsigned int stack_size, int prio)
{
    pthread_attr_t tattr;
    thread_wrapper_arg *targ;

    assert(stack_size > 0);
    assert(tid);
    assert(start);

    pthread_attr_init(&tattr);
    pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_JOINABLE);
    if (pthread_attr_setstacksize(&tattr, stack_size) != 0) {
        os_printf("Invalid thread stack size %u. Min stack size on Linux = %u",
                  stack_size, PTHREAD_STACK_MIN);
        pthread_attr_destroy(&tattr);
        return BHT_ERROR;
    }

    targ = (thread_wrapper_arg*) BH_MALLOC(sizeof(*targ));
    if (!targ) {
        pthread_attr_destroy(&tattr);
        return BHT_ERROR;
    }

    targ->start = start;
    targ->arg = arg;
    targ->stack_size = stack_size;

    if (pthread_create(tid, &tattr, os_thread_wrapper, targ) != 0) {
        pthread_attr_destroy(&tattr);
        BH_FREE(targ);
        return BHT_ERROR;
    }

    pthread_attr_destroy(&tattr);
    return BHT_OK;
}

int os_thread_create(korp_tid *tid, thread_start_routine_t start, void *arg,
                     unsigned int stack_size)
{
    return os_thread_create_with_prio(tid, start, arg, stack_size,
                                      BH_THREAD_DEFAULT_PRIORITY);
}

korp_tid os_self_thread()
{
    return (korp_tid) pthread_self();
}

int os_mutex_init(korp_mutex *mutex)
{
    return pthread_mutex_init(mutex, NULL) == 0 ? BHT_OK : BHT_ERROR;
}

int os_recursive_mutex_init(korp_mutex *mutex)
{
    int ret;

    pthread_mutexattr_t mattr;

    assert(mutex);
    ret = pthread_mutexattr_init(&mattr);
    if (ret)
        return BHT_ERROR;

    pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);
    ret = pthread_mutex_init(mutex, &mattr);
    pthread_mutexattr_destroy(&mattr);

    return ret == 0 ? BHT_OK : BHT_ERROR;
}

int os_mutex_destroy(korp_mutex *mutex)
{
    int ret;

    assert(mutex);
    ret = pthread_mutex_destroy(mutex);

    return ret == 0 ? BHT_OK : BHT_ERROR;
}

int os_mutex_lock(korp_mutex *mutex)
{
    int ret;

    assert(mutex);
    ret = pthread_mutex_lock(mutex);

    return ret == 0 ? BHT_OK : BHT_ERROR;
}

int os_mutex_unlock(korp_mutex *mutex)
{
    int ret;

    assert(mutex);
    ret = pthread_mutex_unlock(mutex);

    return ret == 0 ? BHT_OK : BHT_ERROR;
}

int os_cond_init(korp_cond *cond)
{
    assert(cond);

    if (pthread_cond_init(cond, NULL) != BHT_OK)
        return BHT_ERROR;

    return BHT_OK;
}

int os_cond_destroy(korp_cond *cond)
{
    assert(cond);

    if (pthread_cond_destroy(cond) != BHT_OK)
        return BHT_ERROR;

    return BHT_OK;
}

int os_cond_wait(korp_cond *cond, korp_mutex *mutex)
{
    assert(cond);
    assert(mutex);

    if (pthread_cond_wait(cond, mutex) != BHT_OK)
        return BHT_ERROR;

    return BHT_OK;
}

static void msec_nsec_to_abstime(struct timespec *ts, uint64 usec)
{
    struct timeval tv;
    long int tv_sec_new, tv_nsec_new;

    gettimeofday(&tv, NULL);

    tv_sec_new = (long int)(tv.tv_sec + usec / 1000000);
    if (tv_sec_new >= tv.tv_sec) {
        ts->tv_sec = tv_sec_new;
    }
    else {
        /* integer overflow */
        ts->tv_sec = LONG_MAX;
        os_printf("Warning: os_cond_reltimedwait exceeds limit, "
                  "set to max timeout instead\n");
    }

    tv_nsec_new = (long int)(tv.tv_usec * 1000 + (usec % 1000000) * 1000);
    if (tv.tv_usec * 1000 >= tv.tv_usec
        && tv_nsec_new >= tv.tv_usec * 1000) {
        ts->tv_nsec = tv_nsec_new;
    }
    else {
        /* integer overflow */
        ts->tv_nsec = LONG_MAX;
        os_printf("Warning: os_cond_reltimedwait exceeds limit, "
                  "set to max timeout instead\n");
    }

    if (ts->tv_nsec >= 1000000000L && ts->tv_sec < LONG_MAX) {
        ts->tv_sec++;
        ts->tv_nsec -= 1000000000L;
    }
}

int os_cond_reltimedwait(korp_cond *cond, korp_mutex *mutex, uint64 useconds)
{
    int ret;
    struct timespec abstime;

    if (useconds == BHT_WAIT_FOREVER)
        ret = pthread_cond_wait(cond, mutex);
    else {
        msec_nsec_to_abstime(&abstime, useconds);
        ret = pthread_cond_timedwait(cond, mutex, &abstime);
    }

    if (ret != BHT_OK && ret != ETIMEDOUT)
        return BHT_ERROR;

    return ret;
}

int os_cond_signal(korp_cond *cond)
{
    assert(cond);

    if (pthread_cond_signal(cond) != BHT_OK)
        return BHT_ERROR;

    return BHT_OK;
}

int os_thread_join(korp_tid thread, void **value_ptr)
{
    return pthread_join(thread, value_ptr);
}

int os_thread_detach(korp_tid thread)
{
    return pthread_detach(thread);
}

void os_thread_exit(void *retval)
{
#ifdef OS_ENABLE_HW_BOUND_CHECK
    os_thread_destroy_stack_guard_pages();
#endif
    return pthread_exit(retval);
}

static os_thread_local_attribute uint8 *thread_stack_boundary = NULL;

uint8 *os_thread_get_stack_boundary()
{
    pthread_t self;
#ifdef __linux__
    pthread_attr_t attr;
    size_t guard_size;
#endif
    uint8 *addr = NULL;
    size_t stack_size, max_stack_size;
    int page_size;

    if (thread_stack_boundary)
        return thread_stack_boundary;

    page_size = getpagesize();
    self = pthread_self();
    max_stack_size = (size_t)(APP_THREAD_STACK_SIZE_MAX + page_size - 1)
                     & ~(page_size - 1);

    if (max_stack_size < APP_THREAD_STACK_SIZE_DEFAULT)
        max_stack_size = APP_THREAD_STACK_SIZE_DEFAULT;

#ifdef __linux__
    if (pthread_getattr_np(self, &attr) == 0) {
        pthread_attr_getstack(&attr, (void**)&addr, &stack_size);
        pthread_attr_getguardsize(&attr, &guard_size);
        pthread_attr_destroy(&attr);
        if (stack_size > max_stack_size)
            addr = addr + stack_size - max_stack_size;
        if (guard_size < (size_t)page_size)
            /* Reserved 1 guard page at least for safety */
            guard_size = (size_t)page_size;
        addr += guard_size;
    }
    (void)stack_size;
#elif defined(__APPLE__)
    if ((addr = (uint8*)pthread_get_stackaddr_np(self))) {
        stack_size = pthread_get_stacksize_np(self);
        if (stack_size > max_stack_size)
            addr -= max_stack_size;
        else
            addr -= stack_size;
        /* Reserved 1 guard page at least for safety */
        addr += page_size;
    }
#endif

    thread_stack_boundary = addr;
    return addr;
}

#ifdef OS_ENABLE_HW_BOUND_CHECK

#define SIG_ALT_STACK_SIZE (32 * 1024)

/* Whether the stack pages are touched and guard pages are set */
static os_thread_local_attribute bool stack_guard_pages_inited = false;

/* The signal alternate stack base addr */
static uint8 *sigalt_stack_base_addr;

/* The signal handler passed to os_signal_init() */
static os_signal_handler signal_handler;

#if defined(__GNUC__)
__attribute__((no_sanitize_address)) static uint32
#else
static uint32
#endif
touch_pages(uint8 *stack_min_addr, uint32 page_size)
{
    uint8 sum = 0;
    while (1) {
        volatile uint8 *touch_addr =
            (volatile uint8*)os_alloca(page_size / 2);
        if (touch_addr < stack_min_addr + page_size) {
            sum += *(stack_min_addr + page_size - 1);
            break;
        }
        *touch_addr = 0;
        sum += *touch_addr;
    }
    return sum;
}

bool
os_thread_init_stack_guard_pages()
{
    uint32 page_size = os_getpagesize();
    uint32 guard_page_count = STACK_OVERFLOW_CHECK_GUARD_PAGE_COUNT;
    uint8 *stack_min_addr = os_thread_get_stack_boundary();

    if (!stack_guard_pages_inited) {
        /* Touch each stack page to ensure that it has been mapped: the OS
           may lazily grow the stack mapping as a guard page is hit. */
        (void)touch_pages(stack_min_addr, page_size);
        /* First time to call aot function, protect guard pages */
        if (os_mprotect(stack_min_addr, page_size * guard_page_count,
                        MMAP_PROT_NONE) != 0) {
            return false;
        }
        stack_guard_pages_inited = true;
    }
    return true;
}

void
os_thread_destroy_stack_guard_pages()
{
    if (stack_guard_pages_inited) {
        uint32 page_size = os_getpagesize();
        uint32 guard_page_count = STACK_OVERFLOW_CHECK_GUARD_PAGE_COUNT;
        uint8 *stack_min_addr = os_thread_get_stack_boundary();

        os_mprotect(stack_min_addr, page_size * guard_page_count,
                    MMAP_PROT_READ | MMAP_PROT_WRITE);
        stack_guard_pages_inited = false;
    }
}

static void
mask_signals(int how)
{
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGSEGV);
    sigaddset(&set, SIGBUS);
    pthread_sigmask(how, &set, NULL);
}

__attribute__((noreturn)) static void
signal_callback(int sig_num, siginfo_t *sig_info, void *sig_ucontext)
{
    void *sig_addr = sig_info->si_addr;

    mask_signals(SIG_BLOCK);

    if (signal_handler
        && (sig_num == SIGSEGV || sig_num == SIGBUS)) {
        signal_handler(sig_addr);
    }

    /* signal unhandled */
    switch (sig_num) {
        case SIGSEGV:
            os_printf("unhandled SIGSEGV, si_addr: %p\n", sig_addr);
            break;
        case SIGBUS:
            os_printf("unhandled SIGBUS, si_addr: %p\n", sig_addr);
            break;
        default:
            os_printf("unhandle signal %d, si_addr: %p\n",
                      sig_num, sig_addr);
            break;
    }

    abort();
}

int
os_signal_init(os_signal_handler handler)
{
    int ret = -1;
    struct sigaction sig_act;
    stack_t sigalt_stack_info;
    uint32 map_size = SIG_ALT_STACK_SIZE;
    uint8 *map_addr;

    /* Initialize memory for signal alternate stack */
    if (!(map_addr = os_mmap(NULL, map_size,
                             MMAP_PROT_READ | MMAP_PROT_WRITE,
                             MMAP_MAP_NONE))) {
        os_printf("Failed to mmap memory for alternate stack\n");
        return -1;
    }

    /* Initialize signal alternate stack */
    memset(map_addr, 0, map_size);
    sigalt_stack_info.ss_sp = map_addr;
    sigalt_stack_info.ss_size = map_size;
    sigalt_stack_info.ss_flags = 0;
    if ((ret = sigaltstack(&sigalt_stack_info, NULL)) != 0) {
        goto fail1;
    }

    /* Install signal hanlder */
    sig_act.sa_sigaction = signal_callback;
    sig_act.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_NODEFER;
    sigemptyset(&sig_act.sa_mask);
    if ((ret = sigaction(SIGSEGV, &sig_act, NULL)) != 0
        || (ret = sigaction(SIGBUS, &sig_act, NULL)) != 0) {
        goto fail2;
    }

    sigalt_stack_base_addr = map_addr;
    signal_handler = handler;
    return 0;

fail2:
    memset(&sigalt_stack_info, 0, sizeof(stack_t));
    sigalt_stack_info.ss_flags = SS_DISABLE;
    sigalt_stack_info.ss_size = map_size;
    sigaltstack(&sigalt_stack_info, NULL);
fail1:
    os_munmap(map_addr, map_size);
    return ret;
}

void
os_signal_destroy()
{
    stack_t sigalt_stack_info;

    /* Disable signal alternate stack */
    memset(&sigalt_stack_info, 0, sizeof(stack_t));
    sigalt_stack_info.ss_flags = SS_DISABLE;
    sigalt_stack_info.ss_size = SIG_ALT_STACK_SIZE;
    sigaltstack(&sigalt_stack_info, NULL);

    os_munmap(sigalt_stack_base_addr, SIG_ALT_STACK_SIZE);
}

void
os_signal_unmask()
{
    mask_signals(SIG_UNBLOCK);
}

void
os_sigreturn()
{
#if defined(__APPLE__)
    #define UC_RESET_ALT_STACK 0x80000000
    extern int __sigreturn(void *, int);

    /* It's necessary to call __sigreturn to restore the sigaltstack state
       after exiting the signal handler. */
    __sigreturn(NULL, UC_RESET_ALT_STACK);
#endif
}
#endif /* end of OS_ENABLE_HW_BOUND_CHECK */

