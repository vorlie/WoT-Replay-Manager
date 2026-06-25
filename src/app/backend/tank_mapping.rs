use serde_json::Value;
use std::collections::HashMap;

pub fn load_tank_mapping_from_file(path: &str) -> HashMap<String, String> {
    let mut map = HashMap::new();
    let data = match std::fs::read_to_string(path) {
        Ok(s) => s,
        Err(_) => return map,
    };

    let v: Value = match serde_json::from_str(&data) {
        Ok(v) => v,
        Err(_) => return map,
    };

    let obj = match v.as_object() {
        Some(o) => o,
        None => return map,
    };

    for (k, val) in obj {
        if let Some(s) = val.as_str() {
            map.insert(k.clone(), s.to_string());
        }
    }

    map
}

pub fn map_tank_display(full_tank_str: &str, tank_map: &HashMap<String, String>) -> String {
    let mut suffix_label = "".to_string();
    let mut tank = full_tank_str.to_string();

    if tank.ends_with("_FEP23") {
        suffix_label = " (Overwhelming Fire)".to_string();
        let new_len = tank.len() - 6;
        tank.truncate(new_len);
    }

    let tank_id = tank
        .split('-')
        .nth(1)
        .unwrap_or("")
        .to_string();

    if let Some(mapped) = tank_map.get(&tank_id) {
        format!("{}{}", mapped, suffix_label)
    } else {
        format!("{}{}", tank, suffix_label)
    }
}

