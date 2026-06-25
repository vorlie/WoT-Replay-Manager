use crate::app::model::Settings;

pub fn launch_replay(settings: &Settings, replay_path: &str) -> Result<(), String> {
    if cfg!(target_os = "windows") {
        if settings.executable_path.is_empty() {
            return Err("WoT executable path not set".into());
        }
        let status = std::process::Command::new(&settings.executable_path)
            .arg(replay_path)
            .status()
            .map_err(|e| e.to_string())?;
        if status.success() {
            Ok(())
        } else {
            Err(format!("WoT returned non-zero exit code"))
        }
    } else {
        if settings.executable_path.is_empty() || settings.bottle_name.is_empty() {
            return Err("WoT executable path or bottle name not set".into());
        }

        // bottles-cli is expected to exist in PATH.
        // Matches Qt logic:
        // env -u LD_LIBRARY_PATH bottles-cli run -b <bottle> -e <wot_exec> --args <replay>
        let cmd = format!(
            "env -u LD_LIBRARY_PATH bottles-cli run -b {} -e {} --args {}",
            settings.bottle_name,
            settings.executable_path,
            replay_path
        );

        let status = std::process::Command::new("/bin/sh")
            .arg("-c")
            .arg(cmd)
            .status()
            .map_err(|e| e.to_string())?;

        if status.success() {
            Ok(())
        } else {
            Err("Failed to launch replay".into())
        }
    }
}

