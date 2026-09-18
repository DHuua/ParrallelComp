import java.util.concurrent.ThreadLocalRandom;

public class Part1 {

    static long totalHits = 0;

    static final long TOTAL_POINTS = 50_000_000L;
    static final int NUM_THREADS = 4;

    public static void main(String[] args) throws InterruptedException {
        totalHits = 0;

        long pointsPerThread = TOTAL_POINTS / NUM_THREADS;
        Thread[] threads = new Thread[NUM_THREADS];

        long start = System.nanoTime();

        for (int t = 0; t < NUM_THREADS; t++) {
            threads[t] = new Thread(() -> {
                ThreadLocalRandom rnd = ThreadLocalRandom.current();
                for (long i = 0; i < pointsPerThread; i++) {
                    double x = rnd.nextDouble();
                    double y = rnd.nextDouble();
                    if (x * x + y * y <= 1.0) {
                        totalHits++;
                    }
                }
            });
            threads[t].start();
        }

        for (Thread th : threads) th.join();

        long end = System.nanoTime();
        double pi = 4.0 * totalHits / TOTAL_POINTS;

        System.out.printf("totalHits = %d / %d%n", totalHits, TOTAL_POINTS);
        System.out.printf("pi ~= %.6f%n", pi);
        System.out.printf("time = %.1f ms%n", (end - start) / 1_000_000.0);
    }
}