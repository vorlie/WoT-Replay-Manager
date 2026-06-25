use crate::app::model::Settings;

use std::fs;
use std::path::{Path, PathBuf};

fn config_dir() -> Option<PathBuf> {
    directories::ProjectDirs::from("com", "wot", "wot-replay-manager")
        .map(|p| p.config_dir().to_path_buf())
}

fn config_path() -> Option<PathBuf> {
    config_dir().map(|d| d.join("config.ini"))
}

pub fn load_settings() -> Settings {
    let path = match config_path() {
        Some(p) => p,
        None => return Settings::default(),
    };

    if !path.exists() {
        return Settings::default();
    }

    let data = match fs::read_to_string(&path) {
        Ok(s) => s,
        Err(_) => return Settings::default(),
    };

    let parsed: Result<Settings, _> = serde_ini::from_str(&data);
    parsed.unwrap_or_default()
}

pub fn save_settings(settings: &Settings) {
    let path = match config_path() {
        Some(p) => p,
        None => return,
    };

    if let Some(dir) = path.parent() {
        let _ = fs::create_dir_all(dir);
    }

    let data = match serde_ini::to_string(settings) {
        Ok(s) => s,
        Err(_) => return,
    };

    let _ = fs::write(Path::new(&path), data);
}

