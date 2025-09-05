# WoT Replay Manager

The WoT Replay Manager is a desktop application built with Python and PyQt6 that helps you manage and launch your World of Tanks replays. It provides a user-friendly interface to sort, view, and launch replay files, as well as clean up old replays that are no longer compatible with the current game version.
***Mainly made for Linux users.***

- **Cross-Platform Compatibility**: Supports both Windows and Linux operating systems.
- **Replay Listing**: Automatically detects and lists .wotreplay files from your specified directory.
- **Detailed Information**: Displays key replay metadata, including player name, tank, map, date, damage dealt, and game version.
- **Sorting**: Sort your replay list by date, player name, tank, map, or damage.
- **Automatic Cleanup**: Identifies and allows you to delete old replays that are incompatible with your current client version, saving you disk space.
- **Launch Replays**: Launches selected replays using your bottles-cli configuration.
- **Persistent Settings**: Saves your specified paths (executable, replays folder, etc.) so you only need to configure them once.

## Prerequisites
To run this application, you will need:

- **Python 3.x**
- **PyQt6**: `pip install PyQt6`
- **wotreplay**: `pip install wotreplay`
- **bottles-cli**: Required for launching replays. This tool is typically used on Linux systems for managing Windows applications.
    - **Note**: This application is not designed to work with Flatpak Bottles.
    - **Bottles**: https://usebottles.com/

## How to Use
1. **First-Time Setup**

Upon first launch, the application will prompt you to configure the necessary settings. You will need to provide the following paths:
- **Bottles Bottle Name** (Linux only): The name of the Bottles container where World of Tanks is installed (e.g., `WindowsGames`). This setting will be ignored on Windows.
- **WoT Executable Path**: The full path to your `WorldOfTanks.exe` file.
- **Replays Folder Path**: The folder where your `.wotreplay` files are saved.
- **Client Version XML Path**: The path to the `version.xml` file in your World of Tanks game directory (used for replay cleanup).

The file dialogs will automatically open in the last known game directory to make it easier to find the correct files.

2. **Managing Replays**
- The main window will display a list of all detected replays.
- Use the "**Sort by**" dropdown menu to change the order of the replay list.
- Select a replay from the list and click the "**Launch Replay**" button to start the replay in World of Tanks.

3. **Cleaning Up Old Replays**
- Click the "**Cleanup Old Replays**" button to find and delete replays that are no longer compatible with your current game version.
- A confirmation dialog will appear, showing you how many replays are about to be deleted.
- This feature helps you free up disk space by removing outdated files.

## Code Structure
- `main.py`: The main application file containing the `ReplayManager` and `SettingsDialog` classes. It handles the UI, user interactions, and core application logic.
- `utils/__init__.py`: Contains helper functions, such as `get_replay_data`, which is responsible for parsing the replay files and extracting metadata.