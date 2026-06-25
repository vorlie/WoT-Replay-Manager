## Tank Data Source (Wargaming API)

WoT Replay Manager uses the official Wargaming Tankopedia API to build a lightweight local tank mapping.

Wargaming.net provides a vehicle endpoint that returns full tank metadata, which is then reduced to only the fields required by the application.

API reference:
[https://developers.wargaming.net/reference/all/wot/encyclopedia/vehicles/](https://developers.wargaming.net/reference/all/wot/encyclopedia/vehicles/)

---

## Data Flow

The raw API response contains metadata, pagination info, and a dictionary of vehicles keyed by internal IDs:

```json
{
  "status": "ok",
  "meta": { ... },
  "data": {
    "1": {
      "tag": "R04_T-34",
      "name": "T-34"
    }
  }
}
```

Only a small subset of this data is required by the app.

---

## Required Format

The application uses a simplified mapping:

```json
{
  "R04_T-34": "T-34",
  "F08_AMX_50_100": "AMX 50 100",
  "R228_KR_1": "KR-1"
}
```

This structure allows fast lookup from internal vehicle tag → display name.

---

## Python Transformation Script

A small utility script converts the full API response into the simplified mapping:

```python
import json

# Load raw API response
with open("tanks_raw.json", "r", encoding="utf-8") as f:
    raw = json.load(f)

# Build simplified tag -> name mapping
parsed = {
    tank["tag"]: tank["name"]
    for tank in raw.get("data", {}).values()
    if "tag" in tank and "name" in tank
}

# Save output
with open("tank_mapping.json", "w", encoding="utf-8") as f:
    json.dump(parsed, f, indent=2, ensure_ascii=False)

print(f"Generated tank_mapping.json with {len(parsed)} tanks.")
```

---

## API Parameters

To minimize response size, only required fields should be requested:

### `fields`

Comma-separated list of returned fields.

Recommended:

```
fields=tag,name
```

### `limit`

Maximum number of returned entries (default: 100, max: 100 per request).

---

## Notes

* The mapping file is derived directly from the Wargaming Tankopedia API response.
* Only `tag -> name` is stored to reduce file size and improve lookup speed.
* The file can be regenerated at any time without affecting application logic.
* This design allows custom overrides, localization, or future game updates without requiring an application update.
