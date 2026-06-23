# HTTP API

All routes are served by the ESP32 local web server. Normal command responses remain `{"ok":true}`. If a command cannot update shared state, the firmware returns HTTP `503` with `{"ok":false,"error":"state_unavailable"}`.

## Routes
| Method | Route | Response |
| --- | --- | --- |
| GET | `/` | HTML dashboard |
| GET | `/api/status` | Status JSON |
| GET | `/api/logs` | JSON array of event strings |
| POST | `/api/mode/auto` | `{"ok":true}` |
| POST | `/api/mode/manual` | `{"ok":true}` |
| POST | `/api/mode/away` | `{"ok":true}` |
| POST | `/api/relay1/on` | `{"ok":true}` |
| POST | `/api/relay1/off` | `{"ok":true}` |
| POST | `/api/relay2/on` | `{"ok":true}` |
| POST | `/api/relay2/off` | `{"ok":true}` |

## Status fields
- `temperature_c`: DHT11 temperature or `null`.
- `humidity`: DHT11 humidity or `null`.
- `dht_ok`: DHT11 read status.
- `distance_cm`: HC-SR04 distance or `null`.
- `distance_ok`: HC-SR04 echo status.
- `light_is_dark`: interpreted LDR darkness flag.
- `occupied`: occupancy after hold timeout logic.
- `mode`: `AUTO`, `MANUAL`, or `AWAY`.
- `state`: `BOOT`, `CALIBRATING`, `AUTO_MONITORING`, `MANUAL_OVERRIDE`, `ALERT`, or `FAULT`.
- `relay1`, `relay2`: commanded relay states.
- `temperature_alert`: temperature hysteresis latch.
- `fault_reason`: active fault text or empty string.
- `uptime_s`: uptime in seconds.
- `wifi_mode`: `STA` or `AP`.
- `ip`: current dashboard IP address.
- `heap_free`: free heap bytes.
- `firmware_name`, `firmware_version`, `build_label`: firmware metadata.
- `sensor_stale`: true when the latest sensor snapshot is older than the configured stale timeout.
- `dht_consecutive_failures`, `ultrasonic_consecutive_failures`: current bounded consecutive sensor failure counters.
- `dht_total_failures`, `ultrasonic_total_failures`: total sensor failure counters since boot.
- `last_dht_ok_ms`, `last_distance_ok_ms`: last successful sensor read timestamps from `millis()`.
- `task_sensor_heartbeat_ms`, `task_control_heartbeat_ms`, `task_web_heartbeat_ms`, `task_heartbeat_heartbeat_ms`: task loop heartbeat timestamps from `millis()`.
- `heap_min_free`: minimum observed free heap bytes.
- `reset_reason`: ESP32 reset reason string when available.
- `wifi_reconnect_state`: station connection or fallback AP state.
- `watchdog_enabled`: true when the ESP32 task watchdog was enabled by the selected framework.
- `rssi`: station RSSI when connected in station mode.
