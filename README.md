# WoT Replay Manager

WoT Replay Manager is a multi-platform companion for organising and viewing your replays alongside your World of Tanks install. Built with C++, Rust and QT6, manage your replays with ease using an intuitive, snappy interface!


## Features

* **Seamless Integration**:
  * Launch replays directly using your WoT installation
  * Persistent settings for paths and preferences
* **Smart Replay Management**: 
  * Lists all `.wotreplay` files from your chosen directory
  * Shows detailed information: player name, tank, map, date, damage dealt
    * Note: Damage dealt shows 0 for incomplete replays
  * Displays server and client version compatibility
* **Powerful Organization**:
  * Sort replays by date, player name, tank, map, or damage
  * Filter and search functionality
* **Cross-Platform**: Native support for both Windows and Linux
---

## Disclaimer

**WoT Replay Manager is a fan-made tool and is not affiliated with, endorsed, or approved by Wargaming.net or any other associated entity.**

---

## Installation

### Windows
1. Download the latest `WoT-Replay-Manager-{version}-Win64.zip` from the [Releases page](https://github.com/vorlie/WoT-Replay-Manager/releases/latest)
2. Extract the ZIP file to your preferred location
3. Run `WoT-Replay-Manager.exe`

### Linux
1. Download the latest `WoT-Replay-Manager-{version}-linux-x86_64.tar.gz` from the [Releases page](https://github.com/vorlie/WoT-Replay-Manager/releases/latest)
2. Extract the archive:
```bash
tar xf WoT-Replay-Manager-*-linux-x86_64.tar.gz
```
3. Make the run script executable:
```bash
chmod +x WoT-Replay-Manager/run.sh
```
4. Run the application:
```bash
./WoT-Replay-Manager/run.sh
```

> **Note**: All required dependencies are bundled with both versions. No additional installation steps needed.

### ⚠️ Linux Deployment Caveat

The bundled Linux executable was compiled on **Arch Linux** against a specific version of **Qt 6.x**. While the necessary Qt libraries are included in the package, system dependencies (like specific C/C++ runtime versions) can sometimes conflict.
- If you are running a modern distribution with Qt 6.x already installed (e.g., a recent KDE Plasma environment), the binary should work seamlessly.
- If you encounter errors related to missing or incompatible .so files, please follow the Building From Source instructions below.

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
├── bin
│   └── WoT-Replay-Manager*
├── icon.png
├── lib
│   ├── libb2.so.1*
│   ├── libbrotlicommon.so.1*
│   ├── libbrotlidec.so.1*
│   ├── libbz2.so.1.0*
│   ├── libc.so.6*
│   ├── libcap.so.2*
│   ├── libdbus-1.so.3*
│   ├── libdouble-conversion.so.3*
│   ├── libEGL.so.1*
│   ├── libexpat.so.1*
│   ├── libffi.so.8*
│   ├── libfontconfig.so.1*
│   ├── libfreetype.so.6*
│   ├── libgcc_s.so.1
│   ├── libGLdispatch.so.0*
│   ├── libglib-2.0.so.0*
│   ├── libGLX.so.0*
│   ├── libgomp.so.1*
│   ├── libgraphite2.so.3*
│   ├── libharfbuzz.so.0*
│   ├── libICE.so.6*
│   ├── libicudata.so.76*
│   ├── libicui18n.so.76*
│   ├── libicuuc.so.76*
│   ├── libm.so.6*
│   ├── libmd4c.so.0*
│   ├── libOpenGL.so.0*
│   ├── libpcre2-16.so.0*
│   ├── libpcre2-8.so.0*
│   ├── libpng16.so.16*
│   ├── libQt6Core.so.6*
│   ├── libQt6DBus.so.6*
│   ├── libQt6Gui.so.6*
│   ├── libQt6Sql.so.6*
│   ├── libQt6WaylandClient.so.6*
│   ├── libQt6Widgets.so.6*
│   ├── libQt6XcbQpa.so.6*
│   ├── libSM.so.6*
│   ├── libstdc++.so.6*
│   ├── libsystemd.so.0*
│   ├── libuuid.so.1*
│   ├── libwayland-client.so.0*
│   ├── libwayland-cursor.so.0*
│   ├── libwot_parser_lib.so*
│   ├── libX11.so.6*
│   ├── libX11-xcb.so.1*
│   ├── libXau.so.6*
│   ├── libxcb.so.1*
│   ├── libxcb-cursor.so.0*
│   ├── libxcb-icccm.so.4*
│   ├── libxcb-image.so.0*
│   ├── libxcb-keysyms.so.1*
│   ├── libxcb-randr.so.0*
│   ├── libxcb-render.so.0*
│   ├── libxcb-render-util.so.0*
│   ├── libxcb-shape.so.0*
│   ├── libxcb-shm.so.0*
│   ├── libxcb-sync.so.1*
│   ├── libxcb-util.so.1*
│   ├── libxcb-xfixes.so.0*
│   ├── libxcb-xinput.so.0*
│   ├── libxcb-xkb.so.1*
│   ├── libXdmcp.so.6*
│   ├── libxkbcommon.so.0*
│   ├── libxkbcommon-x11.so.0*
│   ├── libz.so.1*
│   └── libzstd.so.1*
├── LICENSE
├── plugins
│   ├── platforms
│   │   ├── libqwayland-egl.so*
│   │   ├── libqwayland-generic.so*
│   │   └── libqxcb.so*
│   └── sqldrivers
│       └── libqsqlite.so*
└── run.sh*
```

> ⚠️ **Important Notes:**
> * On Windows, all DLLs including `wot_parser_lib.dll` must be in the root directory with the executable
> * On Linux, use the provided `run.sh` script which sets up proper library paths