#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>
#include "wasm_loader_mock.h"
#include "wasm_export.h"
#include "bh_platform.h"

WASMModule *
__wrap_wasm_loader_load(uint8 *buf, uint32 size,
#if WASM_ENABLE_MULTI_MODULE != 0
                 bool main_module,
#endif
                 const LoadArgs *args, char *error_buf, uint32 error_buf_size)
{
    /* Check expected parameters */
    check_expected_ptr(buf);
    check_expected_uint(size);
#if WASM_ENABLE_MULTI_MODULE != 0
    check_expected_uint(main_module);
#endif
    check_expected_ptr(args);

    /* Mock error buffer writing if provided */
    bool populate_error = mock_type(bool);
    if (populate_error && error_buf) {
        const char *error_msg = mock_ptr_type(const char *);
        if (error_msg && error_buf_size > 0) {
            strncpy(error_buf, error_msg, error_buf_size);
            if (error_buf_size > 0) {
                error_buf[error_buf_size - 1] = '\0';
            }
        }
    }

    /* Return mocked module pointer */
    return mock_ptr_type(WASMModule *);
}
