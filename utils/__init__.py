import os
from wotreplay import ReplayData

def get_replay_data(replay_dir):
    """
    Parses replay files in a given directory to extract key metadata.
    ...
    """
    replays = []
    if not os.path.exists(replay_dir):
        print(f"Error: Replay directory not found at '{replay_dir}'")
        return replays

    for file_name in os.listdir(replay_dir):
        if file_name.endswith(".wotreplay"):
            file_path = os.path.join(replay_dir, file_name)
            try:
                replay = ReplayData(file_path=file_path)
                metadata = replay.battle_metadata[0]
                is_complete_str = "Yes" if metadata.get("battle_type") == 1 else "No"
                damage_dealt = 0

                if is_complete_str == "Yes":
                    battle_result = replay.battle_performance[0]
                    damage_dealt = battle_result.get('damage_dealt', 0)

                replay_datetime_obj = metadata["replay_date"]
                replay_date_str = replay_datetime_obj.strftime("%Y-%m-%d %H:%M")
                player_vehicle = metadata["tank_tag"]
                map_display_name = metadata["map_display_name"]
                player_name = metadata["player_name"]
                server_name = metadata["server_name"]
                tank_tag = metadata["tank_tag"]
                version = metadata["version"]
                tank_and_map_info = f"{player_vehicle} on {map_display_name}"

                replays.append(
                    {
                        "path": file_path,
                        "player_name": player_name,
                        "tank": player_vehicle,
                        "tank_tag": tank_tag,
                        "map": map_display_name,
                        "date": replay_date_str,
                        "datetime_obj": replay_datetime_obj,
                        "tank_and_map_info": tank_and_map_info,
                        "server_name": server_name,
                        "version": version,
                        "is_complete": is_complete_str,
                        "damage_dealt": damage_dealt,
                    }
                )

            except Exception as e:  # noqa: F841
                #print(f"Failed to parse replay file '{file_name}': {e}")
                continue

    return replays