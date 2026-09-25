

set -euo pipefail

if [ $# -ne 1 ]; then
    echo "usage: $0 <last4digits>" >&2
    exit 1
fi
LAST4=$1

echo ">> compiling..."
gcc -O2 -fopenmp collatz_seq.c  -o collatz_seq
gcc -O2 -fopenmp collatz_omp.c  -o collatz_omp
gcc -O2 -fopenmp false_sharing.c -o false_sharing
gcc -O2 -fopenmp scheduling.c   -o scheduling

MAXT=$(nproc)
echo ">> logical threads detected: $MAXT (adjust THREAD_LIST below if you want fewer)"

extract() { grep "^$1=" | cut -d= -f2; }

run3_avg() {
    # $1 = command to run (as a string), prints "run1 run2 run3 avg" of time_seconds
    local cmd="$1"
    local r1 r2 r3
    r1=$(eval "$cmd" | extract time_seconds)
    r2=$(eval "$cmd" | extract time_seconds)
    r3=$(eval "$cmd" | extract time_seconds)
    local avg
    avg=$(awk "BEGIN{printf \"%.6f\", ($r2+$r3)/2.0}")
    echo "$r1 $r2 $r3 $avg"
}

echo "phase,variant,threads,run1,run2,run3,avg_seconds" > results.csv

echo ">> Phase 2: sequential baseline"
read r1 r2 r3 avg <<< "$(run3_avg "./collatz_seq $LAST4")"
echo "seq,baseline,1,$r1,$r2,$r3,$avg" >> results.csv
T_SEQ=$avg
echo "T_seq = $T_SEQ s"

echo ">> Phase 3: thread scaling"
THREAD_LIST="1 2 4 8 16"
for k in $THREAD_LIST; do
    if [ "$k" -gt "$MAXT" ]; then continue; fi
    read r1 r2 r3 avg <<< "$(run3_avg "./collatz_omp $LAST4 $k")"
    echo "omp_scaling,k=$k,$k,$r1,$r2,$r3,$avg" >> results.csv
    echo "k=$k -> $avg s"
done

echo ">> Phase 4A: false sharing"
for mode in naive padded reduction; do
    read r1 r2 r3 avg <<< "$(run3_avg "./false_sharing $LAST4 $MAXT $mode")"
    echo "false_sharing,$mode,$MAXT,$r1,$r2,$r3,$avg" >> results.csv
    echo "$mode -> $avg s"
done

echo ">> Phase 4B: scheduling"
declare -A SCHEDULES=(
    [static]="static"
    [static_1000]="static,1000"
    [dynamic_100]="dynamic,100"
    [dynamic_10000]="dynamic,10000"
    [guided]="guided"
)
for name in "${!SCHEDULES[@]}"; do
    export OMP_SCHEDULE="${SCHEDULES[$name]}"
    read r1 r2 r3 avg <<< "$(run3_avg "./scheduling $LAST4 $MAXT")"
    echo "scheduling,$name,$MAXT,$r1,$r2,$r3,$avg" >> results.csv
    echo "$name -> $avg s"
    unset OMP_SCHEDULE
done

echo ">> done. results.csv written. T_seq=$T_SEQ"