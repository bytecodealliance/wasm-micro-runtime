/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* for O_DIRECT */
#endif

#include "wasm_native.h"
#include "wasm_runtime.h"
#include "wasm_log.h"
#include "wasm_memory.h"
#include "wasm_platform_log.h"

#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <fcntl.h>
#include <errno.h>


#define get_module_inst() \
    wasm_runtime_get_current_module_inst()

#define validate_app_addr(offset, size) \
    wasm_runtime_validate_app_addr(module_inst, offset, size)

#define addr_app_to_native(offset) \
    wasm_runtime_addr_app_to_native(module_inst, offset)

#define addr_native_to_app(ptr) \
    wasm_runtime_addr_native_to_app(module_inst, ptr)

#define module_malloc(size) \
    wasm_runtime_module_malloc(module_inst, size)

#define module_free(offset) \
    wasm_runtime_module_free(module_inst, offset)


static int32
__syscall0_wrapper(int32 arg0)
{
    switch (arg0) {
        case 199: /* getuid */
            /* TODO */
        default:
            bh_printf("##_syscall0 called, syscall id: %d\n", arg0);
    }
    return 0;
}

static int32
__syscall1_wrapper(int32 arg0, int32 arg1)
{
    switch (arg0) {
        case 6: /* close */
            /* TODO */
        default:
            bh_printf("##_syscall1 called, syscall id: %d\n", arg0);
    }
    return 0;
}

static int32
__syscall2_wrapper(int32 arg0, int32 arg1, int32 arg2)
{
    switch (arg0) {
        case 183: /* getcwd */
            /* TODO */
        default:
            bh_printf("##_syscall2 called, syscall id: %d\n", arg0);
    }
    return 0;
}

static int32
__syscall3_wrapper(int32 arg0, int32 arg1, int32 arg2, int32 arg3)
{
    WASMModuleInstance *module_inst = get_module_inst();

    switch (arg0) {
        case 146: /* writev */
        {
            /* Implement syscall 54 and syscall 146 to support printf()
               for non SIDE_MODULE=1 mode */
            struct iovec_app {
                int32 iov_base_offset;
                uint32 iov_len;
            } *vec;
            int32 vec_offset = arg2, str_offset;
            uint32 iov_count = arg3, i;
            int32 count = 0;
            char *iov_base, *str;

            if (!validate_app_addr(vec_offset, sizeof(struct iovec_app)))
                return 0;

            vec = (struct iovec_app *)addr_app_to_native(vec_offset);
            for (i = 0; i < iov_count; i++, vec++) {
                if (vec->iov_len > 0) {
                    if (!validate_app_addr(vec->iov_base_offset, 1))
                        return 0;
                    iov_base = (char*)addr_app_to_native(vec->iov_base_offset);

                    if (!(str_offset = module_malloc(vec->iov_len + 1)))
                        return 0;

                    str = addr_app_to_native(str_offset);

                    memcpy(str, iov_base, vec->iov_len);
                    str[vec->iov_len] = '\0';
                    count += wasm_printf("%s", str);

                    module_free(str_offset);
                }
            }
            return count;
        }
        case 145: /* readv */
        case 3: /* read*/
        case 5: /* open */
        case 221: /* fcntl */
        /* TODO */
        default:
            bh_printf("##_syscall3 called, syscall id: %d\n", arg0);
    }
    return 0;
}

static int32
__syscall4_wrapper(int32 arg0, int32 arg1, int32 arg2,
                   int32 arg3, int32 arg4)
{
    bh_printf("##_syscall4 called, syscall id: %d\n", arg0);
    return 0;
}

static int32
__syscall5_wrapper(int32 arg0, int32 arg1, int32 arg2,
                   int32 arg3, int32 arg4, int32 arg5)
{
    switch (arg0) {
        case 140: /* llseek */
            /* TODO */
        default:
            bh_printf("##_syscall5 called, args[0]: %d\n", arg0);
    }
    return 0;
}

#define GET_EMCC_SYSCALL_ARGS()                                     \
  WASMModuleInstance *module_inst = get_module_inst();              \
  int32 *args;                                                      \
  if (!validate_app_addr(args_off, 1))                              \
    return 0;                                                       \
  args = addr_app_to_native(args_off)                               \

#define EMCC_SYSCALL_WRAPPER0(id)                                   \
  static int32 ___syscall##id##_wrapper(int32 _id) {                \
    return __syscall0_wrapper(id);                                  \
  }

#define EMCC_SYSCALL_WRAPPER1(id)                                   \
  static int32 ___syscall##id##_wrapper(int32 _id, int32 args_off) {\
    GET_EMCC_SYSCALL_ARGS();                                        \
    return __syscall1_wrapper(id, args[0]);                         \
  }

#define EMCC_SYSCALL_WRAPPER2(id)                                   \
  static int32 ___syscall##id##_wrapper(int32 _id, int32 args_off) {\
    GET_EMCC_SYSCALL_ARGS();                                        \
    return __syscall2_wrapper(id, args[0], args[1]);                \
  }

#define EMCC_SYSCALL_WRAPPER3(id)                                   \
  static int32 ___syscall##id##_wrapper(int32 _id, int32 args_off) {\
    GET_EMCC_SYSCALL_ARGS();                                        \
    return __syscall3_wrapper(id, args[0], args[1], args[2]);       \
  }

#define EMCC_SYSCALL_WRAPPER4(id)                                   \
  static int32 ___syscall##id##_wrapper(int32 _id, int32 args_off) {\
    GET_EMCC_SYSCALL_ARGS();                                        \
    return __syscall4_wrapper(id, args[0], args[1], args[2], args[3]);\
  }

#define EMCC_SYSCALL_WRAPPER5(id)                                   \
  static int32 ___syscall##id##_wrapper(int32 _id, int32 args_off) {\
    GET_EMCC_SYSCALL_ARGS();                                        \
    return __syscall5_wrapper(id, args[0], args[1], args[2],        \
                              args[3], args[4]);                    \
  }

EMCC_SYSCALL_WRAPPER0(199)

EMCC_SYSCALL_WRAPPER1(6)

EMCC_SYSCALL_WRAPPER2(183)

EMCC_SYSCALL_WRAPPER3(3)
EMCC_SYSCALL_WRAPPER3(5)
EMCC_SYSCALL_WRAPPER3(54)
EMCC_SYSCALL_WRAPPER3(145)
EMCC_SYSCALL_WRAPPER3(146)
EMCC_SYSCALL_WRAPPER3(221)

EMCC_SYSCALL_WRAPPER5(140)

static int32
getTotalMemory_wrapper()
{
    WASMModuleInstance *module_inst = wasm_runtime_get_current_module_inst();
    WASMMemoryInstance *memory = module_inst->default_memory;
    return NumBytesPerPage * memory->cur_page_count;
}

static int32
enlargeMemory_wrapper()
{
    bool ret;
    WASMModuleInstance *module_inst = wasm_runtime_get_current_module_inst();
    WASMMemoryInstance *memory = module_inst->default_memory;
    uint32 DYNAMICTOP_PTR_offset = module_inst->DYNAMICTOP_PTR_offset;
    uint32 addr_data_offset = *(uint32*)(memory->global_data + DYNAMICTOP_PTR_offset);
    uint32 *DYNAMICTOP_PTR = (uint32*)(memory->memory_data + addr_data_offset);
    uint32 memory_size_expected = *DYNAMICTOP_PTR;
    uint32 total_page_count = (memory_size_expected + NumBytesPerPage - 1) / NumBytesPerPage;

    if (total_page_count < memory->cur_page_count) {
        return 1;
    }
    else {
        ret = wasm_runtime_enlarge_memory(module_inst, total_page_count -
                                          memory->cur_page_count);
        return ret ? 1 : 0;
    }
}

static void
_abort_wrapper(int32 code)
{
    WASMModuleInstance *module_inst = wasm_runtime_get_current_module_inst();
    char buf[32];

    snprintf(buf, sizeof(buf), "env.abort(%i)", code);
    wasm_runtime_set_exception(module_inst, buf);
}

static void
abortOnCannotGrowMemory_wrapper()
{
    WASMModuleInstance *module_inst = wasm_runtime_get_current_module_inst();
    wasm_runtime_set_exception(module_inst, "abort on cannot grow memory");
}

static void
___setErrNo_wrapper(int32 error_no)
{
    errno = error_no;
}

/* TODO: add function parameter/result types check */
#define REG_NATIVE_FUNC(module_name, func_name) \
    {#module_name, #func_name, func_name##_wrapper}

typedef struct WASMNativeFuncDef {
    const char *module_name;
    const char *func_name;
    void *func_ptr;
} WASMNativeFuncDef;

static WASMNativeFuncDef native_func_defs[] = {
    REG_NATIVE_FUNC(env, __syscall0),
    REG_NATIVE_FUNC(env, __syscall1),
    REG_NATIVE_FUNC(env, __syscall2),
    REG_NATIVE_FUNC(env, __syscall3),
    REG_NATIVE_FUNC(env, __syscall4),
    REG_NATIVE_FUNC(env, __syscall5),
    REG_NATIVE_FUNC(env, ___syscall3),
    REG_NATIVE_FUNC(env, ___syscall5),
    REG_NATIVE_FUNC(env, ___syscall6),
    REG_NATIVE_FUNC(env, ___syscall54),
    REG_NATIVE_FUNC(env, ___syscall140),
    REG_NATIVE_FUNC(env, ___syscall145),
    REG_NATIVE_FUNC(env, ___syscall146),
    REG_NATIVE_FUNC(env, ___syscall183),
    REG_NATIVE_FUNC(env, ___syscall199),
    REG_NATIVE_FUNC(env, ___syscall221),
    REG_NATIVE_FUNC(env, _abort),
    REG_NATIVE_FUNC(env, abortOnCannotGrowMemory),
    REG_NATIVE_FUNC(env, enlargeMemory),
    REG_NATIVE_FUNC(env, getTotalMemory),
    REG_NATIVE_FUNC(env, ___setErrNo),
};

void*
wasm_platform_native_func_lookup(const char *module_name,
                                 const char *func_name)
{
    uint32 size = sizeof(native_func_defs) / sizeof(WASMNativeFuncDef);
    WASMNativeFuncDef *func_def = native_func_defs;
    WASMNativeFuncDef *func_def_end = func_def + size;

    if (!module_name || !func_name)
        return NULL;

    while (func_def < func_def_end) {
        if (!strcmp(func_def->module_name, module_name)
            && !strcmp(func_def->func_name, func_name))
            return (void*)(uintptr_t)func_def->func_ptr;
        func_def++;
    }

    return NULL;
}
