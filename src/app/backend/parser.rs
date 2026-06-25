use crate::app::model::ReplayInfo;

pub fn parse_replay_file(path: &str) -> Result<ReplayInfo, String> {
    let replay_parser = wot_replay_parser::ReplayParser::parse_file(path)
        .map_err(|e| format!("Failed to parse replay: {e:?}"))?;

    let start = replay_parser
        .replay_json_start()
        .map_err(|e| format!("Failed to get start JSON: {e:?}"))?;

    let replay_json_end: Option<serde_json::Value> =
        replay_parser.replay_json_end().map(|v| v.clone());

    let mut damage = start.get("damageDealt").and_then(|v| v.as_i64()).unwrap_or(0);

    if damage == 0 {
        damage = replay_json_end
            .as_ref()
            .and_then(|end| end.as_array())
            .and_then(|arr| arr.first())
            .and_then(|block| block.get("personal"))
            .and_then(|personal| personal.as_object())
            .and_then(|vehicles| vehicles.values().next())
            .and_then(|vehicle| vehicle.get("damageDealt"))
            .and_then(|dmg| dmg.as_i64())
            .unwrap_or(0);
    }

    Ok(ReplayInfo {
        path: path.to_string(),
        player_name: start
            .get("playerName")
            .and_then(|v| v.as_str())
            .unwrap_or("")
            .to_string(),
        tank: start
            .get("playerVehicle")
            .and_then(|v| v.as_str())
            .unwrap_or("")
            .to_string(),
        map: start
            .get("mapDisplayName")
            .and_then(|v| v.as_str())
            .unwrap_or("")
            .to_string(),
        date: start
            .get("dateTime")
            .and_then(|v| v.as_str())
            .unwrap_or("")
            .to_string(),
        damage,
        server: start
            .get("serverName")
            .and_then(|v| v.as_str())
            .unwrap_or("")
            .to_string(),
        version: start
            .get("clientVersionFromXml")
            .and_then(|v| v.as_str())
            .unwrap_or("")
            .to_string(),
    })
}

