# WoT Replay Manager

WoT Replay Manager is a modern desktop application built with **C++/Qt6** and a **Rust replay parser** for managing World of Tanks replay files. It provides an intuitive interface for organizing and launching your replays.

## Features

* **Cross-Platform**: Native support for both Windows and Linux
* **Smart Replay Management**: 
  * Lists all `.wotreplay` files from your chosen directory
  * Shows detailed information: player name, tank, map, date, damage dealt
  * Displays server and client version compatibility
* **Powerful Organization**:
  * Sort replays by date, player name, tank, map, or damage
  * Filter and search functionality
* **Seamless Integration**:
  * Launch replays directly using your WoT installation
  * Persistent settings for paths and preferences

---

## Disclaimer

**WoT Replay Manager is a fan-made tool and is not affiliated with, endorsed, or approved by Wargaming.net or any other associated entity.**

---

## Prerequisites

### Linux

1. **Qt6 libraries** – Ensure Qt6 is installed. For a fully standalone binary, you can build static Qt6.
2. **Rust toolchain** – Required to build the parser library if you want to rebuild from source. Install via:

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```

### Windows

1. **Qt6** (MSVC or MinGW, matching the compiler used for the project).
2. **Rust toolchain** – Required to build the parser library if you want to rebuild from source.

---

## Release Structure

### Windows Release Structure
```
WoT-Replay-Manager/
├── WoT-Replay-Manager.exe
├── wot_parser_lib.dll
├── Qt DLLs (Qt6Core.dll, Qt6Gui.dll, etc.)
├── Support DLLs (libgcc, libstdc++, etc.)
├── generic/
│   └── qtuiotouchplugin.dll
├── iconengines/
│   └── qsvgicon.dll
├── imageformats/
│   └── qgif.dll, qico.dll, qjpeg.dll, qsvg.dll
├── networkinformation/
├── platforms/
│   └── qwindows.dll
├── styles/
├── tls/
└── translations/
```

### Linux Release Structure
```
WoT-Replay-Manager/
├── bin/
│   └── WoT-Replay-Manager
├── lib/
│   ├── libwot_parser_lib.so
│   ├── Qt libraries
│   └── System libraries
├── plugins/
│   └── platforms/
│       ├── libqwayland-egl.so
│       ├── libqwayland-generic.so
│       └── libqxcb.so
├── icon.png
├── run.sh
├── wotrm.desktop
└── LICENSE
```

> ⚠️ **Important Notes:**
> * On Windows, all DLLs including `wot_parser_lib.dll` must be in the root directory with the executable
> * On Linux, use the provided `run.sh` script which sets up proper library paths

---

## Building the Application

### Linux

1. Install Qt Creator and select a Qt6 kit during installation
2. Open Qt Creator
3. Click **File → Open File or Project** and select `CMakeLists.txt` in the project root
4. Configure the kit (choose your Qt6 version and compiler)
5. Click **Build**
6. The binary will be in the build directory created by Qt Creator

The Rust library needs to be built separately (see section below).

---

### Windows

1. Open the project in **Qt Creator**, configure the kit, and build.
2. Use `windeployqt` to bundle required Qt DLLs:

```powershell
C:\Qt\6.x.x\mingw_64\bin\windeployqt.exe path\to\WoT-Replay-Manager.exe
```

3. Copy the compiled Rust library `wot_parser_lib.dll` **next to the executable**.

4. The final release folder should look like:

```
WoT-Replay-Manager/
├── WoT-Replay-Manager.exe
├── wot_parser_lib.dll
├── Qt6Core.dll
├── Qt6Gui.dll
├── Qt6Widgets.dll
├── libgcc_s_seh-1.dll
├── libstdc++-6.dll
├── libwinpthread-1.dll
├── platforms/
├── imageformats/
├── iconengines/
├── translations/
└── ...
```

> The app is now fully standalone on Windows. The DLL will be correctly found by the executable at runtime.

---

## Building the Rust Parser Library

The Rust parser library is used via FFI by the C++/Qt application.

### Linux

```bash
cd wot_parser_lib
cargo build --release
cp target/release/libwot_parser_lib.so ../WoT-Replay-Manager/lib/
```

### Windows

```powershell
cd wot_parser_lib
cargo build --release
copy target\release\wot_parser_lib.dll ..\WoT-Replay-Manager\
```

> After this, the C++ app can call the parser functions directly via FFI.

---

## Running the Application

* **Linux**: Ensure `libwot_parser_lib.so` is present in `lib/` and run via `run.sh`, which sets `LD_LIBRARY_PATH` and `QT_PLUGIN_PATH`.
* **Windows**: Ensure `wot_parser_lib.dll` is present **next to the executable** and run `WoT-Replay-Manager.exe`.

---

## Notes

* All replay parsing is done inside the Rust library.
* The C++/Qt app only handles UI, settings, and launching the game.
* Keep the Rust library alongside the executable to avoid runtime errors.
* The application supports `.wotreplay` files only.
