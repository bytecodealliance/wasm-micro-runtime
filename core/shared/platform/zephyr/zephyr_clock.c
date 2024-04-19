#include "platform_api_extension.h"
#include "platform_api_vmcore.h"

#include "zephyr_errno.h"

__wasi_errno_t
os_clock_res_get(__wasi_clockid_t clock_id, __wasi_timestamp_t *resolution)
{
    return __WASI_ENOSYS;
}

__wasi_errno_t
os_clock_time_get(__wasi_clockid_t clock_id, __wasi_timestamp_t precision,
                  __wasi_timestamp_t *time)
{
    return __WASI_ENOSYS;
}