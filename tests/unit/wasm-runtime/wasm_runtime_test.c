#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <cmocka.h>

/* Include the actual implementation with test visibility */
#if WAMR_BUILD_TEST != 1
    #error "WAMR_BUILD_TEST must be defined as 1 to include test implementations"
#endif

#include "wasm_runtime.h"
#include "wasm_loader_mock.h"
#include "wasm_export.h"
#include "wasm_runtime.h"
#include "bh_platform.h"

WASM_RUNTIME_API_INTER void
set_error_buf(char *error_buf, uint32 error_buf_size, const char *string);

WASM_RUNTIME_API_INTER void
set_error_buf_v(char *error_buf, uint32 error_buf_size, const char *format, ...);

/* ==================== set_error_buf() Tests ==================== */

static void
test_set_error_buf_null_buffer(void **state)
{
    (void)state;
    char dummy_buffer[128];

    /* Should not crash or write anything */
    set_error_buf(NULL, sizeof(dummy_buffer), "test error");
    set_error_buf(NULL, 0, "test error");
    set_error_buf(NULL, 128, NULL);
}

static void
test_set_error_buf_valid_buffer(void **state)
{
    (void)state;
    char error_buf[256];
    const char *test_error = "test error message";

    memset(error_buf, 0xAA, sizeof(error_buf)); /* Fill with pattern */
    set_error_buf(error_buf, sizeof(error_buf), test_error);

    /* Check prefix is added */
    assert_string_not_equal(error_buf, test_error);
    assert_true(strstr(error_buf, "WASM module instantiate failed:") != NULL);
    assert_true(strstr(error_buf, test_error) != NULL);
}

static void
test_set_error_buf_buffer_overflow(void **state)
{
    (void)state;
    char error_buf[10]; /* Small buffer */
    const char *long_error =
        "This is a very long error message that should be truncated";

    memset(error_buf, 0xAA, sizeof(error_buf));
    set_error_buf(error_buf, sizeof(error_buf), long_error);

    /* Should be null-terminated and not overflow */
    assert_true(strlen(error_buf) < sizeof(error_buf));
    assert_true(error_buf[sizeof(error_buf) - 1] == '\0'
                || error_buf[sizeof(error_buf) - 1] == (char)0xAA);
}

static void
test_set_error_buf_exact_size(void **state)
{
    (void)state;
    /* Test with buffer exactly sized for message */
    const char *error_msg = "err";
    char error_buf[256];
    int needed_size =
        snprintf(NULL, 0, "WASM module instantiate failed: %s", error_msg) + 1;

    char exact_buf[needed_size];
    set_error_buf(exact_buf, sizeof(exact_buf), error_msg);

    /* Should be null-terminated */
    assert_true(exact_buf[sizeof(exact_buf) - 1] == '\0');
}

static void
test_wasm_load_null_buffer(void **state)
{
    (void)state;
    LoadArgs args = { 0 };
    char error_buf[128];
    WASMModule *result;

    /* The actual behavior depends on wasm_loader_load implementation.
       For unit test, we expect it to call wasm_loader_load with NULL buffer.
       We'll set expectation for NULL buffer. */
    expect_uint_value(__wrap_wasm_loader_load, buf, (uintptr_t)NULL);
    expect_uint_value(__wrap_wasm_loader_load, size, 0);
#if WASM_ENABLE_MULTI_MODULE != 0
    expect_uint_value(__wrap_wasm_loader_load, main_module, false);
#endif
    expect_uint_value(__wrap_wasm_loader_load, args, (uintptr_t)&args);
    will_return(__wrap_wasm_loader_load, false); /* Don't populate error buffer */
    will_return(__wrap_wasm_loader_load, NULL);

    result = wasm_load(NULL, 0,
#if WASM_ENABLE_MULTI_MODULE != 0
                       false,
#endif
                       &args, error_buf, sizeof(error_buf));
    assert_null(result);
}

static void
test_set_error_buf_empty_string(void **state)
{
    (void)state;
    char error_buf[128];

    set_error_buf(error_buf, sizeof(error_buf), "");
    assert_true(strstr(error_buf, "WASM module instantiate failed:") != NULL);
}

static void
test_set_error_buf_null_string(void **state)
{
    (void)state;
    char error_buf[128];

    /* NULL string should be handled (may crash or handle gracefully) */
    set_error_buf(error_buf, sizeof(error_buf), NULL);
    /* If it doesn't crash, we consider it passed */
}

const struct CMUnitTest set_error_buf_tests[] = {
    cmocka_unit_test(test_set_error_buf_null_buffer),
    cmocka_unit_test(test_set_error_buf_valid_buffer),
    cmocka_unit_test(test_set_error_buf_buffer_overflow),
    cmocka_unit_test(test_set_error_buf_exact_size),
    cmocka_unit_test(test_set_error_buf_empty_string),
    cmocka_unit_test(test_set_error_buf_null_string),
};

/* ==================== set_error_buf_v() Tests ==================== */

static void
test_set_error_buf_v_basic_formatting(void **state)
{
    (void)state;
    char error_buf[256];

    set_error_buf_v(error_buf, sizeof(error_buf), "Error %d: %s", 42,
                            "test");

    assert_true(strstr(error_buf, "WASM module instantiate failed:") != NULL);
    assert_true(strstr(error_buf, "Error 42: test") != NULL);
}

static void
test_set_error_buf_v_multiple_args(void **state)
{
    (void)state;
    char error_buf[256];

    set_error_buf_v(error_buf, sizeof(error_buf), "%s %d %u %f", "test",
                            -1, 100, 3.14);
    /* Just verify it doesn't crash with multiple args */
    assert_true(strlen(error_buf) > 0);
}

static void
test_set_error_buf_v_null_format(void **state)
{
    (void)state;
    char error_buf[128];

    /* NULL format - should handle gracefully or crash */
    set_error_buf_v(error_buf, sizeof(error_buf), NULL);
    /* If no crash, test passes */
}

static void
test_set_error_buf_v_internal_buffer_overflow(void **state)
{
    (void)state;
    char error_buf[256];
    /* Create string longer than internal 128-byte buffer */
    char long_str[200];
    memset(long_str, 'A', sizeof(long_str) - 1);
    long_str[sizeof(long_str) - 1] = '\0';

    set_error_buf_v(error_buf, sizeof(error_buf), "%s", long_str);
    /* Should truncate internally but not crash */
    assert_true(strlen(error_buf) < sizeof(error_buf));
}

const struct CMUnitTest set_error_buf_v_tests[] = {
    cmocka_unit_test(test_set_error_buf_v_basic_formatting),
    cmocka_unit_test(test_set_error_buf_v_multiple_args),
    cmocka_unit_test(test_set_error_buf_v_null_format),
    cmocka_unit_test(test_set_error_buf_v_internal_buffer_overflow),
};

/* ==================== wasm_load() Tests ==================== */

static void
test_wasm_load_success(void **state)
{
    (void)state;
    uint8_t buffer[100] = { 0 };
    LoadArgs args = { 0 };
    char error_buf[128];
    WASMModule *mock_module = (WASMModule *)0xDEADBEEF;

    /* Setup expectations for wasm_loader_load */
    expect_uint_value(__wrap_wasm_loader_load, buf, (uintptr_t)buffer);
    expect_uint_value(__wrap_wasm_loader_load, size, 100);
#if WASM_ENABLE_MULTI_MODULE != 0
    expect_uint_value(__wrap_wasm_loader_load, main_module, true);
#endif
    expect_uint_value(__wrap_wasm_loader_load, args, (uintptr_t)&args);
    /* Don't populate error buffer */
    will_return(__wrap_wasm_loader_load, false);
    will_return(__wrap_wasm_loader_load, mock_module);

    WASMModule *result = wasm_load(buffer, 100,
#if WASM_ENABLE_MULTI_MODULE != 0
                                   true,
#endif
                                   &args, error_buf, sizeof(error_buf));

    assert_ptr_equal(result, mock_module);
}

static void
test_wasm_load_error_buffer_populated(void **state)
{
    (void)state;
    uint8_t buffer[50] = { 0 };
    LoadArgs args = { 0 };
    char error_buf[128] = { 0 };
    const char *error_msg = "Loader error";
    WASMModule *mock_module = NULL; /* Simulate failure */

    /* Expect wasm_loader_load to be called and return NULL */
    expect_uint_value(__wrap_wasm_loader_load, buf, (uintptr_t)buffer);
    expect_uint_value(__wrap_wasm_loader_load, size, 50);
#if WASM_ENABLE_MULTI_MODULE != 0
    expect_uint_value(__wrap_wasm_loader_load, main_module, true);
#endif
    expect_uint_value(__wrap_wasm_loader_load, args, (uintptr_t)&args);
    /* Tell mock to populate error buffer */
    will_return(__wrap_wasm_loader_load, true); /* populate error_buf */
    will_return(__wrap_wasm_loader_load, error_msg);
    will_return(__wrap_wasm_loader_load, mock_module);

    WASMModule *result = wasm_load(buffer, 50,
#if WASM_ENABLE_MULTI_MODULE != 0
                                   true,
#endif
                                   &args, error_buf, sizeof(error_buf));

    assert_null(result);
    /* Check that error buffer was populated (by mock) */
    assert_string_equal(error_buf, error_msg);
}

static void
test_wasm_load_null_error_buffer(void **state)
{
    (void)state;
    uint8_t buffer[50] = { 0 };
    LoadArgs args = { 0 };
    WASMModule *mock_module = (WASMModule *)0xDEADBEEF;

    /* Should work even with NULL error buffer */
    expect_uint_value(__wrap_wasm_loader_load, buf, (uintptr_t)buffer);
    expect_uint_value(__wrap_wasm_loader_load, size, 50);
#if WASM_ENABLE_MULTI_MODULE != 0
    expect_uint_value(__wrap_wasm_loader_load, main_module, true);
#endif
    expect_uint_value(__wrap_wasm_loader_load, args, (uintptr_t)&args);
    will_return(__wrap_wasm_loader_load,
                false); /* Don't populate error buffer (it's NULL) */
    will_return(__wrap_wasm_loader_load, mock_module);

    WASMModule *result = wasm_load(buffer, 50,
#if WASM_ENABLE_MULTI_MODULE != 0
                                   true,
#endif
                                   &args, NULL, 0);

    assert_ptr_equal(result, mock_module);
}

static void
test_wasm_load_zero_error_buffer_size(void **state)
{
    (void)state;
    uint8_t buffer[50] = { 0 };
    LoadArgs args = { 0 };
    char error_buf[128];
    WASMModule *mock_module = (WASMModule *)0xDEADBEEF;

    /* Should work even with zero error buffer size */
    expect_uint_value(__wrap_wasm_loader_load, buf, (uintptr_t)buffer);
    expect_uint_value(__wrap_wasm_loader_load, size, 50);
#if WASM_ENABLE_MULTI_MODULE != 0
    expect_uint_value(__wrap_wasm_loader_load, main_module, true);
#endif
    expect_uint_value(__wrap_wasm_loader_load, args, (uintptr_t)&args);
    will_return(__wrap_wasm_loader_load,
                false); /* Don't populate error buffer (size is 0) */
    will_return(__wrap_wasm_loader_load, mock_module);

    WASMModule *result = wasm_load(buffer, 50,
#if WASM_ENABLE_MULTI_MODULE != 0
                                   true,
#endif
                                   &args, error_buf, 0);

    assert_ptr_equal(result, mock_module);
}

/* Test group for multi-module enabled case */
const struct CMUnitTest wasm_load_tests_multi_module_enabled[] = {
    cmocka_unit_test(test_wasm_load_success),
    cmocka_unit_test(test_wasm_load_null_buffer),
    cmocka_unit_test(test_wasm_load_error_buffer_populated),
    cmocka_unit_test(test_wasm_load_null_error_buffer),
    cmocka_unit_test(test_wasm_load_zero_error_buffer_size),
};

/* Test group for multi-module disabled case (identical tests,
   compiled conditionally) */
const struct CMUnitTest wasm_load_tests_multi_module_disabled[] = {
    cmocka_unit_test(test_wasm_load_success),
    cmocka_unit_test(test_wasm_load_null_buffer),
    cmocka_unit_test(test_wasm_load_error_buffer_populated),
    cmocka_unit_test(test_wasm_load_null_error_buffer),
    cmocka_unit_test(test_wasm_load_zero_error_buffer_size),
};