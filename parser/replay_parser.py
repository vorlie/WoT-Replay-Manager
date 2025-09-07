#!/usr/bin/env python3
import os
import json
import sys
from wotreplay import ReplayData

def get_replay_data(replay_dir):
    """
    Parses replay files in a given directory and extracts key metadata.
    Handles incomplete replays gracefully.
    Returns a list of dicts with replay info.
    """
    replays = []

    if not os.path.exists(replay_dir):
        print(f"Error: Replay directory not found at '{replay_dir}'", file=sys.stderr)
        return replays

    for file_name in os.listdir(replay_dir):
        if not file_name.endswith(".wotreplay"):
            continue

        file_path = os.path.join(replay_dir, file_name)
        try:
            replay = ReplayData(file_path=file_path)
            metadata = replay.battle_metadata[0]

            # Check if replay is complete
            is_complete = metadata.get("battle_type") == 1
            damage_dealt = 0

            if is_complete:
                try:
                    battle_result = replay.battle_performance[0]
                    damage_dealt = battle_result.get("damage_dealt", 0)
                except Exception:
                    damage_dealt = 0

            replay_datetime_obj = metadata.get("replay_date")
            replay_date_str = replay_datetime_obj.strftime("%Y-%m-%d %H:%M") if replay_datetime_obj else ""

            player_vehicle = metadata.get("player_vehicle", "")
            tank_tag = metadata.get("tank_tag", "")
            map_display_name = metadata.get("map_display_name", "")
            player_name = metadata.get("player_name", "")
            server_name = metadata.get("server_name", "")
            version = metadata.get("version", "")
            tank_and_map_info = f"{player_vehicle} on {map_display_name}" if player_vehicle and map_display_name else ""

            replays.append({
                "path": file_path,
                "player_name": player_name,
                "player_vehicle": player_vehicle,
                "tank_tag": tank_tag,
                "map": map_display_name,
                "date": replay_date_str,
                "datetime_obj": replay_datetime_obj.isoformat() if replay_datetime_obj else None,
                "tank_and_map_info": tank_and_map_info,
                "server_name": server_name,
                "version": version,
                "is_complete": "Yes" if is_complete else "No",
                "damage_dealt": damage_dealt
            })

        except Exception as e:
            # Skip failed replays but log to stderr
            print(f"Failed to parse replay: '{file_name}' -> {e}", file=sys.stderr)
            continue

    return replays


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: replay_parser.py <replay_directory>", file=sys.stderr)
        sys.exit(1)

    replay_dir = sys.argv[1]
    parsed_replays = get_replay_data(replay_dir)

    # Output JSON to stdout
    print(json.dumps(parsed_replays, indent=2, ensure_ascii=False))
