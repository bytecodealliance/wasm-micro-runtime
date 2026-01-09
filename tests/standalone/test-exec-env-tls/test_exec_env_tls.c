/*
 * Copyright (C) 2024 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/**
 * Test case for exec_env_tls dangling pointer issue.
 *
 * This test reproduces a real bug in WAMR where exec_env_tls is not cleared
 * on early return paths, causing "invalid exec env" errors in subsequent calls.
 *
 * BUG LOCATION: aot_runtime.c and wasm_runtime.c in
 * invoke_native_with_hw_bound_check
 *
 *   // Line ~2475: TLS is SET
 *   wasm_runtime_set_exec_env_tls(exec_env);
 *
 *   // Line ~2487-2489: Early return WITHOUT clearing TLS!
 *   if (!wasm_runtime_detect_native_stack_overflow(exec_env)) {
 *       return false;  // BUG: TLS never cleared!
 *   }
 *
 * SCENARIO: Native stack overflow triggers early return
 *   1. Create exec_env_A
 *   2. Set native_stack_boundary to trigger overflow check failure
 *   3. Call WASM -> fails with "native stack overflow"
 *   4. TLS still points to exec_env_A (never cleared!)
 *   5. Destroy exec_env_A -> TLS is now dangling pointer
 *   6. Create exec_env_B
 *   7. Call WASM -> "invalid exec env" because TLS != exec_env_B
 *
 * The fix: Clear exec_env_tls in wasm_exec_env_destroy() if it points to
 * the exec_env being destroyed.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wasm_export.h"

/* Include internal header for exec_env_tls APIs */
#include "wasm_runtime_common.h"

/* Minimal WASM module that just returns 42
 * Generated with: echo '(module (func (export "test") (result i32) i32.const
 * 42))' | wat2wasm -
 */
static const unsigned char wasm_test_file[] = {
    0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x01, 0x05,
    0x01, 0x60, 0x00, 0x01, 0x7f, 0x03, 0x02, 0x01, 0x00, 0x07,
    0x08, 0x01, 0x04, 0x74, 0x65, 0x73, 0x74, 0x00, 0x00, 0x0a,
    0x06, 0x01, 0x04, 0x00, 0x41, 0x2a, 0x0b
};
#define WASM_FILE_SIZE sizeof(wasm_test_file)

/* Helper to load module with a copy of the buffer */
static wasm_module_t
load_test_module(char *error_buf, uint32_t error_buf_size)
{
    unsigned char *buf_copy = malloc(WASM_FILE_SIZE);
    if (!buf_copy) {
        snprintf(error_buf, error_buf_size, "Failed to allocate buffer");
        return NULL;
    }
    memcpy(buf_copy, wasm_test_file, WASM_FILE_SIZE);
    wasm_module_t module =
        wasm_runtime_load(buf_copy, WASM_FILE_SIZE, error_buf, error_buf_size);
    if (!module) {
        free(buf_copy);
    }
    return module;
}

static char global_heap_buf[4 * 1024 * 1024];

/* ============================================================================
 * TEST: Native stack overflow early return leaves TLS dangling
 * ============================================================================
 * This is the REAL bug scenario - not artificial TLS manipulation.
 */
static int
test_native_stack_overflow_early_return(void)
{
    char error_buf[128];
    wasm_module_t module = NULL;
    wasm_module_inst_t module_inst_a = NULL, module_inst_b = NULL;
    wasm_exec_env_t exec_env_a = NULL, exec_env_b = NULL;
    wasm_function_inst_t func = NULL;
    uint32_t stack_size = 8192, heap_size = 8192;
    uint32_t argv[1];
    int test_passed = 0;
    uint8_t *high_boundary;

    printf("=== TEST: Native stack overflow early return leaves TLS dangling "
           "===\n");
    printf("\n");
    printf("This test reproduces a real bug where:\n");
    printf("  1. wasm_runtime_call_wasm sets exec_env_tls\n");
    printf("  2. Native stack overflow check fails\n");
    printf("  3. Function returns early WITHOUT clearing exec_env_tls\n");
    printf("  4. exec_env is destroyed, TLS becomes dangling pointer\n");
    printf("  5. Next call fails with 'invalid exec env'\n");
    printf("\n");

#ifndef OS_ENABLE_HW_BOUND_CHECK
    printf("SKIP: Hardware bound check is disabled, test not applicable.\n");
    printf("=== TEST: SKIPPED ===\n\n");
    return 1; /* Consider pass when not applicable */
#else

    module = load_test_module(error_buf, sizeof(error_buf));
    if (!module) {
        printf("FAIL: Load module failed: %s\n", error_buf);
        return 0;
    }

    /* Step 1: Create exec_env_A */
    printf("Step 1: Create exec_env_A\n");
    module_inst_a = wasm_runtime_instantiate(module, stack_size, heap_size,
                                             error_buf, sizeof(error_buf));
    if (!module_inst_a) {
        printf("FAIL: Instantiate A failed: %s\n", error_buf);
        goto cleanup;
    }

    exec_env_a = wasm_runtime_create_exec_env(module_inst_a, stack_size);
    if (!exec_env_a) {
        printf("FAIL: Create exec_env A failed\n");
        wasm_runtime_deinstantiate(module_inst_a);
        goto cleanup;
    }
    printf("  exec_env_A = %p\n", (void *)exec_env_a);

    /* Step 2: Set native_stack_boundary to trigger overflow check failure */
    printf("Step 2: Set native_stack_boundary high to trigger overflow\n");
    /* Set boundary to a very high address (above current stack) */
    high_boundary = (uint8_t *)((uintptr_t)&high_boundary + 0x100000);
    wasm_runtime_set_native_stack_boundary(exec_env_a, high_boundary);
    printf("  Set boundary to %p (current stack ~%p)\n", (void *)high_boundary,
           (void *)&high_boundary);

    /* Step 3: Call WASM - should fail with native stack overflow */
    printf("Step 3: Call WASM (expect 'native stack overflow')\n");
    func = wasm_runtime_lookup_function(module_inst_a, "test");
    argv[0] = 0;
    if (wasm_runtime_call_wasm(exec_env_a, func, 0, argv)) {
        printf(
            "  UNEXPECTED: Call succeeded (expected stack overflow failure)\n");
        printf("  This means we couldn't trigger the bug condition.\n");
        wasm_runtime_destroy_exec_env(exec_env_a);
        wasm_runtime_deinstantiate(module_inst_a);
        test_passed = 1; /* Not a test failure, just couldn't trigger */
        goto cleanup;
    }

    const char *exception = wasm_runtime_get_exception(module_inst_a);
    printf("  Call failed as expected: %s\n", exception ? exception : "(null)");

    if (!exception || !strstr(exception, "native stack overflow")) {
        printf("  WARNING: Expected 'native stack overflow', got different "
               "error.\n");
    }

    /* Check exec_env_tls state - this is the bug! */
    WASMExecEnv *tls_after_fail = wasm_runtime_get_exec_env_tls();
    printf("  exec_env_tls after failed call = %p\n", (void *)tls_after_fail);
    if (tls_after_fail == exec_env_a) {
        printf("  BUG CONFIRMED: TLS still points to exec_env_A (not cleared "
               "on early return)\n");
    }
    else if (tls_after_fail == NULL) {
        printf("  TLS is NULL (bug may be fixed or different code path)\n");
    }

    /* Clear exception for next operations */
    wasm_runtime_clear_exception(module_inst_a);

    /* Step 4: Destroy exec_env_A */
    printf("Step 4: Destroy exec_env_A\n");
    void *exec_env_a_addr = exec_env_a;
    wasm_runtime_destroy_exec_env(exec_env_a);
    exec_env_a = NULL;
    wasm_runtime_deinstantiate(module_inst_a);
    module_inst_a = NULL;

    /* Check if fix cleared TLS */
    WASMExecEnv *tls_after_destroy = wasm_runtime_get_exec_env_tls();
    printf("  exec_env_tls after destroy = %p\n", (void *)tls_after_destroy);
    if (tls_after_destroy == NULL) {
        printf("  GOOD: TLS was cleared (fix is working)\n");
    }
    else if (tls_after_destroy == exec_env_a_addr) {
        printf("  BAD: TLS still points to destroyed exec_env (fix needed)\n");
    }

    /* Allocate dummy to prevent address reuse */
    wasm_module_inst_t dummy_inst = wasm_runtime_instantiate(
        module, stack_size, heap_size, error_buf, sizeof(error_buf));
    wasm_exec_env_t dummy_env = NULL;
    if (dummy_inst) {
        dummy_env = wasm_runtime_create_exec_env(dummy_inst, stack_size);
    }

    /* Step 5: Create exec_env_B */
    printf("Step 5: Create exec_env_B\n");
    module_inst_b = wasm_runtime_instantiate(module, stack_size, heap_size,
                                             error_buf, sizeof(error_buf));
    if (!module_inst_b) {
        printf("FAIL: Instantiate B failed: %s\n", error_buf);
        if (dummy_env)
            wasm_runtime_destroy_exec_env(dummy_env);
        if (dummy_inst)
            wasm_runtime_deinstantiate(dummy_inst);
        goto cleanup;
    }

    exec_env_b = wasm_runtime_create_exec_env(module_inst_b, stack_size);
    if (!exec_env_b) {
        printf("FAIL: Create exec_env B failed\n");
        wasm_runtime_deinstantiate(module_inst_b);
        if (dummy_env)
            wasm_runtime_destroy_exec_env(dummy_env);
        if (dummy_inst)
            wasm_runtime_deinstantiate(dummy_inst);
        goto cleanup;
    }
    printf("  exec_env_B = %p\n", (void *)exec_env_b);

    if (exec_env_b == exec_env_a_addr) {
        printf(
            "  WARNING: Same address as destroyed exec_env_A (may mask bug)\n");
    }

    /* Clean up dummy */
    if (dummy_env)
        wasm_runtime_destroy_exec_env(dummy_env);
    if (dummy_inst)
        wasm_runtime_deinstantiate(dummy_inst);

    /* Step 6: Call WASM with exec_env_B */
    printf("Step 6: Call WASM with exec_env_B\n");
    func = wasm_runtime_lookup_function(module_inst_b, "test");
    argv[0] = 0;
    if (!wasm_runtime_call_wasm(exec_env_b, func, 0, argv)) {
        exception = wasm_runtime_get_exception(module_inst_b);
        printf("  FAIL: Call failed: %s\n", exception ? exception : "(null)");
        if (exception && strstr(exception, "invalid exec env")) {
            printf("\n");
            printf("  >>> THIS IS THE BUG! <<<\n");
            printf("  exec_env_tls was not cleared on early return,\n");
            printf("  so it still pointed to destroyed exec_env_A.\n");
            printf("  When calling with exec_env_B, the check\n");
            printf("  'exec_env_tls != exec_env' failed.\n");
            printf("\n");
        }
        wasm_runtime_destroy_exec_env(exec_env_b);
        wasm_runtime_deinstantiate(module_inst_b);
        goto cleanup;
    }

    printf("  SUCCESS: Call returned %u\n", argv[0]);
    test_passed = (argv[0] == 42);

    wasm_runtime_destroy_exec_env(exec_env_b);
    wasm_runtime_deinstantiate(module_inst_b);

cleanup:
    if (module)
        wasm_runtime_unload(module);

    if (test_passed) {
        printf("=== TEST: PASSED ===\n\n");
    }
    else {
        printf("=== TEST: FAILED ===\n\n");
    }
    return test_passed;
#endif /* OS_ENABLE_HW_BOUND_CHECK */
}

/* ============================================================================
 * TEST: Sequential daemon pattern (stress test)
 * ============================================================================
 */
static int
test_sequential_daemon_pattern(void)
{
    char error_buf[128];
    wasm_module_t module = NULL;
    wasm_module_inst_t module_inst = NULL;
    wasm_exec_env_t exec_env = NULL;
    wasm_function_inst_t func = NULL;
    uint32_t stack_size = 8192, heap_size = 8192;
    uint32_t argv[1];
    int i;
    int success_count = 0;

    printf("=== TEST: Sequential daemon pattern (100 iterations) ===\n");

    module = load_test_module(error_buf, sizeof(error_buf));
    if (!module) {
        printf("FAIL: Load module failed: %s\n", error_buf);
        return 0;
    }

    for (i = 0; i < 100; i++) {
        /* Simulate handleRequest: grab instance, execute, destroy */
        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                               error_buf, sizeof(error_buf));
        if (!module_inst) {
            printf("FAIL: Iteration %d: Instantiate failed: %s\n", i,
                   error_buf);
            break;
        }

        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        if (!exec_env) {
            printf("FAIL: Iteration %d: Create exec_env failed\n", i);
            wasm_runtime_deinstantiate(module_inst);
            break;
        }

        func = wasm_runtime_lookup_function(module_inst, "test");
        argv[0] = 0;
        if (!wasm_runtime_call_wasm(exec_env, func, 0, argv)) {
            printf("FAIL: Iteration %d: Call failed: %s\n", i,
                   wasm_runtime_get_exception(module_inst));
            wasm_runtime_destroy_exec_env(exec_env);
            wasm_runtime_deinstantiate(module_inst);
            break;
        }

        if (argv[0] != 42) {
            printf("FAIL: Iteration %d: Wrong result %u\n", i, argv[0]);
            wasm_runtime_destroy_exec_env(exec_env);
            wasm_runtime_deinstantiate(module_inst);
            break;
        }

        wasm_runtime_destroy_exec_env(exec_env);
        wasm_runtime_deinstantiate(module_inst);
        success_count++;
    }

    wasm_runtime_unload(module);

    if (success_count == 100) {
        printf("  All 100 iterations succeeded\n");
        printf("=== TEST: PASSED ===\n\n");
        return 1;
    }
    else {
        printf("  Only %d/100 iterations succeeded\n", success_count);
        printf("=== TEST: FAILED ===\n\n");
        return 0;
    }
}

int
main(int argc, char *argv[])
{
    RuntimeInitArgs init_args;
    int tests_passed = 0;
    int tests_total = 0;

    memset(&init_args, 0, sizeof(RuntimeInitArgs));
    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

    if (!wasm_runtime_full_init(&init_args)) {
        printf("Init runtime failed.\n");
        return 1;
    }

    printf("\n");
    printf("==========================================================\n");
    printf("Testing exec_env_tls cleanup on destroy\n");
    printf("==========================================================\n\n");

#ifdef OS_ENABLE_HW_BOUND_CHECK
    printf("Hardware bound check: ENABLED\n\n");
#else
    printf("Hardware bound check: DISABLED\n");
    printf("(Some tests will be skipped)\n\n");
#endif

    /* Test 1: Native stack overflow early return */
    tests_total++;
    if (test_native_stack_overflow_early_return()) {
        tests_passed++;
    }

    /* Test 2: Sequential daemon pattern */
    tests_total++;
    if (test_sequential_daemon_pattern()) {
        tests_passed++;
    }

    wasm_runtime_destroy();

    printf("==========================================================\n");
    printf("Results: %d/%d tests passed\n", tests_passed, tests_total);
    printf("==========================================================\n");

    return (tests_passed == tests_total) ? 0 : 1;
}
