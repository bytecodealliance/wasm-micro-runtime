#include <sys/time.h>
#include "system_header.h"

int time_get_ms()
{
    static struct timeval tv;
    gettimeofday(&tv, NULL);
    long long time_in_mill = (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;

    return (int) time_in_mill;
}
