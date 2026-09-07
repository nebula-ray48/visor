package org.visor.reader;

import io.quarkus.runtime.StartupEvent;
import jakarta.enterprise.context.ApplicationScoped;
import jakarta.enterprise.event.Observes;

@ApplicationScoped
public class MemoryReader {

    void onStart(@Observes StartupEvent ev) {
        System.out.println("[Visor Reader] Starting zero-copy shared memory reader (FFM API)...");
        // TODO: Project Panama (FFM API) を用いた共有メモリの読み取りと、Laravelへのデータ転送を実装
    }
}