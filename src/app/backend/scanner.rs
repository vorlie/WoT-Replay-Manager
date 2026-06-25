use crate::app::backend::tank_mapping::{map_tank_display, load_tank_mapping_from_file};
use crate::app::backend::cache;
use crate::app::model::{ReplayInfo, Settings};

use std::collections::HashSet;

use std::path::PathBuf;
use std::sync::{mpsc, Arc, Mutex};
use std::thread;

pub enum ScanMessage {
    Progress(String),
    Finished(Vec<ReplayInfo>),
    Status(String),
}

pub struct ScanController {
    tx: mpsc::Sender<ScanMessage>,
    worker_join: Arc<Mutex<Option<thread::JoinHandle<()>>>>,
}

impl ScanController {
    pub fn new(tx: mpsc::Sender<ScanMessage>) -> Self {
        Self {
            tx,
            worker_join: Arc::new(Mutex::new(None)),
        }
    }

    pub fn start_scan(
        &self,
        settings: Settings,
        db_path: PathBuf,
        incremental_known_paths: HashSet<String>,
        incremental: bool,
    ) {
        // stop previous job
        if let Ok(mut guard) = self.worker_join.lock() {
            if let Some(handle) = guard.take() {
                let _ = handle.join();
            }
        }

        let tx = self.tx.clone();
        let _worker_join = self.worker_join.clone();



        let handle = thread::spawn(move || {
            let result = run_scan(settings, db_path, incremental_known_paths, incremental, &tx);
            if let Err(e) = result {
                let _ = tx.send(ScanMessage::Status(format!("Scan failed: {e}")));
            }
        });

        if let Ok(mut guard) = self.worker_join.lock() {
            *guard = Some(handle);
        }
    }
}

fn run_scan(
    settings: Settings,
    db_path: PathBuf,
    incremental_known_paths: HashSet<String>,
    incremental: bool,
    tx: &mpsc::Sender<ScanMessage>,
) -> anyhow::Result<()> {
    if settings.replays_path.is_empty() {
        return Ok(());
    }

    // Expect tank_mapping.json next to the executable.
    let tank_map_path = "tank_mapping.json";

    let tank_map = load_tank_mapping_from_file(tank_map_path);

    let mut conn = cache::open_cache(&db_path)?;


    if incremental {
        let _ = tx.send(ScanMessage::Status("Starting incremental scan...".to_string()));
    } else {
        let _ = tx.send(ScanMessage::Status("Starting full scan...".to_string()));
    }

    let mut new_replays: Vec<ReplayInfo> = Vec::new();

    let dir = std::path::Path::new(&settings.replays_path);
    if !dir.exists() {
        return Ok(());
    }

    for entry in std::fs::read_dir(dir)? {
        let entry = entry?;
        let path = entry.path();
        if path.extension().and_then(|s| s.to_str()) != Some("wotreplay") {
            continue;
        }

        let path_str = path.to_string_lossy().to_string();

        if incremental && incremental_known_paths.contains(&path_str) {
            continue;
        }

        let file_name = path.file_name().and_then(|s| s.to_str()).unwrap_or("");
        let _ = tx.send(ScanMessage::Progress(file_name.to_string()));

        // Parse replay using the backend parser module.
        let mut info = match crate::app::backend::parser::parse_replay_file(&path_str) {
            Ok(v) => v,
            Err(_) => continue,
        };

        // Apply tank ID mapping (+ suffix logic already handled in mapping layer).
        info.tank = map_tank_display(&info.tank, &tank_map);

        // Skip failed/empty parses similar to Qt behavior
        if info.player_name.is_empty() {
            continue;
        }

        new_replays.push(info);

    }

    if !new_replays.is_empty() {
        cache::save_replay_cache(&mut conn, &new_replays)?;

    }

    // Sync deletions
    // On disk paths
    let mut disk_paths: HashSet<String> = HashSet::new();
    for entry in std::fs::read_dir(dir)? {
        let entry = entry?;
        let path = entry.path();
        if path.extension().and_then(|s| s.to_str()) != Some("wotreplay") {
            continue;
        }
        disk_paths.insert(path.to_string_lossy().to_string());
    }

    let (_existing_replays, cache_paths) = cache::load_replay_cache(&conn)?;
    let to_delete: Vec<String> = cache_paths
        .into_iter()
        .filter(|p| !disk_paths.contains(p))
        .collect();

    cache::delete_stale_entries(&conn, &to_delete)?;

    let (_replays_after, _) = cache::load_replay_cache(&conn)?;
    let _ = tx.send(ScanMessage::Finished(_replays_after));

    Ok(())
}

