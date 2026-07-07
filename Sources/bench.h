#ifndef WOWLOTTO_BENCH_H
#define WOWLOTTO_BENCH_H

// Runs `iterations` RandomX hashes in interpreter + light mode (the only
// config that works on stock iOS: no JIT, ~256 MB, fits a 4 GB iPhone).
// Returns hashes-per-second, or a negative error code:
//   -1 = couldn't allocate cache (out of memory)
//   -2 = couldn't create VM
double randomx_benchmark(int iterations);

#endif
