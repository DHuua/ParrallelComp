/*
 * collatz_seq.c -- Phase 2: Sequential baseline
 *
 * Compile:
 *   gcc -O2 -fopenmp collatz_seq.c -o collatz_seq
 * Run:
 *   ./collatz_seq <last4digits>
 * (last4digits = last 4 digits of your Student ID, e.g. 0984)
 *
 * Prints: N, max stopping time, checksum (sum of steps mod 1e9+7),
 * and wall-clock time via omp_get_wtime().
 *
 * Per the worksheet's benchmarking rules: run this THREE times.
 * Discard Run 1 (cold cache / page-fault warmup). Average Run 2 and
 * Run 3 for T_seq. Record all three raw numbers in Table 1 --
 * do not just report the average.
 */
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include "collatz_common.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <last4digits>\n", argv[0]);
        return 1;
    }
    unsigned last4 = (unsigned)atoi(argv[1]);
    uint64_t N = workload_N(last4);

    double t0 = omp_get_wtime();

    uint32_t max_steps = 0;
    uint64_t checksum = 0;
    for (uint64_t i = 1; i <= N; i++) {
        uint32_t s = collatz_steps(i);
        if (s > max_steps) max_steps = s;
        checksum = (checksum + s) % MOD;
    }

    double t1 = omp_get_wtime();

    printf("N=%llu\n", (unsigned long long)N);
    printf("max_steps=%u\n", max_steps);
    printf("checksum=%llu\n", (unsigned long long)checksum);
    printf("time_seconds=%.6f\n", t1 - t0);
    return 0;
}