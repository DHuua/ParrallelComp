#ifndef COLLATZ_COMMON_H
#define COLLATZ_COMMON_H

#include <stdint.h>

#define MOD 1000000007ULL

/* Collatz stopping time for n. */
static inline uint32_t collatz_steps(uint64_t n) {
    uint32_t steps = 0;
    while (n > 1) {
        if ((n & 1ULL) == 0ULL) n >>= 1;
        else n = 3ULL * n + 1ULL;
        steps++;
    }
    return steps;
}

/* N = 10,000,000 + (last4 * 1000), per the worksheet formula. */
static inline uint64_t workload_N(unsigned last4) {
    return 10000000ULL + (uint64_t)last4 * 1000ULL;
}

#endif