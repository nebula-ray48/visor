#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

// 共有メモリの名前と確保するサイズ (1MB)
const char* SHM_NAME = "/visor_vram_metrics";
const size_t SHM_SIZE = 1024 * 1024;

int main() {
    std::cout << "[Visor Engine Mock] Initializing shared memory..." << std::endl;
    void* shared_memory = nullptr;

#ifdef _WIN32
    const char* SHM_PATH = "C:\\temp\\visor_vram_metrics.dat";
    // Windows側も CreateFileMappingA の前に CreateFileA でファイルを作る形に変更が必要ですが、現在はMac環境なので下のPOSIX側が動きます
#else
    const char* SHM_PATH = "/tmp/visor_vram_metrics.dat";
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

    // 約60FPSでダミーのフレームカウンタを共有メモリの先頭に書き込み続ける
    uint64_t frame_count = 0;
    while (true) {
        // TODO: ここを後でFlatBuffers (Conduit) のバイナリデータ書き込みに置き換える
        std::memcpy(shared_memory, &frame_count, sizeof(uint64_t));

        if (frame_count % 144 == 0) {
            std::cout << "Wrote frame: " << frame_count << " to shared memory." << std::endl;
        }

        frame_count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    return 0;
}
