![Rust](https://img.shields.io/badge/Rust-stable-orange)
![Platform](https://img.shields.io/badge/Platform-Windows-blue)
![License](https://img.shields.io/github/license/vorlie/WoT-Replay-Manager)

# WoT Replay Manager

A lightweight, native replay manager for **World of Tanks** built with Rust and `egui`.

WoT Replay Manager helps you browse, organize, and launch your `.wotreplay` files through a fast, simple desktop interface.

## Features

### Replay Integration

* Launch replays directly using your World of Tanks installation
* Automatically detects and stores your game path and preferences
* View replay metadata without opening the game

### Replay Management

* Scan and list all `.wotreplay` files from a selected directory
* Display detailed replay information:

  * Player name
  * Vehicle used
  * Map
  * Battle date
  * Damage dealt
  * Server and client version information

> **Note:** Damage dealt may display as `0` for incomplete or corrupted replays.

### Native Desktop Application

* Built entirely in Rust
* Fast startup and low resource usage
* Windows-first experience

---

## Disclaimer

**WoT Replay Manager is a fan-made project and is not affiliated with, endorsed by, or approved by Wargaming.net, World of Tanks, or any of their subsidiaries or partners.**

---

## Installation

### Windows

1. Download the latest release from the Releases page.
2. Extract `WoT-Replay-Manager-{version}-Win64.zip`.
3. Run `WoT-Replay-Manager.exe`.

No additional installation is required.

---

## Building from Source

### Prerequisites

* Rust toolchain (`stable`)
* MSVC Build Tools (when using the `x86_64-pc-windows-msvc` target)

### Build

```powershell
cargo build --release
```

The compiled executable will be available in:

```text
target\release\wot-replay-manager.exe
```

---

## Running

Ensure that `tank_mapping.json` is located next to the executable:

```text
wot-replay-manager.exe
tank_mapping.json
```

Then simply launch:

```powershell
.\wot-replay-manager.exe
```

---

## Supported Files

* `.wotreplay`

---

## Credits

* Built with `egui` and `eframe`
* Replay parsing powered by `wot_replay_parser`
* Created by the community, for the community
