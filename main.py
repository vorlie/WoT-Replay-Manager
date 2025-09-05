import sys
import subprocess
import os
from PyQt6.QtWidgets import (
    QApplication,
    QWidget,
    QVBoxLayout,
    QPushButton,
    QListWidget,
    QFileDialog,
    QAbstractItemView,
    QLabel,
    QMessageBox,
    QDialog,
    QHBoxLayout,
    QLineEdit,
    QComboBox,
)
from PyQt6.QtCore import QSettings, QCoreApplication
from PyQt6.QtGui import QIcon
from utils import get_replay_data
import xml.etree.ElementTree as ET


class SettingsDialog(QDialog):
    def __init__(self, parent=None, initial_directory=None):
        super().__init__(parent)
        self.setWindowTitle("Settings")
        self.setFixedSize(590, 150)
        self.settings = parent.settings
        self.initial_directory = initial_directory
        self.init_ui()

    def init_ui(self):
        main_layout = QVBoxLayout()
        
        bottle_layout = QHBoxLayout()
        bottle_label = QLabel("Bottles Bottle Name:")
        bottle_label.setToolTip("Name of the bottle where World of Tanks is installed (default: WindowsGames)")
        bottle_label.setFixedWidth(150)
        self.bottle_input = QLineEdit()
        self.bottle_input.setText(self.settings.value("bottle_name", "WindowsGames"))
        self.bottle_input.setMinimumWidth(300)
        bottle_layout.addWidget(bottle_label)
        bottle_layout.addWidget(self.bottle_input)
        bottle_layout.addStretch()
        main_layout.addLayout(bottle_layout)

        replays_layout = QHBoxLayout()
        replays_label = QLabel("Replays Folder Path:")
        replays_label.setToolTip("Path to the folder containing the replay files")
        replays_label.setFixedWidth(150)
        self.replays_input = QLineEdit()
        self.replays_input.setText(self.settings.value("replays_path", ""))
        self.replays_input.setReadOnly(True)
        self.replays_input.setMinimumWidth(300)
        replays_browse = QPushButton("Browse")
        replays_browse.clicked.connect(self.browse_replays_path)
        replays_layout.addWidget(replays_label)
        replays_layout.addWidget(self.replays_input)
        replays_layout.addWidget(replays_browse)
        replays_layout.addStretch()
        main_layout.addLayout(replays_layout)
        
        exec_layout = QHBoxLayout()
        exec_label = QLabel("WoT Executable Path:")
        exec_label.setToolTip("Path to the World of Tanks executable")
        exec_label.setFixedWidth(150)
        self.exec_input = QLineEdit()
        self.exec_input.setText(self.settings.value("executable_path", ""))
        self.exec_input.setReadOnly(True)
        self.exec_input.setMinimumWidth(300)
        exec_browse = QPushButton("Browse")
        exec_browse.clicked.connect(self.browse_exec_path)
        exec_layout.addWidget(exec_label)
        exec_layout.addWidget(self.exec_input)
        exec_layout.addWidget(exec_browse)
        exec_layout.addStretch()
        main_layout.addLayout(exec_layout)
        
        xml_layout = QHBoxLayout()
        xml_label = QLabel("Client Version XML Path:")
        xml_label.setToolTip("Path to the World of Tanks version.xml file")
        xml_label.setFixedWidth(150)
        self.xml_input = QLineEdit()
        self.xml_input.setText(self.settings.value("client_version_xml_path", ""))
        self.xml_input.setReadOnly(True)
        self.xml_input.setMinimumWidth(300)
        xml_browse = QPushButton("Browse")
        xml_browse.clicked.connect(self.browse_xml_path)
        xml_layout.addWidget(xml_label)
        xml_layout.addWidget(self.xml_input)
        xml_layout.addWidget(xml_browse)
        xml_layout.addStretch()
        main_layout.addLayout(xml_layout)

        button_layout = QHBoxLayout()
        self.ok_button = QPushButton("OK")
        self.ok_button.clicked.connect(self.save_settings_and_accept)
        self.cancel_button = QPushButton("Cancel")
        self.cancel_button.clicked.connect(self.reject)
        button_layout.addWidget(self.ok_button)
        button_layout.addWidget(self.cancel_button)
        button_layout.addStretch()
        main_layout.addLayout(button_layout)
        
        main_layout.addStretch()

        self.setLayout(main_layout)

    def browse_replays_path(self):
        path = QFileDialog.getExistingDirectory(self, "Select Replays Folder", self.initial_directory)
        if path:
            self.replays_input.setText(path)

    def browse_exec_path(self):
        path, _ = QFileDialog.getOpenFileName(self, "Select WorldOfTanks.exe", self.initial_directory, "Executable Files (*.exe)")
        if path:
            self.exec_input.setText(path)

    def browse_xml_path(self):
        path, _ = QFileDialog.getOpenFileName(self, "Select version.xml", self.initial_directory, "XML Files (*.xml)")
        if path:
            self.xml_input.setText(path)

    def save_settings_and_accept(self):
        self.settings.setValue("bottle_name", self.bottle_input.text())
        self.settings.setValue("replays_path", self.replays_input.text())
        self.settings.setValue("executable_path", self.exec_input.text())
        self.settings.setValue("client_version_xml_path", self.xml_input.text())
        self.accept()


class ReplayManager(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("WoT Replay Manager")
        self.setGeometry(100, 100, 1200, 600)
        self.setWindowIcon(QIcon("./icon.png"))
        self.replays_directory = ""
        self.client_version_xml_path = ""
        self.replays_data = []

        QCoreApplication.setApplicationName("WoT Replay Manager")
        self.settings = QSettings()
        config_dir = os.path.join(
            os.path.expanduser("~"), ".config", "wotreplay_picker"
        )
        config_path = os.path.join(config_dir, "config.ini")

        if not os.path.exists(config_dir):
            os.makedirs(config_dir)

        self.settings = QSettings(config_path, QSettings.Format.IniFormat)

        self.init_ui()
        self.load_initial_settings()

    def init_ui(self):
        layout = QVBoxLayout()

        top_layout = QHBoxLayout()

        settings_button = QPushButton("Settings")
        settings_button.clicked.connect(self.open_settings)
        top_layout.addWidget(settings_button)

        sort_label = QLabel("Sort by:")
        self.sort_combo = QComboBox()
        self.sort_combo.addItem("Date (Newest First)")
        self.sort_combo.addItem("Date (Oldest First)")
        self.sort_combo.addItem("Player Name")
        self.sort_combo.addItem("Tank")
        self.sort_combo.addItem("Damage Dealt (Highest First)")
        self.sort_combo.addItem("Map")
        self.sort_combo.addItem("Info")
        top_layout.addWidget(sort_label)
        top_layout.addWidget(self.sort_combo)

        cleanup_button = QPushButton("Cleanup Old Replays")
        cleanup_button.setToolTip("Deletes old replays that are not compatible with the current game client version.")
        cleanup_button.clicked.connect(self.cleanup_replays)
        top_layout.addWidget(cleanup_button)

        layout.addLayout(top_layout)

        self.sort_combo.currentTextChanged.connect(self.sort_replays)

        self.replay_list = QListWidget()
        self.replay_list.setSelectionMode(
            QAbstractItemView.SelectionMode.SingleSelection
        )
        layout.addWidget(self.replay_list)

        launch_button = QPushButton("Launch Replay")
        launch_button.clicked.connect(self.launch_replay)
        layout.addWidget(launch_button)

        self.setLayout(layout)

        self.wot_executable_path = self.settings.value("executable_path", "")

    def get_current_client_version(self):
        """
        Parses the World of Tanks version.xml file to get the current client version.
        Returns a tuple of version numbers for easy comparison.
        """
        try:
            tree = ET.parse(self.client_version_xml_path)
            root = tree.getroot()
            version_str = root.find("version").text.strip()
            # The format is "v.X.Y.Z.A #B"
            # "v.2.0.0.0 #731" -> "2.0.0.0"
            version_tuple = tuple(map(int, version_str.split(" ")[0][2:].split(".")))
            print(f"Current client version: {version_tuple}")
            return version_tuple
        except Exception as e:
            print(f"Error reading client version.xml: {e}")
            return None

    def cleanup_replays(self):
        """
        Deletes old replays that are not compatible with the current game client version.
        """
        current_version = self.get_current_client_version()
        if not current_version:
            QMessageBox.warning(
                self, "Error", "Could not determine the current client version."
            )
            return

        old_replays = []
        for replay_info in self.replays_data:
            replay_version_str = replay_info.get("version", "")
            print(
                f"Player: {replay_info['player_name']} | "
                f"Tank: {replay_info['tank']} | "
                f"Map: {replay_info['map']} | "
                f"Date: {replay_info['date']} | "
                f"Damage: {replay_info['damage_dealt']} | "
                f"Server: {replay_info['server_name']} | "
                f"Version: {replay_info['version']} | "
                f"Complete: {replay_info['is_complete']}"
            )
            if not replay_version_str:
                continue

            try:
                version_parts = replay_version_str.split(".")
                replay_version = tuple(map(int, version_parts))
            except (ValueError, IndexError):
                print(
                    f"Skipping replay with invalid version string: {replay_version_str}"
                )
                continue

            if replay_version and replay_version < current_version:
                old_replays.append(replay_info["path"])

        if not old_replays:
            QMessageBox.information(
                self, "Cleanup Complete", "No old replays found to delete."
            )
            return

        confirm_box = QMessageBox()
        confirm_box.setWindowTitle("Confirm Deletion")
        confirm_box.setText(
            f"Found {len(old_replays)} old replays. Are you sure you want to delete them?"
        )
        confirm_box.setStandardButtons(
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No
        )
        confirm_box.setDefaultButton(QMessageBox.StandardButton.No)

        if confirm_box.exec() == QMessageBox.StandardButton.Yes:
            deleted_count = 0
            for file_path in old_replays:
                try:
                    os.remove(file_path)
                    deleted_count += 1
                except Exception as e:
                    print(f"Failed to delete {file_path}: {e}")

            QMessageBox.information(
                self,
                "Cleanup Complete",
                f"Successfully deleted {deleted_count} old replays.",
            )
            self.load_replays(reload_data=True)

    def load_initial_settings(self):
        self.bottle_name = self.settings.value("bottle_name", "WindowsGames")
        self.wot_executable_path = self.settings.value("executable_path", "")
        self.replays_directory = self.settings.value("replays_path", "")
        self.client_version_xml_path = self.settings.value(
            "client_version_xml_path", ""
        )

        if not self.wot_executable_path or not self.replays_directory or not self.client_version_xml_path:
            QMessageBox.information(self, "Setup Required", "Please configure the application settings before use.")
            self.open_settings()

            self.wot_executable_path = self.settings.value("executable_path", "")
            self.replays_directory = self.settings.value("replays_path", "")
            self.client_version_xml_path = self.settings.value(
                "client_version_xml_path", ""
            )

            if not self.wot_executable_path or not self.replays_directory or not self.client_version_xml_path:
                QMessageBox.critical(self, "Error", "Application cannot run without all required paths being set.")
                self.close()

        if self.replays_directory:
            self.load_replays()

    def load_replays(self, reload_data=True):
        self.replay_list.clear()

        if reload_data:
            self.replays_data = get_replay_data(self.replays_directory)
        
        for replay in self.replays_data:
            display_text = (
                f"Player: {replay['player_name']} | "
                f"Tank: {replay['tank']} | "
                f"Map: {replay['map']} | "
                f"Date: {replay['date']} | "
                f"Damage: {replay['damage_dealt']} | "
                f"Server: {replay['server_name']} | "
                f"Version: {replay['version']} | "
                f"Complete: {replay['is_complete']}"
            )
            self.replay_list.addItem(display_text)

    def launch_replay(self):
        selected_items = self.replay_list.selectedItems()
        if not selected_items:
            QMessageBox.information(self, "Info", "No replay selected.")
            return

        selected_index = self.replay_list.row(selected_items[0])
        selected_replay = self.replays_data[selected_index]
        replay_path = selected_replay["path"]

        # Check the operating system and use the appropriate command
        if sys.platform.startswith('win'):
            # For Windows, launch the executable with the replay path as an argument
            command = [self.wot_executable_path, replay_path]
            try:
                subprocess.Popen(command)
                QMessageBox.information(
                    self, "Launching", f"Launching replay: {replay_path}"
                )
            except Exception as e:
                QMessageBox.critical(self, "Error", f"An error occurred while launching on Windows: {e}")
        elif sys.platform.startswith('linux'):
            # For Linux, use bottles-cli to run the executable
            command = [
                "bottles-cli",
                "run",
                "-b",
                self.bottle_name,
                "-e",
                self.wot_executable_path,
                "--args",
                replay_path,
            ]
            try:
                subprocess.Popen(command)
                QMessageBox.information(
                    self, "Launching", f"Launching replay with bottles-cli: {replay_path}"
                )
            except FileNotFoundError:
                QMessageBox.critical(
                    self, "Error", "bottles-cli not found. Make sure it's in your PATH."
                )
            except Exception as e:
                QMessageBox.critical(self, "Error", f"An error occurred while launching on Linux: {e}")
        else:
            QMessageBox.critical(self, "Error", f"Unsupported operating system: {sys.platform}")

    def open_settings(self):
        initial_directory = ""

        if self.wot_executable_path and os.path.exists(os.path.dirname(self.wot_executable_path)):
            initial_directory = os.path.dirname(self.wot_executable_path)
        elif self.replays_directory and os.path.exists(self.replays_directory):
            initial_directory = self.replays_directory
        else:
            initial_directory = os.path.expanduser('~')

        dialog = SettingsDialog(self, initial_directory)
        dialog.exec()
        self.wot_executable_path = self.settings.value("executable_path", "")

    def sort_replays(self):
        if not self.replays_data:
            return

        sort_option = self.sort_combo.currentText()

        def get_sort_key(item):
            if sort_option == "Date (Newest First)" or sort_option == "Date (Oldest First)":
                return item['datetime_obj']
            elif sort_option == "Player Name":
                return item['player_name'].lower()
            elif sort_option == "Tank":
                return item['tank'].lower()
            elif sort_option == "Damage Dealt (Highest First)":
                return item['damage_dealt']
            elif sort_option == "Map":
                return item['map'].lower()
            elif sort_option == "Info":
                return item['tank_and_map_info'].lower()
            return item['datetime_obj']

        reverse_order = sort_option in ("Date (Newest First)", "Damage Dealt (Highest First)")
        self.replays_data.sort(key=get_sort_key, reverse=reverse_order)
        self.load_replays(reload_data=False)


if __name__ == "__main__":
    app = QApplication(sys.argv)
    manager = ReplayManager()
    manager.show()
    sys.exit(app.exec())
