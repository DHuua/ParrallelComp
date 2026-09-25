# Lab Report — Parallel Programming (OpenMP Paradigms)
**Student:** Mukamet Muratbek
**Language:** Java 17+ (OpenJDK 21)
**Lab:** Lab 1 — The Fork-Join Model, Team Creation, and Thread Scoping

---

## Section I: System & Hardware Specifications

- CPU model: *TODO — fill in (`wmic cpu get name` or Task Manager → Performance → CPU)*
- Physical cores: *TODO*
- Logical threads: 12 (`Runtime.getRuntime().availableProcessors()`)
- L1/L2/L3 cache: *TODO*
- OS: Windows *(version — check with `winver`)*
- Java version: OpenJDK 21.0.10 (`java -version`)

---

## Section II: Experimental Methodology

All benchmarks were implemented using the Java 17+ concurrency API
(`java.util.concurrent.ForkJoinPool`, `java.util.stream.IntStream`) as the idiomatic
equivalent of OpenMP's `#pragma omp parallel`.

- **Timer:** `System.nanoTime()` for measuring team-creation/join overhead (nanosecond
  precision); `System.currentTimeMillis()` where millisecond resolution is sufficient.
- **Warm-up:** one untimed run before each measurement series to eliminate JIT
  compilation artifacts (the JVM interprets bytecode before JIT warm-up, which inflates
  the first measurements).
- **Trials:** each configuration of P was averaged over 5 runs.
- **Build/run environment:** `javac` + `java -cp .` (no package declarations — classes
  compiled as top-level classes for simplicity in the lab environment).

---

## Section III: Empirical Results & Visualizations

### Task 1.1 — Verification of Non-Determinism

`ForkJoinLab1.java` was executed 10 consecutive times without modifying the source code.
Full output saved in `task1_1_output.txt`.

**Result:** in 9 out of 10 runs (Runs 1–6, 8–10), the Logical Rank output order was
identical: `Rank 2 → Rank 1 → Rank 3 → Rank 0`. In **Run 7**, the order changed — Rank 3
and Rank 0 were swapped (Rank 3 printed via `worker-4` instead of `worker-3`, as in the
other runs):

| Run | Logical Rank order |
|-----|----------------------|
| 1–6, 8–10 | 2, 1, 3, 0 |
| 7 | 2, 1, 0, 3 |

### Task 1.2 — Thread Oversubscription Sweep

`ThreadSweepLab1.java`: measures the pure cost of team creation + join (no computational
workload inside the threads), averaged over 5 runs per P.

| P (threads) | Time, ms | Increase vs P=1 |
|---|-----------|-----------|
| 1  | 0.3464 | — |
| 2  | 0.4184 | +20.8% |
| 4  | 0.7736 | +123.3% |
| 8  | 0.9209 | +165.8% |
| 16 | 0.8257 | +138.4% |
| 32 | 1.1676 | +237.0% |
| 64 | 1.2205 | +252.3% |

*(insert chart here: X-axis — P, Y-axis — time in ms; the line should show an overall
upward trend with a local dip at P=16)*

**Observation:** execution time grows monotonically with P, aside from a small dip at
P=16 relative to P=8 — within the noise expected from JVM/OS scheduling given the small
sample size (5 runs). Since this task performs no computational work, the entire time
increase is pure overhead from thread allocation, ForkJoinPool registration, and
synchronization at the implicit barrier in `join()`.

### Task 1.3 — CPU Core Saturation Analysis

`CpuSaturationLab1.java`: each thread performs 10,000,000 `Math.sqrt` operations
(`heavyWork()`), run as a separate JVM invocation per P. Machine has 12 logical
processors (`Runtime.getRuntime().availableProcessors()`).

| P (threads) | Elapsed, ms |
|---|-----------|
| 1 | 9.87 |
| 4 | 21.02 |
| 8 | 30.69 |

*(insert Task Manager screenshot here showing per-core CPU utilization during the P=8 run)*

**Observation:** time increases with P even though P never exceeds the 12 available
logical cores (so no oversubscription is occurring here). This is a different effect
than Question 1.2's context-switching penalty, and worth discussing separately:

1. **Insufficient workload granularity.** A single thread completes `heavyWork()` in
   under 10ms, which is too short for parallel execution to amortize fixed costs —
   thread/pool creation, task submission, and stream partitioning overhead (quantified
   separately in Task 1.2 as ~0.8–1.2ms per team) become proportionally significant
   relative to the tiny per-thread workload.
2. **No JIT warm-up across runs.** Each P was launched as a fresh `java` process, so
   `heavyWork()` executes without prior JIT optimization in every single measurement —
   the method runs interpreted or only partially compiled, and this cost is paid
   independently, and non-uniformly, in each run.
3. **Dynamic frequency scaling (Turbo Boost).** Modern CPUs raise per-core clock speed
   substantially above base frequency when only one or few cores are active (P=1), and
   throttle back down as more cores become simultaneously active due to shared thermal
   and power limits (P=4, P=8). This alone can account for single-threaded work
   finishing disproportionately fast relative to multi-threaded runs, independent of any
   scheduling overhead.

This result is a useful counterexample to the assumption that "more threads with enough
free cores always helps": with fine-grained, short-lived work, fixed per-thread and
per-run overheads can dominate and produce apparent negative scaling even with no true
oversubscription.

---

## Section IV: Analytical & Discussion Responses

### Question 1.1 (Scheduling)

The order in which Logical Rank is printed is determined not by the program but by the
operating system's scheduler (Windows Scheduler) and how it distributes CPU time slices
among the OS threads created by `ForkJoinPool`. The Java program only launches 4 logical
threads and has no control over their actual execution order on the CPU cores — that is
entirely the kernel's responsibility. Even with identical source code and input, the
print order is governed by non-deterministic factors: the exact moment each OS thread is
granted a CPU time slice, the state of the scheduler's run queue, and background load
from other processes on the system. Across my 10 runs, the order matched in 9 cases and
diverged in one (Run 7) — this confirms that the observed determinism is incidental
rather than architecturally guaranteed: with more runs or under heavier system load,
divergence would occur more frequently.

### Question 1.2 (Oversubscription)

When the number of software threads P exceeds the number of physical/logical CPU cores,
the operating system is forced to multiplex several threads onto a single core via
**context switching**. Each context switch requires: (1) saving the current thread's
register state into its Thread Control Block (TCB), (2) loading the next thread's saved
state, and (3) updating page tables when switching across processes. Beyond the direct
cost of the switch itself, **cache thrashing** occurs: data that the previous thread kept
"hot" in that core's L1/L2 cache gets evicted by the new thread's data, so when execution
returns to the first thread it must reload from slower L3/RAM (a cache miss). This shows
up indirectly in my data: time grows from 0.92ms (P=8) to 1.22ms (P=64) — a 1.3x increase
with zero computational workload, meaning the increase is purely scheduling and
context-switch overhead.

### Question 1.3 (Barriers)

The implicit barrier at the end of a parallel region (in Java, the blocking `.join()`
call on the pool's submitted task) guarantees that **every** thread in the team has
finished executing its portion of work and that all of their writes to shared memory are
visible to the master thread before execution continues sequentially. Without this
barrier, several hazards would arise: (1) **data races** — the master thread could begin
reading results that workers haven't finished writing yet; (2) **broken memory
visibility** — even if data is technically written, without synchronization there is no
happens-before relationship guaranteed by the Java Memory Model (JMM), so the master
thread could observe a stale value cached from its own core; (3) **logical errors** in
subsequent computation that depends on the full results of the parallel region, if some
workers have not yet finished.

### Question 1.4 (Architectural Mapping)

- **Hardware execution thread (Hyper-Threading / SMT):** a physical mechanism on the CPU
  die where one physical core emulates two logical cores by duplicating part of the
  register file (architectural state) while sharing execution units (ALU, FPU). This
  improves utilization of otherwise idle execution units but does not double the core's
  actual computational throughput.
- **OS kernel thread:** an entity managed by the operating system scheduler; it is the
  kernel thread that is actually queued for execution on a hardware thread. Each kernel
  thread has its own stack, priority, and TCB, and creating/switching between them
  requires kernel system calls (relatively expensive — on the order of microseconds).
- **Language-level green/virtual thread:** a lightweight abstraction managed by the
  language runtime (e.g., Java Virtual Threads from Project Loom, JDK 21+) that
  multiplexes many logical threads on top of a much smaller number of OS kernel threads.
  Creating and switching between virtual threads is orders of magnitude cheaper
  (nanoseconds to tens of nanoseconds), since it requires no kernel system calls —
  scheduling happens entirely inside the JVM.

In this lab, `ForkJoinPool` creates actual OS kernel threads (visible via the `OS ID`
values in the Task 1.1 output) — these are platform threads, not virtual threads.

---

## Section V: Conclusions & Insights

*To be filled in after completing all labs (1–5).*