#include "bench.h"
#include "randomx.h"

#include <pthread.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/sysctl.h>

#define MAX_THREADS 16

static double now_s(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}

static int cpu_count(void) {
    int n = 0;
    size_t sz = sizeof(n);
    if (sysctlbyname("hw.logicalcpu", &n, &sz, NULL, 0) != 0 || n < 1) n = 4;
    if (n > MAX_THREADS) n = MAX_THREADS;
    return n;
}

typedef struct {
    randomx_cache *cache;
    double deadline;      // monotonic seconds
    long count;           // out: hashes done (-1 on VM failure)
} worker_t;

static void *worker(void *arg) {
    worker_t *w = (worker_t *)arg;
    // Interpreter + light mode: no JIT (iOS forbids it), 256 MB shared cache.
    randomx_vm *vm = randomx_create_vm(RANDOMX_FLAG_DEFAULT, w->cache, NULL);
    if (vm == NULL) { w->count = -1; return NULL; }

    unsigned char input[76];
    memset(input, 0, sizeof(input));
    unsigned char hash[RANDOMX_HASH_SIZE];

    long c = 0;
    unsigned int nonce = 0;
    while (now_s() < w->deadline) {
        memcpy(input, &nonce, sizeof(nonce));
        nonce++;
        randomx_calculate_hash(vm, input, sizeof(input), hash);
        c++;
    }
    randomx_destroy_vm(vm);
    w->count = c;
    return NULL;
}

double randomx_benchmark(int seconds, int *out_threads) {
    const char key[] = "xmr-solo wownero iphone";
    randomx_cache *cache = randomx_alloc_cache(RANDOMX_FLAG_DEFAULT);
    if (cache == NULL) return -1.0;
    randomx_init_cache(cache, key, sizeof(key) - 1);

    int nt = cpu_count();
    if (out_threads) *out_threads = nt;

    pthread_t th[MAX_THREADS];
    worker_t w[MAX_THREADS];
    double t0 = now_s();
    double deadline = t0 + (seconds > 0 ? seconds : 8);

    for (int i = 0; i < nt; i++) {
        w[i].cache = cache;
        w[i].deadline = deadline;
        w[i].count = 0;
        pthread_create(&th[i], NULL, worker, &w[i]);
    }

    long total = 0;
    for (int i = 0; i < nt; i++) {
        pthread_join(th[i], NULL);
        if (w[i].count > 0) total += w[i].count;
    }
    double dt = now_s() - t0;

    randomx_release_cache(cache);
    if (dt <= 0.0) return 0.0;
    return (double)total / dt;
}
