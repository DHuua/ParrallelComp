
import csv
import sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

def main():
    path = sys.argv[1] if len(sys.argv) > 1 else "results.csv"

    t_seq = None
    scaling = {}  # k -> avg_seconds

    with open(path) as f:
        for row in csv.DictReader(f):
            if row["phase"] == "seq":
                t_seq = float(row["avg_seconds"])
            elif row["phase"] == "omp_scaling":
                k = int(row["threads"])
                scaling[k] = float(row["avg_seconds"])

    if t_seq is None or not scaling:
        sys.exit("results.csv is missing seq/omp_scaling rows -- run run_benchmarks.sh first")

    ks = sorted(scaling.keys())
    s_emp = {k: t_seq / scaling[k] for k in ks}

    if 2 not in s_emp:
        sys.exit("need a k=2 measurement to derive p")
    p = 2.0 * (1.0 - 1.0 / s_emp[2])
    print(f"Derived parallel fraction p = {p:.4f}")

    s_theo = {k: 1.0 / ((1 - p) + p / k) for k in ks}

    print(f"{'k':>4} {'S_emp':>10} {'S_theo':>10} {'Delta':>10}")
    for k in ks:
        print(f"{k:>4} {s_emp[k]:>10.3f} {s_theo[k]:>10.3f} {s_theo[k]-s_emp[k]:>10.3f}")

    plt.figure(figsize=(7, 5))
    plt.plot(ks, [s_emp[k] for k in ks], "o-", label="Empirical S(k)")
    plt.plot(ks, [s_theo[k] for k in ks], "s--", label=f"Amdahl theoretical (p={p:.3f})")
    plt.plot(ks, ks, ":", color="gray", label="Linear ideal")
    plt.xlabel("Threads (k)")
    plt.ylabel("Speedup S(k)")
    plt.title("Amdahl Reality Gap -- Collatz benchmark")
    plt.legend()
    plt.grid(True, alpha=0.3)
    plt.tight_layout()
    plt.savefig("speedup_plot.png", dpi=150)
    print("saved speedup_plot.png")

if __name__ == "__main__":
    main()