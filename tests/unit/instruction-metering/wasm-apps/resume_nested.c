#include <stdint.h>

static int32_t
helper_loop(int32_t n)
{
    while (n > 0) {
        n = n - 1;
    }

    return n;
}

int32_t
install(int32_t n)
{
    return helper_loop(n);
}

int32_t
noop(void)
{
    return 9;
}
