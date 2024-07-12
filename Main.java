import java.util.Arrays;
import java.util.HashMap;

public class Main {

    private static final int windowSize = 200_000;
    private static final int msgCount = 30_000_000;
    private static final int msgSize = 1024;

    private static long worst = 0;

    private static byte[] createMessage(final int n) {
        final byte[] msg = new byte[msgSize];
        Arrays.fill(msg, (byte) n);
        return msg;
    }

    private static void pushMessage(final byte[][] store, final int id) {
        final long start = System.nanoTime();
        store[id % windowSize] = createMessage(id);
        final long elapsed = System.nanoTime() - start;
        if (elapsed > worst) {
            worst = elapsed;
        }
    }

    public static void main(String[] args) {
        final long start = System.nanoTime();
        final byte[][] store = new byte[windowSize][msgSize];
        for (int i = 0; i < msgCount; i++) {
            pushMessage(store, i);
        }
        final long end = System.nanoTime();
        System.out.println("Worst push time: " + (worst / 1000_000) + " ms");
        System.out.println("Runtime was " + (end - start) / 1_000_000_000 + " sec");
    }
}

