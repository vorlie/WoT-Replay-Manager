#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]
mod app;

use app::app_main::WotApp;

fn main() -> eframe::Result {
    let options = eframe::NativeOptions::default();
    eframe::run_native(
        "WoT Replay Manager",
        options,
        Box::new(|_cc| Ok(Box::new(WotApp::default()))),
    )
}






