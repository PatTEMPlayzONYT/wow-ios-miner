#ifndef WOWLOTTO_BENCH_H
#define WOWLOTTO_BENCH_H

// Runs RandomX for `seconds` across ALL CPU cores (interpreter + light mode —
// the only config stock iOS allows). Returns total hashes-per-second, or a
// negative error code. Writes the number of worker threads used to
// *out_threads (may be NULL).
double randomx_benchmark(int seconds, int *out_threads);

#endif
