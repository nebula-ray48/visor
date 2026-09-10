#pragma once

#include <cstdint>

namespace visor {

inline constexpr char kSharedMetricsPath[] = "/tmp/visor_vram_metrics.dat";
inline constexpr std::uint32_t kSharedMetricsVersion = 1;

struct SharedMetrics {
    std::uint32_t version = kSharedMetricsVersion;
    std::uint32_t size = sizeof(SharedMetrics);
    std::uint64_t frame_count = 0;
    std::uint64_t total_vram_mb = 0;
    std::uint64_t used_vram_mb = 0;
    std::uint32_t allocation_count = 0;
    float frame_time_ms = 0.0F;
};

static_assert(sizeof(SharedMetrics) == 40);

} // namespace visor