#include <inttypes.h>
#include <stdio.h>
#include <pthread.h>

#define SAFE_CALL(call)                                    \
    do {                                                   \
        int ret = (call);                                  \
        if (ret != 0) {                                    \
            printf("Failed to call %s: %d\n", #call, ret); \
            return -1;                                     \
        }                                                  \
    } while (0)

pthread_cond_t cond;
pthread_mutex_t mp;

int global_value = 0;

void
sleep(int64_t time_ns)
{
    int dummy = 0;
    __builtin_wasm_memory_atomic_wait32(&dummy, dummy, time_ns);
}

__attribute__((export_name("wasi_thread_start"))) int
wasi_thread_start(int thread_id, int *start_arg)
{
    printf("in a new thread; thread_id: %d\n", thread_id);
    printf("sleeping\n");
    sleep(4ll * 1000 * 1000 * 1000);
    printf("done with sleeping!\n");
    pthread_mutex_lock(&mp);
    global_value = 12;

    int ret = pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mp);
    return 0;
}

int
main()
{
    struct timespec timeout = { 0 };
    SAFE_CALL(clock_gettime(CLOCK_REALTIME, &timeout));

    timeout.tv_sec += 10;

    SAFE_CALL(pthread_cond_init(&cond, NULL));

    SAFE_CALL(__wasi_thread_spawn(NULL));

    SAFE_CALL(pthread_mutex_lock(&mp));
    while (global_value != 12) {
        SAFE_CALL(pthread_cond_timedwait(&cond, &mp, &timeout));
    }

    global_value += 4;
    SAFE_CALL(pthread_mutex_unlock(&mp));

    printf("new value %d\n", global_value);

    return 0;
}