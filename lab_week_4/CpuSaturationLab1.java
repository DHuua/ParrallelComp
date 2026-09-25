import java.util.concurrent.ForkJoinPool;
import java.util.stream.IntStream;

public class CpuSaturationLab1 {

    static void heavyWork() {
        double acc = 0;
        for (int i = 0; i < 10_000_000; i++) {
            acc += Math.sqrt(i);
        }
    }

    public static void main(String[] args) throws Exception {
        int numThreads = args.length > 0 ? Integer.parseInt(args[0]) : 4;
        System.out.println("Cores available: " + Runtime.getRuntime().availableProcessors());
        System.out.println("Spawning " + numThreads + " threads doing heavy work...");

        ForkJoinPool pool = new ForkJoinPool(numThreads);
        long start = System.nanoTime();
        pool.submit(() ->
            IntStream.range(0, numThreads).parallel().forEach(idx -> heavyWork())
        ).join();
        long elapsed = System.nanoTime() - start;
        pool.shutdown();

        System.out.printf("Elapsed: %.2f ms%n", elapsed / 1_000_000.0);
    }
}