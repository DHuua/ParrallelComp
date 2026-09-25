/*
 * false_sharing.c -- Phase 4, Experiment A
 *
 * Counts how many i in [1,N] have collatz_steps(i) > 100.
 *
 * Compile:
 *   gcc -O2 -fopenmp false_sharing.c -o false_sharing
 * Run (uses max physical thread count you pass in):
 *   ./false_sharing <last4digits> <num_threads> naive
 *   ./false_sharing <last4digits> <num_threads> padded
 *   ./false_sharing <last4digits> <num_threads> reduction
 *
 * Run each variant 3x (discard cold run) at your max PHYSICAL core
 * count and record into Table 2. Effective throughput = N / time.
 * Speedup Penalty Ratio = time_naive / time_mitigated.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "collatz_common.h"

#define THRESHOLD 100
#define MAX_THREADS 256

/* Padded so each thread's counter lives on its own 64-byte cache line. */
typedef struct {
    long count;
    char pad[64 - sizeof(long)];
} PaddedCounter;

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s <last4digits> <num_threads> naive|padded|reduction\n", argv[0]);
        return 1;
    }
    unsigned last4 = (unsigned)atoi(argv[1]);
    int threads = atoi(argv[2]);
    const char *mode = argv[3];
    uint64_t N = workload_N(last4);

    if (threads > MAX_THREADS) {
        fprintf(stderr, "increase MAX_THREADS\n");
        return 1;
    }
    omp_set_num_threads(threads);

    long total_hits = 0;
    double t0, t1;

    if (strcmp(mode, "naive") == 0) {
        /* Implementation 1: plain int array -- adjacent elements share
           a 64-byte cache line, so every increment invalidates other
           threads' cached copies (MESI ping-pong). */
        static long hit_count[MAX_THREADS];
        memset(hit_count, 0, sizeof(hit_count));

        t0 = omp_get_wtime();
        #pragma omp parallel for schedule(static)
        for (uint64_t i = 1; i <= N; i++) {
            if (collatz_steps(i) > THRESHOLD) {
                hit_count[omp_get_thread_num()]++;
            }
        }
        t1 = omp_get_wtime();

        for (int t = 0; t < threads; t++) total_hits += hit_count[t];

    } else if (strcmp(mode, "padded") == 0) {
        /* Implementation 2a: pad each counter out to a full cache line
           so no two threads ever touch the same line. */
        static PaddedCounter hit_count[MAX_THREADS];
        memset(hit_count, 0, sizeof(hit_count));

        t0 = omp_get_wtime();
        #pragma omp parallel for schedule(static)
        for (uint64_t i = 1; i <= N; i++) {
            if (collatz_steps(i) > THRESHOLD) {
                hit_count[omp_get_thread_num()].count++;
            }
        }
        t1 = omp_get_wtime();

        for (int t = 0; t < threads; t++) total_hits += hit_count[t].count;

    } else if (strcmp(mode, "reduction") == 0) {
        /* Implementation 2b: let OpenMP keep a private accumulator per
           thread in a register/stack slot and combine at the end --
           no shared cache line touched at all during the loop. */
        t0 = omp_get_wtime();
        #pragma omp parallel for schedule(static) reduction(+:total_hits)
        for (uint64_t i = 1; i <= N; i++) {
            if (collatz_steps(i) > THRESHOLD) {
                total_hits++;
            }
        }
        t1 = omp_get_wtime();

    } else {
        fprintf(stderr, "unknown mode: %s\n", mode);
        return 1;
    }

    printf("mode=%s\n", mode);
    printf("threads=%d\n", threads);
    printf("N=%llu\n", (unsigned long long)N);
    printf("total_hits=%ld\n", total_hits);
    printf("time_seconds=%.6f\n", t1 - t0);
    printf("throughput_iter_per_sec=%.2f\n", (double)N / (t1 - t0));
    return 0;
}