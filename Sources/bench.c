#include "bench.h"
#include "randomx.h"

#include <pthread.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/sysctl.h>
#include <libkern/OSCacheControl.h>
#include <dlfcn.h>

#ifndef MAP_JIT
#define MAP_JIT 0x0800
#endif

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

// ---------------------------------------------------------------------------
// JIT capability probe. On stock iOS, mmap(MAP_JIT) fails or the page can't be
// executed, so RandomX's JIT would crash. We map a page, write a single `ret`
// instruction, and try to run it — catching any fault — to decide safely
// whether JIT is really usable (i.e. SideJITServer enabled it).
// ---------------------------------------------------------------------------
// pthread_jit_write_protect_np works on iOS but the SDK header marks it
// "unavailable on iOS", so we resolve it at runtime with dlsym.
typedef void (*jit_wp_fn)(int);

static sigjmp_buf g_jbuf;
static void jit_sig(int s) { (void)s; siglongjmp(g_jbuf, 1); }

static int jit_available(void) {
    size_t sz = 4096;
    uint32_t *p = (uint32_t *)mmap(NULL, sz, PROT_READ | PROT_WRITE | PROT_EXEC,
                                   MAP_PRIVATE | MAP_ANON | MAP_JIT, -1, 0);
    if (p == MAP_FAILED) return 0;

    jit_wp_fn wp = (jit_wp_fn)dlsym(RTLD_DEFAULT, "pthread_jit_write_protect_np");

    struct sigaction sa, o_bus, o_segv, o_ill, o_trap;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = jit_sig;
    sigaction(SIGBUS,  &sa, &o_bus);
    sigaction(SIGSEGV, &sa, &o_segv);
    sigaction(SIGILL,  &sa, &o_ill);
    sigaction(SIGTRAP, &sa, &o_trap);

    int ok = 0;
    if (sigsetjmp(g_jbuf, 1) == 0) {
        if (wp) wp(0);                     // writable
        p[0] = 0xD65F03C0u;                // AArch64 RET
        if (wp) wp(1);                     // executable
        sys_icache_invalidate(p, sz);
        ((void (*)(void))p)();             // faults here if JIT not permitted
        ok = 1;
    }

    sigaction(SIGBUS,  &o_bus,  NULL);
    sigaction(SIGSEGV, &o_segv, NULL);
    sigaction(SIGILL,  &o_ill,  NULL);
    sigaction(SIGTRAP, &o_trap, NULL);
    munmap(p, sz);
    return ok;
}

// ---------------------------------------------------------------------------
typedef struct {
    randomx_cache   *cache;
    randomx_dataset *dataset;
    randomx_flags    flags;
    double           deadline;
    long             count;      // -1 on VM failure
} worker_t;

static void *hash_worker(void *arg) {
    worker_t *w = (worker_t *)arg;
    randomx_vm *vm = randomx_create_vm(w->flags, w->cache, w->dataset);
    if (vm == NULL) { w->count = -1; return NULL; }
    unsigned char in[76];  memset(in, 0, sizeof(in));
    unsigned char out[RANDOMX_HASH_SIZE];
    long c = 0; unsigned int n = 0;
    while (now_s() < w->deadline) {
        memcpy(in, &n, sizeof(n)); n++;
        randomx_calculate_hash(vm, in, sizeof(in), out);
        c++;
    }
    randomx_destroy_vm(vm);
    w->count = c;
    return NULL;
}

typedef struct {
    randomx_dataset *ds; randomx_cache *cache;
    unsigned long start, count;
} dsinit_t;

static void *ds_worker(void *arg) {
    dsinit_t *d = (dsinit_t *)arg;
    randomx_init_dataset(d->ds, d->cache, d->start, d->count);
    return NULL;
}

double randomx_benchmark(int seconds, int *out_threads, int *out_mode) {
    const char key[] = "xmr-solo wownero iphone";
    const int nt = cpu_count();
    if (out_threads) *out_threads = nt;

    const int jit = jit_available();
    // A15 has ARMv8 crypto (hardware AES) — the lib is built for armv8-a+crypto.
    randomx_flags base = RANDOMX_FLAG_HARD_AES;
    randomx_flags cflags = jit ? (base | RANDOMX_FLAG_JIT | RANDOMX_FLAG_SECURE)
                               : base;

    randomx_cache *cache = randomx_alloc_cache(cflags);
    if (cache == NULL) {                       // JIT cache failed — go plain
        cache = randomx_alloc_cache(base);
        cflags = base;
        if (cache == NULL) return -1.0;
    }
    randomx_init_cache(cache, key, sizeof(key) - 1);

    randomx_dataset *ds = NULL;
    randomx_flags vmflags = cflags;
    int mode = jit ? 2 : 0;

    if (jit) {
        ds = randomx_alloc_dataset(RANDOMX_FLAG_DEFAULT);
        if (ds != NULL) {                      // fast mode: build the 2 GB set
            unsigned long items = randomx_dataset_item_count();
            pthread_t it[MAX_THREADS]; dsinit_t di[MAX_THREADS];
            unsigned long per = items / nt;
            for (int i = 0; i < nt; i++) {
                di[i].ds = ds; di[i].cache = cache;
                di[i].start = (unsigned long)i * per;
                di[i].count = (i == nt - 1) ? (items - (unsigned long)i * per) : per;
                pthread_create(&it[i], NULL, ds_worker, &di[i]);
            }
            for (int i = 0; i < nt; i++) pthread_join(it[i], NULL);
            vmflags = base | RANDOMX_FLAG_JIT | RANDOMX_FLAG_SECURE
                      | RANDOMX_FLAG_FULL_MEM;
            mode = 1;
        }
    }
    if (out_mode) *out_mode = mode;

    pthread_t th[MAX_THREADS]; worker_t w[MAX_THREADS];
    double t0 = now_s();
    double deadline = t0 + (seconds > 0 ? seconds : 8);
    for (int i = 0; i < nt; i++) {
        w[i].cache    = (mode == 1) ? NULL : cache;   // fast mode uses dataset
        w[i].dataset  = (mode == 1) ? ds : NULL;
        w[i].flags    = vmflags;
        w[i].deadline = deadline;
        w[i].count    = 0;
        pthread_create(&th[i], NULL, hash_worker, &w[i]);
    }
    long total = 0;
    for (int i = 0; i < nt; i++) {
        pthread_join(th[i], NULL);
        if (w[i].count > 0) total += w[i].count;
    }
    double dt = now_s() - t0;

    if (ds) randomx_release_dataset(ds);
    randomx_release_cache(cache);
    if (dt <= 0.0) return 0.0;
    return (double)total / dt;
}
