from wotreplay import ReplayData
replay = ReplayData(file_path='<path_to_replay_file>',
                     db_path='', db_name='', load=False)

print(replay.battle_metadata)
print("================================")
print(replay.battle_performance)
print("================================")
print(replay.common)
print("================================")
print(replay.battle_frags)
print("================================")
print(replay.battle_economy)
print("================================")
print(replay.battle_xp)