use crate::app::model::ReplayInfo;

use rusqlite::params;
use rusqlite::Connection;
use std::path::Path;

pub fn open_cache(db_path: &Path) -> rusqlite::Result<Connection> {
    let conn = Connection::open(db_path)?;

    conn.execute(
        "CREATE TABLE IF NOT EXISTS replays (\
            path TEXT PRIMARY KEY,\
            playerName TEXT,\
            tank TEXT,\
            map TEXT,\
            date TEXT,\
            damage INTEGER,\
            server TEXT,\
            version TEXT\
        )",
        [],
    )?;

    Ok(conn)
}

pub fn load_replay_cache(
    conn: &Connection,
) -> rusqlite::Result<(Vec<ReplayInfo>, Vec<String>)> {
    let mut stmt = conn.prepare(
        "SELECT path, playerName, tank, map, date, damage, server, version FROM replays",
    )?;

    let mut rows = stmt.query([])?;

    let mut replays = Vec::new();
    let mut cache_paths = Vec::new();

    while let Some(row) = rows.next()? {
        let path: String = row.get(0)?;
        let player_name: String = row.get(1)?;
        let tank: String = row.get(2)?;
        let map: String = row.get(3)?;
        let date: String = row.get(4)?;
        let damage: i64 = row.get(5)?;
        let server: String = row.get(6)?;
        let version: String = row.get(7)?;

        cache_paths.push(path.clone());

        if std::path::Path::new(&path).exists() {
            replays.push(ReplayInfo {
                path,
                player_name,
                tank,
                map,
                date,
                damage,
                server,
                version,
            });
        }
    }

    Ok((replays, cache_paths))
}

pub fn save_replay_cache(
    conn: &mut Connection,
    replays: &[ReplayInfo],
) -> rusqlite::Result<()> {
    conn.execute_batch("BEGIN IMMEDIATE")?;

    let mut stmt = conn.prepare(
        "INSERT OR REPLACE INTO replays\
            (path, playerName, tank, map, date, damage, server, version)\
            VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8)",
    )?;

    for info in replays {
        stmt.execute(params![
            &info.path,
            &info.player_name,
            &info.tank,
            &info.map,
            &info.date,
            info.damage,
            &info.server,
            &info.version
        ])?;
    }

    conn.execute_batch("COMMIT")?;
    Ok(())
}



pub fn delete_stale_entries(
    conn: &Connection,
    paths_to_delete: &[String],
) -> rusqlite::Result<()> {
    if paths_to_delete.is_empty() {
        return Ok(());
    }

    let placeholders = std::iter::repeat("?")
        .take(paths_to_delete.len())
        .collect::<Vec<_>>()
        .join(",");

    let sql = format!(
        "DELETE FROM replays WHERE path IN ({})",
        placeholders
    );

    // Execute requires &mut Connection only for some operations; execute() takes &self.
    // We can keep conn as shared.
    let params_vec: Vec<&dyn rusqlite::ToSql> = paths_to_delete
        .iter()
        .map(|p| p as &dyn rusqlite::ToSql)
        .collect();

    conn.execute(&sql, params_vec.as_slice())?;
    Ok(())
}

