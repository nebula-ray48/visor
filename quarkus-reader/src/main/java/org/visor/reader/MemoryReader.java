package org.visor.reader;

import io.quarkus.runtime.StartupEvent;
import jakarta.enterprise.context.ApplicationScoped;
import jakarta.enterprise.event.Observes;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.nio.channels.FileChannel;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardOpenOption;
import java.nio.file.Files;

@ApplicationScoped
public class MemoryReader {

    void onStart(@Observes StartupEvent ev) {
        System.out.println("[Visor Reader] Starting zero-copy shared memory reader (FFM API)...");
        // Java 21の仮想スレッドで非同期の監視ループを開始
        Thread.startVirtualThread(this::readLoop);
    }

    private void readLoop() {
        // C++側で指定した一時ファイルパス
        Path path = Paths.get(System.getProperty("os.name").toLowerCase().contains("win")
            ? "C:\\temp\\visor_vram_metrics.dat"
            : "/tmp/visor_vram_metrics.dat");

        // C++側の起動を待つリトライループ
        while (!Files.exists(path)) {
            try { Thread.sleep(500); } catch (InterruptedException e) {}
        }

        try (FileChannel channel = FileChannel.open(path, StandardOpenOption.READ)) {
            // FileChannel.map により、OSのメモリ領域をJavaの MemorySegment として直接マッピング（ゼロコピー）
            MemorySegment segment = channel.map(FileChannel.MapMode.READ_ONLY, 0, 1024 * 1024, Arena.global());

            long lastFrame = -1;
            while (true) {
                // C++が書き込んだ先頭8バイト(uint64_t)を直接ポインタアクセスで読み取る
                long currentFrame = segment.get(ValueLayout.JAVA_LONG, 0);

                if (currentFrame != lastFrame) {
                    System.out.println("⚡ [Quarkus] Read frame from C++: " + currentFrame);
                    lastFrame = currentFrame;
                }

                // 144Hz (約7ms) 以上の頻度でポーリング
                Thread.sleep(5);
            }
        } catch (Exception e) {
            System.err.println("🚨 Failed to read shared memory: " + e.getMessage());
        }
    }
}