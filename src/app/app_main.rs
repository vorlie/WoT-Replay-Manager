use crate::app::backend::{cache, launcher, scanner, settings_store};

use crate::app::model::{ReplayInfo, Settings};

use eframe::egui;
use std::collections::HashSet;
use std::path::PathBuf;
use std::sync::mpsc;

pub struct WotApp {

    pub settings: Settings,
    pub replays: Vec<ReplayInfo>,
    pub status: String,
    pub selected_row: Option<usize>,

    db_path: Option<PathBuf>,
    scan_rx: Option<mpsc::Receiver<scanner::ScanMessage>>,
    scan_ctrl: Option<scanner::ScanController>,

    show_settings: bool,
    settings_work: Settings,
}

impl Default for WotApp {
    fn default() -> Self {
        let settings_work = Settings::default();
        Self {
            settings: settings_store::load_settings(),
            replays: Vec::new(),
            status: String::new(),
            selected_row: None,
            db_path: None,
            scan_rx: None,
            scan_ctrl: None,
            show_settings: false,
            settings_work,
        }
    }
}


impl WotApp {
    fn ensure_db(&mut self) {
        if self.db_path.is_some() {
            return;
        }

        if let Some(dir) = directories::ProjectDirs::from("com", "wot", "wot-replay-manager")
            .map(|p| p.config_dir().to_path_buf())
        {
            let _ = std::fs::create_dir_all(&dir);
            self.db_path = Some(dir.join("replays_cache.sqlite"));
        }
    }

    fn load_cache_to_ui(&mut self) {
        self.ensure_db();
        let Some(db_path) = self.db_path.clone() else { return; };
        if !db_path.exists() {
            return;
        }

        if let Ok(conn) = cache::open_cache(&db_path) {
            if let Ok((replays, _paths)) = cache::load_replay_cache(&conn) {
                self.replays = replays;
            }
        }
    }

    fn start_scan_incremental(&mut self) {
        self.ensure_db();
        let Some(db_path) = self.db_path.clone() else { return; };

        let (tx, rx) = mpsc::channel();
        self.scan_rx = Some(rx);
        let ctrl = scanner::ScanController::new(tx);
        self.scan_ctrl = Some(ctrl);


        let incremental_known: HashSet<String> = if let Ok(conn) = cache::open_cache(&db_path) {
            cache::load_replay_cache(&conn)
                .map(|(_replays, paths)| paths.into_iter().collect())
                .unwrap_or_default()
        } else {
            HashSet::new()
        };

        let incremental = true;
        let ctrl_ref = self.scan_ctrl.as_ref().unwrap();
        ctrl_ref.start_scan(
            self.settings.clone(),
            db_path,
            incremental_known,
            incremental,
        );
    }

    fn start_cleanup(&mut self) {
        self.start_scan_incremental();
    }

    fn launch_selected(&mut self) {
        let row = match self.selected_row {
            Some(r) => r,
            None => return,
        };
        let info = match self.replays.get(row) {
            Some(i) => i,
            None => return,
        };
        if let Err(e) = launcher::launch_replay(&self.settings, &info.path) {
            self.status = e;
        }
    }
}

impl eframe::App for WotApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {

        // Poll scan messages
        if let Some(rx) = &self.scan_rx {
            for msg in rx.try_iter() {
                match msg {
                    scanner::ScanMessage::Progress(p) => self.status = format!("Scanning: {p}"),
                    scanner::ScanMessage::Status(s) => self.status = s,
                    scanner::ScanMessage::Finished(replays) => {
                        self.replays = replays;
                        self.selected_row = None;
                        self.status = "Scan complete".to_string();
                    }
                }
            }
        }

        egui::TopBottomPanel::top("top_panel").show(ctx, |ui| {
            ui.horizontal(|ui| {
                ui.heading("WoT Replay Manager");
                ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                    ui.colored_label(egui::Color32::LIGHT_BLUE, &self.status);
                });
            });
        });


        egui::CentralPanel::default().show(ctx, |ui| {
            ui.add_space(6.0);

            ui.horizontal(|ui| {
                if ui.button("Settings").clicked() {
                    self.settings_work = self.settings.clone();
                    self.show_settings = true;
                }

                if ui.button("Cleanup Old Replays").clicked() {
                    self.start_cleanup();
                }

                ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                    let enabled = self.selected_row.is_some();
                    let text = "Launch Replay";
                    if ui.add_enabled(enabled, egui::Button::new(text)).clicked() {
                        self.launch_selected();
                    }
                });
            });

            ui.separator();

            if ui.button("Load cache now").clicked() {
                self.load_cache_to_ui();
                self.start_scan_incremental();
            }

            ui.add_space(8.0);
            let selected = self.selected_row;

            egui::ScrollArea::both()
                .auto_shrink([false; 2])
                .show(ui, |ui| {
                    egui::Grid::new("replays_grid")
                        .striped(true)
                        .min_col_width(80.0)
                        .spacing([12.0, 6.0])
                        .show(ui, |ui| {
                            ui.label("...");
                            ui.label("Player");
                            ui.label("Tank");
                            ui.label("Map");
                            ui.label("Date");
                            ui.label("Damage");
                            ui.label("Server");
                            ui.label("Version");
                            ui.end_row();

                            for (idx, info) in self.replays.iter().enumerate() {
                                let is_selected = selected == Some(idx);
                                let row_bg = if is_selected {
                                    egui::Color32::from_rgb(40, 70, 120)
                                } else {
                                    egui::Color32::TRANSPARENT
                                };

                                ui.separator();

                                let selectable = ui.add(egui::SelectableLabel::new(
                                    is_selected,
                                    info.player_name.clone(),
                                ));
                                if selectable.clicked() {
                                    self.selected_row = Some(idx);
                                }

                                ui.label(&info.tank);
                                ui.label(&info.map);
                                ui.label(&info.date);
                                ui.label(info.damage.to_string());
                                ui.label(&info.server);
                                ui.label(&info.version);
                                ui.end_row();

                                let _ = row_bg;
                            }
                        });
                });
        });




        let show_settings = self.show_settings;
        if show_settings {
            let mut show_settings_local = show_settings;
            egui::Window::new("Settings")
                .open(&mut show_settings_local)
                .collapsible(false)
                .resizable(false)
                .show(ctx, |ui| {

                    ui.horizontal(|ui| {
                        ui.label("WoT Executable Path");
                    });
                    ui.text_edit_singleline(&mut self.settings_work.executable_path);

                    ui.separator();

                    ui.horizontal(|ui| {
                        ui.label("WoT Replays Path");
                    });
                    ui.text_edit_singleline(&mut self.settings_work.replays_path);

                    ui.separator();

                    ui.horizontal(|ui| {
                        ui.label("WoT version.xml Path");
                    });
                    ui.text_edit_singleline(&mut self.settings_work.client_version_xml_path);

                    ui.separator();

                    ui.horizontal(|ui| {
                        ui.label("Bottle Name (Linux)");
                    });
                    ui.text_edit_singleline(&mut self.settings_work.bottle_name);

                    ui.separator();

                    ui.horizontal(|ui| {
                        if ui.button("Save & Rescan").clicked() {
                            self.settings = self.settings_work.clone();
                            settings_store::save_settings(&self.settings);
                            self.status = "Settings saved".into();
                            self.load_cache_to_ui();
                            self.start_scan_incremental();
                            self.show_settings = false;
                        }
                        if ui.button("Cancel").clicked() {
                            self.show_settings = false;
                        }

                    });
                });
            
        }
    }
}


