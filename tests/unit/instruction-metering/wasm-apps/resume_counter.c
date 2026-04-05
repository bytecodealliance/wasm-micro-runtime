#include <stdint.h>

int32_t
countdown(int32_t n)
{
    while (n > 0) {
        n = n - 1;
    }

    return n;
}

int32_t
noop(void)
{
    return 7;
}
