#ifndef WOWLOTTO_BENCH_H
#define WOWLOTTO_BENCH_H

// Benchmarks RandomX for `seconds` across all cores, automatically using the
// fastest mode the device allows:
//   *out_mode = 0  interpreter + light  (no JIT available — stock iOS)
//   *out_mode = 1  JIT + fast (2 GB dataset)   — best; needs JIT enabled
//   *out_mode = 2  JIT + light (dataset didn't fit)
// Writes the worker-thread count to *out_threads. Returns total H/s, or a
// negative error code. Both out params may be NULL.
double randomx_benchmark(int seconds, int *out_threads, int *out_mode);

#endif
