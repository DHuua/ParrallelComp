import java.util.concurrent.ForkJoinPool;
import java.util.stream.IntStream;

public class ThreadSweepLab1 {

    static long runTeam(int numThreads) throws Exception {
        ForkJoinPool pool = new ForkJoinPool(numThreads);
        long start = System.nanoTime();
        try {
            pool.submit(() ->
                IntStream.range(0, numThreads).parallel().forEach(idx -> {
                    // "полезная" нагрузка отсутствует — чистый замер стоимости team creation/join
                    long tid = Thread.currentThread().threadId();
                })
            ).join();
        } finally {
            pool.shutdown();
        }
        return System.nanoTime() - start;
    }

    public static void main(String[] args) throws Exception {
        int[] threadCounts = {1, 2, 4, 8, 16, 32, 64};
        System.out.println("P,time_ns,time_ms");
        for (int p : threadCounts) {
            // warm-up прогон, не учитываем
            runTeam(p);
            // усредняем по 5 прогонам
            long total = 0;
            for (int i = 0; i < 5; i++) total += runTeam(p);
            long avg = total / 5;
            System.out.printf("%d,%d,%.4f%n", p, avg, avg / 1_000_000.0);
        }
    }
}