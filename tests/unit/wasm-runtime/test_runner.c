#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

/* Include test implementations */
#include "wasm_runtime_test.c"

int
main(void)
{
    int result = 0;

    /* Run all test groups */
    // result |= cmocka_run_group_tests(set_error_buf_tests, NULL, NULL);
    // result |= cmocka_run_group_tests(set_error_buf_v_tests, NULL, NULL);

    /* Run tests with multi-module enabled */
    result |= cmocka_run_group_tests(wasm_load_tests_multi_module_enabled, NULL,
                                     NULL);

    /* Run tests with multi-module disabled (if compiled) */
#if WASM_ENABLE_MULTI_MODULE == 0
    result |= cmocka_run_group_tests(wasm_load_tests_multi_module_disabled,
                                     NULL, NULL);
#endif

    return result;
}