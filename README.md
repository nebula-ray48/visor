# Visor (VRAM Profiler)

High-performance, cross-platform VRAM and memory bandwidth profiler.

## Architecture
- **Engine Mock (C++23):** Writes memory metrics to OS Shared Memory (Memory-Mapped Files).
- **Conduit (FlatBuffers):** Cross-language data serialization schema.
- **Reader (Java):** Reads shared memory via FFM API (Project Panama) with zero-copy.
- **Dashboard (PHP/Laravel):** Real-time web UI using FrankenPHP.
