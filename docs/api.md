# API Reference

The API remains local to the ESP32. Existing routes and core JSON fields are preserved.

## Routes

| Method | Route | Response |
|---|---|---|
| GET | `/` | HTML dashboard |
| GET | `/api/status` | Live JSON object |
| GET | `/api/logs` | JSON array of event strings |
| POST | `/api/mode/auto` | `{"ok":true}` |
| POST | `/api/mode/manual` | `{"ok":true}` |
| POST | `/api/mode/away` | `{"ok":true}` |
| POST | `/api/relay1/on` | `{"ok":true}` |
| POST | `/api/relay1/off` | `{"ok":true}` |
| POST | `/api/relay2/on` | `{"ok":true}` |
| POST | `/api/relay2/off` | `{"ok":true}` |

`/api/status` and `/api/logs` include `Cache-Control: no-store` and CORS headers for local tooling compatibility.

## Status fields

| Field | Meaning |
|---|---|
| `temperature_c` | DHT11 temperature in Celsius or `null`. |
| `humidity` | DHT11 relative humidity or `null`. |
| `dht_ok` | DHT11 read validity. |
| `distance_cm` | HC-SR04 distance or `null`. |
| `distance_ok` | HC-SR04 echo validity. |
| `light_is_dark` | LDR dark interpretation after configured polarity. |
| `occupied` | Occupancy after hold logic. |
| `mode` | `AUTO`, `MANUAL`, or `AWAY`. |
| `state` | `BOOT`, `CALIBRATING`, `AUTO_MONITORING`, `MANUAL_OVERRIDE`, `ALERT`, or `FAULT`. |
| `relay1`, `relay2` | Logical relay on/off state. |
| `temperature_alert` | Temperature hysteresis latch. |
| `fault_reason` | Empty string or fault text. |
| `uptime_s` | Device uptime seconds. |
| `wifi_mode`, `ip`, `heap_free`, `rssi` | Optional diagnostics added for the dashboard. |
