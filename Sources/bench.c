#include "bench.h"
#include "randomx.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

double randomx_benchmark(int iterations) {
    // RANDOMX_FLAG_DEFAULT = interpreter, light mode, no JIT, no large pages.
    // This is the ONLY combination that runs inside a stock (non-jailbroken,
    // no-JIT) iOS app. It's the slow floor; JIT/fast-mode would be faster.
    const char key[] = "xmr-solo wownero iphone bench";
    randomx_flags flags = RANDOMX_FLAG_DEFAULT;

    randomx_cache *cache = randomx_alloc_cache(flags);
    if (cache == NULL) return -1.0;
    randomx_init_cache(cache, key, sizeof(key) - 1);

    randomx_vm *vm = randomx_create_vm(flags, cache, NULL);
    if (vm == NULL) {
        randomx_release_cache(cache);
        return -2.0;
    }

    unsigned char input[76];
    memset(input, 0, sizeof(input));
    unsigned char hash[RANDOMX_HASH_SIZE];

    // one warm-up hash (first call sets up dataset access patterns)
    randomx_calculate_hash(vm, input, sizeof(input), hash);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < iterations; i++) {
        memcpy(input, &i, sizeof(i));            // vary the nonce
        randomx_calculate_hash(vm, input, sizeof(input), hash);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);

    double secs = (double)(t1.tv_sec - t0.tv_sec)
                + (double)(t1.tv_nsec - t0.tv_nsec) / 1e9;

    randomx_destroy_vm(vm);
    randomx_release_cache(cache);

    if (secs <= 0.0) return 0.0;
    return (double)iterations / secs;
}
