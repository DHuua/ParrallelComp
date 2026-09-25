/*
 * scheduling.c -- Phase 4, Experiment B
 *
 * Measures the effect of OpenMP scheduling policy on a workload
 * with highly non-uniform per-iteration cost (Collatz steps).
 *
 * Compile:
 *   gcc -O2 -fopenmp scheduling.c -o scheduling
 * Run (schedule is picked up from OMP_SCHEDULE, clause is "runtime"):
 *   OMP_SCHEDULE="static"        ./scheduling <last4digits> <num_threads>
 *   OMP_SCHEDULE="static,1000"   ./scheduling <last4digits> <num_threads>
 *   OMP_SCHEDULE="dynamic,100"   ./scheduling <last4digits> <num_threads>
 *   OMP_SCHEDULE="dynamic,10000" ./scheduling <last4digits> <num_threads>
 *   OMP_SCHEDULE="guided"        ./scheduling <last4digits> <num_threads>
 *
 * Fill Table 3 with the resulting time_seconds for each row (3 runs,
 * discard cold run, average runs 2+3, like everywhere else).
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

    uint32_t max_steps = 0;
    uint64_t checksum = 0;

    double t0 = omp_get_wtime();
    #pragma omp parallel for schedule(runtime) reduction(max:max_steps) reduction(+:checksum)
    for (uint64_t i = 1; i <= N; i++) {
        uint32_t s = collatz_steps(i);
        if (s > max_steps) max_steps = s;
        checksum = (checksum + s) % MOD;
    }
    checksum %= MOD;
    double t1 = omp_get_wtime();

    printf("N=%llu\n", (unsigned long long)N);
    printf("threads=%d\n", threads);
    printf("max_steps=%u\n", max_steps);
    printf("checksum=%llu\n", (unsigned long long)checksum);
    printf("time_seconds=%.6f\n", t1 - t0);
    return 0;
}