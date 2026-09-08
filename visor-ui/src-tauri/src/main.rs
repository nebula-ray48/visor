// Prevents additional console window on Windows in release, DO NOT REMOVE!!
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use tauri::{Emitter, Manager};
use std::fs::OpenOptions;
use std::thread;
use std::time::Duration;
use memmap2::MmapOptions;

#[tauri::command]
fn start_profiler(app: tauri::AppHandle) {
    thread::spawn(move || {
        let path = if cfg!(windows) {
            "C:\\temp\\visor_vram_metrics.dat"
        } else {
            "/tmp/visor_vram_metrics.dat"
        };

        let file = loop {
            if let Ok(f) = OpenOptions::new().read(true).open(path) {
                break f;
            }
            thread::sleep(Duration::from_millis(500));
        };

        let mmap = unsafe { MmapOptions::new().map(&file).unwrap() };
        let mut last_frame = 0;

        loop {
            let bytes: [u8; 8] = mmap[0..8].try_into().unwrap();
            let current_frame = u64::from_le_bytes(bytes);

            if current_frame != last_frame {
                app.emit("frame-update", current_frame).unwrap();
                last_frame = current_frame;
            }
            thread::sleep(Duration::from_millis(5));
        }
    });
}

fn main() {
    tauri::Builder::default()
        .plugin(tauri_plugin_opener::init())
        .invoke_handler(tauri::generate_handler![start_profiler])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}