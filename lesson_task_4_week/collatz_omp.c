/*
 * collatz_omp.c -- Phase 3: Multi-thread scaling
 *
 * Compile:
 *   gcc -O2 -fopenmp collatz_omp.c -o collatz_omp
 * Run:
 *   ./collatz_omp <last4digits> <num_threads>
 *
 * Same checksum/max as collatz_seq.c so you can verify correctness
 * (checksum must match the sequential run exactly -- if it doesn't,
 * you have a race condition).
 *
 * Run this for k in {1,2,4,8,16(if supported)}, 3 runs each,
 * discard the cold run, average runs 2+3 -> T_k. Fill Table 1:
 *   S_emp(k)  = T_seq / T_k
 *   p         = 2*(1 - 1/S_emp(2))      [derive once, from k=2]
 *   S_theo(k) = 1 / ((1-p) + p/k)
 *   Delta(k)  = S_theo(k) - S_emp(k)
 */
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include "collatz_common.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <last4digits> <num_threads>\n", argv[0]);
        return 1;
    }
    unsigned last4 = (unsigned)atoi(argv[1]);
    int threads = atoi(argv[2]);
    uint64_t N = workload_N(last4);

    omp_set_num_threads(threads);

    double t0 = omp_get_wtime();

    uint32_t max_steps = 0;
    uint64_t checksum = 0;

    #pragma omp parallel for reduction(max:max_steps) reduction(+:checksum) schedule(static)
    for (uint64_t i = 1; i <= N; i++) {
        uint32_t s = collatz_steps(i);
        if (s > max_steps) max_steps = s;
        checksum = (checksum + s) % MOD;
    }
    /* Note: reduction(+:checksum) can overflow the mod invariant across
       threads before the final reduce; re-mod once at the end. */
    checksum %= MOD;

    double t1 = omp_get_wtime();

    printf("N=%llu\n", (unsigned long long)N);
    printf("threads=%d\n", threads);
    printf("max_steps=%u\n", max_steps);
    printf("checksum=%llu\n", (unsigned long long)checksum);
    printf("time_seconds=%.6f\n", t1 - t0);
    return 0;
}