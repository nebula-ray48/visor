import { invoke } from "@tauri-apps/api/core";
import { listen } from "@tauri-apps/api/event";

const frameCounter = document.getElementById("frame-counter");

// Rustから送信される 'frame-update' イベントを監視
listen<number>("frame-update", (event) => {
  if (frameCounter) {
    frameCounter.textContent = event.payload.toString();
  }
});

// 起動時にRust側の監視ループ（start_profilerコマンド）をキックする
invoke("start_profiler");