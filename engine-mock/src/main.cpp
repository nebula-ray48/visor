#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>

#include "visor/shared_metrics.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

const size_t SHM_SIZE = 1024 * 1024;

int main() {
    std::cout << "[Visor Engine Mock] Initializing shared memory..." << std::endl;
    void* shared_memory = nullptr;

#ifdef _WIN32
    const char* SHM_PATH = "C:\\temp\\visor_vram_metrics.dat";
    // Windows側も CreateFileMappingA の前に CreateFileA でファイルを作る形に変更が必要ですが、現在はMac環境なので下のPOSIX側が動きます
#else
    const char* SHM_PATH = visor::kSharedMetricsPath;
#endif

#ifndef _WIN32
    int shm_fd = open(SHM_PATH, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        std::cerr << "POSIX: open failed." << std::endl;
        return 1;
    }
    ftruncate(shm_fd, SHM_SIZE);
    shared_memory = mmap(0, SHM_SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0);
#endif

    if (!shared_memory) {
        std::cerr << "Failed to allocate shared memory." << std::endl;
        return 1;
    }

    std::cout << "[Visor Engine Mock] Shared memory mapped successfully at " << shared_memory << std::endl;

    auto* metrics = static_cast<visor::SharedMetrics*>(shared_memory);
    uint64_t frame_count = 0;
    while (true) {
        metrics->frame_count = frame_count;
        metrics->total_vram_mb = 8192;
        metrics->used_vram_mb = 2048 + (frame_count % 1024);
        metrics->allocation_count = 128 + static_cast<uint32_t>(frame_count % 32);
        metrics->frame_time_ms = 14.0F + static_cast<float>(frame_count % 60) / 20.0F;

        if (frame_count % 144 == 0) {
            std::cout << "Wrote frame: " << frame_count << " to shared memory." << std::endl;
        }

        frame_count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}
