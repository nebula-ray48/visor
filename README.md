# Visor Profiler

要件定義書に沿った、Vulkan + Dear ImGui + ImPlot の独立プロファイラです。

## Architecture
- `include/visor/shared_metrics.hpp`: エンジンとプロファイラで共有する固定レイアウト
- `engine-mock`: 共有メモリのデータ送信元
- `visor-profiler`: Vulkan ウィンドウと ImGui/ImPlot ダッシュボード
- `schemas`: 将来の FlatBuffers 拡張用スキーマ
