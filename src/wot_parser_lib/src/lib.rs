use std::ffi::{CString, CStr};
use std::os::raw::c_char;
use wot_replay_parser::ReplayParser;
use serde_json::{json, Value};

#[unsafe(no_mangle)]
pub unsafe extern "C" fn parse_replay(path_to_replay_file: *const c_char) -> *const c_char {
    let c_str = CStr::from_ptr(path_to_replay_file);
    let path = match c_str.to_str() {
        Ok(s) => s,
        Err(e) => {
            let error = format!("Failed to convert CStr to str: {}", e);
            return CString::new(error).unwrap().into_raw();
        }
    };

    let replay_parser = match ReplayParser::parse_file(path) {
        Ok(r) => r,
        Err(e) => {
            let error = format!("Failed to parse replay: {:?}", e);
            return CString::new(error).unwrap().into_raw();
        }
    };

    let start = match replay_parser.replay_json_start() {
        Ok(v) => v,
        Err(e) => {
            let error = format!("Failed to get start JSON: {:?}", e);
            return CString::new(error).unwrap().into_raw();
        }
    };

    let replay_json_end: Option<Value> = replay_parser.replay_json_end().map(|v| v.clone());

    let mut damage = start.get("damageDealt").and_then(|v| v.as_i64()).unwrap_or(0);
    //println!("Rust Parser: Damage from start JSON: {}", damage);

    if damage == 0 {
        if let Some(end) = &replay_json_end {
            if let Some(player_performance_block) = end.as_array().and_then(|arr| arr.get(0)) {
                if let Some(personal) = player_performance_block.get("personal").and_then(|v| v.as_object()) {
                    if let Some((_veh_id, vehicle)) = personal.iter().next() {
                        damage = vehicle.get("damageDealt").and_then(|v| v.as_i64()).unwrap_or(0);
                        //println!("Rust Parser:  Damage from end.personal (first vehicle): {}", damage);
                    } else {
                        //println!("Rust Parser: 'personal' block exists but no vehicles found");
                    }
                } else {
                    //println!("Rust Parser: 'personal' block not found in the first element of the end JSON array");
                }

            } else {
                //println!("Rust Parser: End JSON is not an array or is empty");
            }
        } else {
            //println!("Rust Parser: No end JSON available");
        }
    }

    let flattened = json!({
        "path": path,
        "playerName": start.get("playerName").and_then(|v| v.as_str()).unwrap_or(""),
        "tank": start.get("playerVehicle").and_then(|v| v.as_str()).unwrap_or(""),
        "map": start.get("mapDisplayName").and_then(|v| v.as_str()).unwrap_or(""),
        "date": start.get("dateTime").and_then(|v| v.as_str()).unwrap_or(""),
        "damage": damage,
        "server": start.get("serverName").and_then(|v| v.as_str()).unwrap_or(""),
        "version": start.get("clientVersionFromXml").and_then(|v| v.as_str()).unwrap_or("")
    });

    let json_string = serde_json::to_string_pretty(&flattened).unwrap();
    CString::new(json_string).unwrap().into_raw()
}

#[unsafe(no_mangle)]
pub extern "C" fn free_string(s: *mut c_char) {
    if s.is_null() {
        return;
    }
    unsafe { CString::from_raw(s); }
}
