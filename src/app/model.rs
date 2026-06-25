use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct Settings {
    pub executable_path: String,
    pub replays_path: String,
    pub client_version_xml_path: String,
    pub bottle_name: String,
}

impl Default for Settings {
    fn default() -> Self {
        Self {
            executable_path: String::new(),
            replays_path: String::new(),
            client_version_xml_path: String::new(),
            bottle_name: "WindowsGames".to_string(),
        }
    }
}

#[derive(Debug, Clone)]
pub struct ReplayInfo {
    pub path: String,
    pub player_name: String,
    pub tank: String,
    pub map: String,
    pub date: String,
    pub damage: i64,
    pub server: String,
    pub version: String,
}

