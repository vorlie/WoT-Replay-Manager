# WoT Replay Manager

WoT Replay Manager is a desktop application built with **C++/Qt6** and a **Python parser**. It helps you manage and launch your World of Tanks replays with a modern, user-friendly interface. The app automatically parses replay files, displays detailed metadata, and allows cleanup of outdated replays.

* **Cross-Platform**: Works on both Windows and Linux.
* **Replay Listing**: Detects and lists `.wotreplay` files from your chosen directory.
* **Detailed Information**: Shows player name, tank, map, date, damage, and client version.
* **Sorting**: Sort replays by date, player name, tank, map, or damage.
* ~~**Automatic Cleanup**: Delete replays incompatible with your current client version.~~ (In development)
* **Launch Replays**: Launch selected replays using your game installation.
* **Persistent Settings**: Saves paths (WoT executable, replays folder, etc.) for future sessions.

## Disclaimer

**WoT Replay Manager is a fan-made tool and is not affiliated with, endorsed, or approved by Wargaming.net or any other associated entity.**

---

## Prerequisites

### Linux

1. **Qt6 libraries**
   Ensure Qt6 is installed. If you want a fully standalone binary, you can build static Qt6.

2. **Python 3.12**
   Required to compile the parser using Nuitka.

3. **Nuitka (for parser compilation)**

```bash
python3.12 -m pip install --upgrade pip
python3.12 -m pip install nuitka
```

---

### Windows

1. **Qt6** (MSVC or MinGW, matching the compiler used for the project).
2. **Python 3.12** (for building the parser).
3. **Nuitka** (for building the parser executable).

---

## Folder Layout

For both Linux and Windows, the folder structure should look like this:

```
WoT-Replay-Manager/
├── bin/
│   └── WoT-Replay-Manager (or .exe on Windows)
├── parser/
│   ├── replay_parser.bin (or replay_parser.exe)
│   └── all Python runtime dependencies
├── lib/ (Qt shared libs for Linux)
├── plugins/ (Qt plugins for Linux)
├── run.sh (Linux launcher)
├── icon.png
├── LICENSE
├── README.md
└── ...
```

> ⚠️ The `parser` folder must contain the compiled parser executable and all its dependencies. The main app relies on `./parser/replay_parser.bin` (Linux) or `./parser/replay_parser.exe` (Windows).

---

## Building the Application

### Linux

You have two options to build the project:

**1. Using the Terminal (CMake + Make):**

```bash
git clone <repo-url>
cd WoT-Replay-Manager
mkdir build && cd build
cmake ..
make -j$(nproc)
```

The compiled binary will be located in `build/WoT-Replay-Manager`.

**2. Using Qt Creator (recommended):**

1. Open **Qt Creator**.
2. Click **File → Open File or Project** and select `CMakeLists.txt` in the project root.
3. Configure the kit (choose your Qt6 version and compiler).
4. Click the **Build** button to compile the project.
5. The resulting binary will appear in the build folder defined by Qt Creator (usually inside `build-...`).

> Using Qt Creator ensures proper Qt setup, environment variables, and integration with plugins, which can prevent common runtime issues.

---

### Windows

1. Copy the source files to your Windows machine.
2. Open the project in **Qt Creator**, configure, and build.
3. Use `windeployqt` to bundle the required Qt DLLs:

```powershell
C:\Qt\6.x.x\mingw_64\bin\windeployqt.exe path\to\WoT-Replay-Manager.exe
```

4. Make sure the `parser` folder is present alongside the executable.

---

## Building the Python Parser (`replay_parser`)

### Linux

```bash
cd parser
python3.12 -m venv .venv
source .venv/bin/activate
pip install --upgrade pip nuitka

python3.12 -m nuitka --standalone replay_parser.py
cp -r replay_parser.dist/* ../parser/
```

### Windows

```powershell
cd parser
python -m nuitka --standalone replay_parser.py
# Copy all generated files to parser folder
```

> Note: Antivirus software may flag the compiled parser. Users can build it themselves to avoid this.

---

## Running the Application

* **Linux**: Use `run.sh` to launch. It sets `LD_LIBRARY_PATH`, `QT_PLUGIN_PATH`, and platform plugin (`QT_QPA_PLATFORM=xcb`).
* **Windows**: Run `WoT-Replay-Manager.exe`. Ensure the parser folder is present alongside the executable.
