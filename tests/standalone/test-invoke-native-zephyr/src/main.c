/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <stdlib.h>
#include <string.h>
#include "bh_platform.h"
#include "bh_assert.h"
#include "bh_log.h"
#include "wasm_export.h"
#if defined(BUILD_TARGET_RISCV64_LP64) || defined(BUILD_TARGET_RISCV32_ILP32)
#include "test_wasm_riscv64.h"
#else
#include "test_wasm.h"
#endif /* end of BUILD_TARGET_RISCV64_LP64 || BUILD_TARGET_RISCV32_ILP32 */

#include <zephyr.h>
#include <sys/printk.h>

#if defined(BUILD_TARGET_RISCV64_LP64) || defined(BUILD_TARGET_RISCV32_ILP32)
#if defined(BUILD_TARGET_RISCV64_LP64)
#define CONFIG_GLOBAL_HEAP_BUF_SIZE 2400
#define CONFIG_APP_STACK_SIZE 288
#define CONFIG_MAIN_THREAD_STACK_SIZE 4800
#else
#define CONFIG_GLOBAL_HEAP_BUF_SIZE 5000
#define CONFIG_APP_STACK_SIZE 288
#define CONFIG_MAIN_THREAD_STACK_SIZE 5120
#endif
#define CONFIG_APP_HEAP_SIZE 256
#else /* else of BUILD_TARGET_RISCV64_LP64 || BUILD_TARGET_RISCV32_ILP32 */

#define CONFIG_GLOBAL_HEAP_BUF_SIZE 131072
#define CONFIG_APP_STACK_SIZE 8192
#define CONFIG_APP_HEAP_SIZE 8192

#ifdef CONFIG_NO_OPTIMIZATIONS
#define CONFIG_MAIN_THREAD_STACK_SIZE 8192
#else
#define CONFIG_MAIN_THREAD_STACK_SIZE 4096
#endif

#endif /* end of BUILD_TARGET_RISCV64_LP64 || BUILD_TARGET_RISCV32_ILP32 */

static int app_argc;
static char **app_argv;

/**
 * Find the unique main function from a WASM module instance
 * and execute that function.
 *
 * @param module_inst the WASM module instance
 * @param argc the number of arguments
 * @param argv the arguments array
 *
 * @return true if the main function is called, false otherwise.
 */
bool
wasm_application_execute_main(wasm_module_inst_t module_inst,
                              int argc, char *argv[]);

static void*
app_instance_main(wasm_module_inst_t module_inst)
{
    const char *exception;

    wasm_application_execute_main(module_inst, app_argc, app_argv);
    if ((exception = wasm_runtime_get_exception(module_inst)))
        printf("%s\n", exception);
    return NULL;
}

static char global_heap_buf[CONFIG_GLOBAL_HEAP_BUF_SIZE] = { 0 };

#ifdef CONFIG_BOARD_ESP32
#include "mem_alloc.h"
/*
esp32_technical_reference_manual:
"
The capacity of Internal SRAM 1 is 128 KB. Either CPU can read and write this memory at addresses
0x3FFE_0000 ~ 0x3FFF_FFFF of the data bus, and also at addresses 0x400A_0000 ~ 0x400B_FFFF of the
instruction bus.
"

The custom linker script defines dram0_1_seg and map it to 0x400A_0000 ~ 0x400B_FFFF for instruction bus access.
Here we define the buffer that will be placed to dram0_1_seg.
*/
static char esp32_executable_memory_buf[100 * 1024] __attribute__((section (".aot_code_buf"))) = { 0 };

/* the poll allocator for executable memory */
static mem_allocator_t esp32_exec_mem_pool_allocator;

static int
esp32_exec_mem_init()
{
    if (!(esp32_exec_mem_pool_allocator =
                mem_allocator_create(esp32_executable_memory_buf,
                                     sizeof(esp32_executable_memory_buf))))
        return -1;

    return 0;
}

static void
esp32_exec_mem_destroy()
{
    mem_allocator_destroy(esp32_exec_mem_pool_allocator);
}

static void *
esp32_exec_mem_alloc(unsigned int size)
{
    return mem_allocator_malloc(esp32_exec_mem_pool_allocator, size);
}

static void
esp32_exec_mem_free(void *addr)
{
    mem_allocator_free(esp32_exec_mem_pool_allocator, addr);
}
#endif /* end of #ifdef CONFIG_BOARD_ESP32 */

void
test_invoke_native();
void iwasm_main(void *arg1, void *arg2, void *arg3)
{
    int start, end;
    start = k_uptime_get_32();
    uint8 *wasm_file_buf = NULL;
    uint32 wasm_file_size;
    wasm_module_t wasm_module = NULL;
    wasm_module_inst_t wasm_module_inst = NULL;
    RuntimeInitArgs init_args;
    char error_buf[128];
#if WASM_ENABLE_LOG != 0
    int log_verbose_level = 2;
#endif

    (void) arg1;
    (void) arg2;
    (void) arg3;

    memset(&init_args, 0, sizeof(RuntimeInitArgs));

    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

    /* initialize runtime environment */
    if (!wasm_runtime_full_init(&init_args)) {
        printf("Init runtime environment failed.\n");
        return;
    }

#ifdef CONFIG_BOARD_ESP32
    /* Initialize executable memory */
    if (esp32_exec_mem_init() != 0) {
        printf("Init executable memory failed.\n");
        goto fail1;
    }
    /* Set hook functions for executable memory management */
    set_exec_mem_alloc_func(esp32_exec_mem_alloc, esp32_exec_mem_free);
#endif

#if WASM_ENABLE_LOG != 0
    bh_log_set_verbose_level(log_verbose_level);
#endif

test_invoke_native();
return;

    /* load WASM byte buffer from byte buffer of include file */
    wasm_file_buf = (uint8*) wasm_test_file;
    wasm_file_size = sizeof(wasm_test_file);

    /* load WASM module */
    if (!(wasm_module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
                                          error_buf, sizeof(error_buf)))) {
        printf("%s\n", error_buf);
#ifdef CONFIG_BOARD_ESP32
        goto fail1_1;
#else
        goto fail1;
#endif
    }

    /* instantiate the module */
    if (!(wasm_module_inst = wasm_runtime_instantiate(wasm_module,
                                                      CONFIG_APP_STACK_SIZE,
                                                      CONFIG_APP_HEAP_SIZE,
                                                      error_buf,
                                                      sizeof(error_buf)))) {
        printf("%s\n", error_buf);
        goto fail2;
    }

    /* invoke the main function */
    app_instance_main(wasm_module_inst);

    /* destroy the module instance */
    wasm_runtime_deinstantiate(wasm_module_inst);

fail2:
    /* unload the module */
    wasm_runtime_unload(wasm_module);

#ifdef CONFIG_BOARD_ESP32
fail1_1:
    /* destroy executable memory */
    esp32_exec_mem_destroy();
#endif

fail1:
    /* destroy runtime environment */
    wasm_runtime_destroy();

    end = k_uptime_get_32();

    printf("elpase: %d\n", (end - start));
}

#define MAIN_THREAD_STACK_SIZE (CONFIG_MAIN_THREAD_STACK_SIZE)
#define MAIN_THREAD_PRIORITY 5

K_THREAD_STACK_DEFINE(iwasm_main_thread_stack, MAIN_THREAD_STACK_SIZE);
static struct k_thread iwasm_main_thread;

bool iwasm_init(void)
{
    k_tid_t tid = k_thread_create(&iwasm_main_thread, iwasm_main_thread_stack,
                                  MAIN_THREAD_STACK_SIZE,
                                  iwasm_main, NULL, NULL, NULL,
                                  MAIN_THREAD_PRIORITY, 0, K_NO_WAIT);
    return tid ? true : false;
}
void main(void)
{
    iwasm_init();
}

#include "wasm_runtime.h"

static void
test_native1(WASMModuleInstance *module_inst,
             double arg2, float arg3)
{
    printf("##test_native1 result:\n");
    printf("arg2: %f, arg3: %f\n",
           arg2, arg3);
}

static void
test_native_args1(WASMModuleInstance *module_inst,
                  int arg0, uint64_t arg1, float arg2, double arg3,
                  int arg4, int64_t arg5, int64_t arg6, int arg7,
                  double arg8, float arg9, int arg10, double arg11,
                  float arg12, int64_t arg13, uint64_t arg14, float arg15,
                  double arg16, int64_t arg17, uint64_t arg18, float arg19/**/)
{
    printf("##test_native_args1 result:\n");
    printf("arg0: 0x%X, arg1: 0x%X%08X, arg2: %f, arg3: %f\n",
            arg0, (int32)(arg1>>32), (int32)arg1, arg2, arg3);
    printf("arg4: 0x%X, arg5: 0x%X%08X, arg6: 0x%X%08X, arg7: 0x%X\n",
            arg4, (int32)(arg5>>32), (int32)arg5,
            (int32)(arg6>>32), (int32)arg6, arg7);
    printk("arg8: %f, arg9: %f, arg10: 0x%X, arg11: %f\n",
            arg8, arg9, arg10, arg11);

    printf("arg8 check: %s\n", (arg8 == 8888.7777) ? "true" : "false");
    printf("arg9 check: %s\n", (arg9 == 7777.8888f) ? "true" : "false");
#if 1
    printf("arg12: %f, arg13: 0x%X%08X, arg14: 0x%X%08X, arg15: %f\n",
            arg12, (int32)(arg13>>32), (int32)arg13,
            (int32)(arg14>>32), (int32)arg14, arg15);
    printf("arg16: %f, arg17: 0x%X%08X, arg18: 0x%X%08X, arg19: %f\n",
            arg16, (int32)(arg17>>32), (int32)arg17,
            (int32)(arg18>>32), (int32)arg18, arg19);
#endif
}

static void
test_native_args2(WASMModuleInstance *module_inst,
                  uint64_t arg1, float arg2, double arg3,
                  int arg4, int64_t arg5, int64_t arg6, int arg7,
                  double arg8, float arg9, int arg10, double arg11,
                  float arg12, int64_t arg13, uint64_t arg14, float arg15,
                  double arg16, int64_t arg17, uint64_t arg18, float arg19)
{
    printf("##test_native_args2 result:\n");
    printf("arg1: 0x%X%08X, arg2: %f, arg3: %f\n",
            (int32)(arg1>>32), (int32)arg1, arg2, arg3);
    printf("arg4: 0x%X, arg5: 0x%X%08X, arg6: 0x%X%08X, arg7: 0x%X\n",
            arg4, (int32)(arg5>>32), (int32)arg5,
            (int32)(arg6>>32), (int32)arg6, arg7);
    printf("arg8: %f, arg9: %f, arg10: 0x%X, arg11: %f\n",
            arg8, arg9, arg10, arg11);
    printf("arg12: %f, arg13: 0x%X%08X, arg14: 0x%X%08X, arg15: %f\n",
            arg12, (int32)(arg13>>32), (int32)arg13,
            (int32)(arg14>>32), (int32)arg14, arg15);
    printf("arg16: %f, arg17: 0x%X%08X, arg18: 0x%X%08X, arg19: %f\n",
            arg16, (int32)(arg17>>32), (int32)arg17,
            (int32)(arg18>>32), (int32)arg18, arg19);
}

static int32
test_return_i32(WASMModuleInstance *module_inst)
{
    return 0x12345678;
}

static int64
test_return_i64(WASMModuleInstance *module_inst)
{
    return 0x12345678ABCDEFFFll;
}

static float32
test_return_f32(WASMModuleInstance *module_inst)
{
    return 1234.5678f;
}

static float64
test_return_f64(WASMModuleInstance *module_inst)
{
    return 87654321.12345678;
}

#define STORE_I64(addr, value) do {             \
    union { int64 val; uint32 parts[2]; } u;    \
    u.val = (int64)(value);                     \
    (addr)[0] = u.parts[0];                     \
    (addr)[1] = u.parts[1];                     \
  } while (0)

#define STORE_F64(addr, value) do {             \
    union { float64 val; uint32 parts[2]; } u;  \
    u.val = (value);                            \
    (addr)[0] = u.parts[0];                     \
    (addr)[1] = u.parts[1];                     \
  } while (0)

#define I32 VALUE_TYPE_I32
#define I64 VALUE_TYPE_I64
#define F32 VALUE_TYPE_F32
#define F64 VALUE_TYPE_F64

typedef struct WASMTypeTest {
    uint16 param_count;
    /* only one result is supported currently */
    uint16 result_count;
    uint16 param_cell_num;
    uint16 ret_cell_num;
    /* types of params and results */
    uint8 types[128];
} WASMTypeTest;

void
test_invoke_native()
{
    uint32 argv[128], *p = argv;
    WASMTypeTest func_type1 = {
        20, 0, 0, 0,
        { I32, I64, F32, F64,
          I32, I64, I64, I32,
          F64, F32, I32, F64,
          F32, I64, I64, F32,
          F64, I64, I64, F32
        }
    };
    WASMTypeTest func_type2 = {
        19, 0, 0, 0,
        { I64, F32, F64,
          I32, I64, I64, I32,
          F64, F32, I32, F64,
          F32, I64, I64, F32,
          F64, I64, I64, F32
        }
    };
    WASMTypeTest func_type_i32 = { 0, 1, 0, 0, { I32 } };
    WASMTypeTest func_type_i64 = { 0, 1, 0, 0, { I64 } };
    WASMTypeTest func_type_f32 = { 0, 1, 0, 0, { F32 } };
    WASMTypeTest func_type_f64 = { 0, 1, 0, 0, { F64 } };
    WASMModuleInstance module_inst = { 0 };
    WASMExecEnv exec_env = { 0 };

    module_inst.module_type = Wasm_Module_Bytecode;
    exec_env.module_inst = (WASMModuleInstanceCommon *)&module_inst;

    *p++ = 0x12345678;
    STORE_I64(p, 0xFFFFFFFF87654321ll); p += 2;
    *(float32*)p++ = 1234.5678f;
    STORE_F64(p, 567890.1234); p += 2;

    *p++ = 0x11111111;
    STORE_I64(p, 0xAAAAAAAABBBBBBBBll); p += 2;
    STORE_I64(p, 0x7788888899ll); p += 2;
    *p++ = 0x3456;

    STORE_F64(p, 8888.7777); p +=2;
    *(float32*)p++ = 7777.8888f;
    *p++ = 0x66666;
    STORE_F64(p, 999999.88888); p += 2;

    *(float32*)p++ = 555555.22f;
    STORE_I64(p, 0xBBBBBAAAAAAAAll); p += 2;
    STORE_I64(p, 0x3333AAAABBBBll); p += 2;
    *(float32*)p++ = 88.77f;

    STORE_F64(p, 9999.01234); p += 2;
    STORE_I64(p, 0x1111122222222ll); p += 2;
    STORE_I64(p, 0x444455555555ll); p += 2;
    *(float32*)p++ = 77.88f;

    wasm_runtime_invoke_native(&exec_env, test_native_args1,
                               (WASMType*)&func_type1, NULL, NULL,
                               argv, p - argv, argv);
    printf("\n");

    wasm_runtime_invoke_native(&exec_env, test_native_args2,
                               (WASMType*)&func_type2, NULL, NULL,
                               argv+1, p - argv - 1, argv);
    printf("\n");

    wasm_runtime_invoke_native(&exec_env, test_return_i32,
                               (WASMType*)&func_type_i32, NULL, NULL,
                               NULL, 0, argv);
    printf("test_return_i32: 0x%X\n\n", argv[0]);

    wasm_runtime_invoke_native(&exec_env, test_return_i64,
                               (WASMType*)&func_type_i64, NULL, NULL,
                               NULL, 0, argv);
    printf("test_return_i64: 0x%X%08X\n\n",
           (int32)((*(int64*)argv) >> 32), (int32)(*(int64*)argv));

    wasm_runtime_invoke_native(&exec_env, test_return_f32,
                               (WASMType*)&func_type_f32, NULL, NULL,
                               NULL, 0, argv);
    printf("test_return_f32: %f\n\n", *(float32*)argv);

    wasm_runtime_invoke_native(&exec_env, test_return_f64,
                               (WASMType*)&func_type_f64, NULL, NULL,
                               NULL, 0, argv);
    printf("test_return_f64: %f\n\n", *(float64*)argv);
}
