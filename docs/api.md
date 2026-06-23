# HTTP API

The ESP32 serves all routes on the local network or fallback AP. Command success responses remain `{"ok":true}`. If shared state cannot be updated, command routes return HTTP `503` with `{"ok":false,"error":"state_unavailable"}`.

## Routes

| Method | Route | Purpose | Normal response |
| --- | --- | --- | --- |
| GET | `/` | Dashboard HTML. | `text/html` |
| GET | `/api/status` | Sensor, system, Wi-Fi, heap, and diagnostics. | JSON object |
| GET | `/api/logs` | Bounded event log. | JSON array |
| POST | `/api/mode/auto` | Select AUTO mode. | `{"ok":true}` |
| POST | `/api/mode/manual` | Select MANUAL mode. | `{"ok":true}` |
| POST | `/api/mode/away` | Select AWAY mode. | `{"ok":true}` |
| POST | `/api/relay1/on` | Turn Relay 1 on and select MANUAL. | `{"ok":true}` |
| POST | `/api/relay1/off` | Turn Relay 1 off and select MANUAL. | `{"ok":true}` |
| POST | `/api/relay2/on` | Turn Relay 2 on and select MANUAL. | `{"ok":true}` |
| POST | `/api/relay2/off` | Turn Relay 2 off and select MANUAL. | `{"ok":true}` |

## Status fields

| Field | Type | Description |
| --- | --- | --- |
| `temperature_c` | number or null | DHT11 temperature. |
| `humidity` | number or null | DHT11 relative humidity. |
| `dht_ok` | boolean | Latest DHT11 read status. |
| `distance_cm` | integer or null | HC-SR04 distance. |
| `distance_ok` | boolean | Latest ultrasonic read status. |
| `light_is_dark` | boolean | Interpreted LDR darkness flag. |
| `occupied` | boolean | Occupancy after hold logic. |
| `mode` | string | `AUTO`, `MANUAL`, or `AWAY`. |
| `state` | string | `BOOT`, `CALIBRATING`, `AUTO_MONITORING`, `MANUAL_OVERRIDE`, `ALERT`, or `FAULT`. |
| `relay1`, `relay2` | boolean | Commanded relay states. |
| `temperature_alert` | boolean | Temperature hysteresis latch. |
| `fault_reason` | string | Active fault text or empty string. |
| `uptime_s` | integer | Runtime in seconds. |
| `firmware_name`, `firmware_version`, `build_label` | string | Firmware metadata. |
| `sensor_stale` | boolean | Latest sensor snapshot is older than the stale timeout. |
| `dht_consecutive_failures`, `ultrasonic_consecutive_failures` | integer | Current bounded failure counters. |
| `dht_total_failures`, `ultrasonic_total_failures` | integer | Total failures since boot. |
| `last_dht_ok_ms`, `last_distance_ok_ms` | integer | Last successful sensor timestamps. |
| `task_sensor_heartbeat_ms`, `task_control_heartbeat_ms`, `task_web_heartbeat_ms`, `task_heartbeat_heartbeat_ms` | integer | Task heartbeat timestamps. |
| `heap_min_free`, `heap_free` | integer | Minimum and current free heap bytes. |
| `reset_reason` | string | Reset reason when available. |
| `wifi_reconnect_state` | string | Station/fallback connection state. |
| `watchdog_enabled` | boolean | Watchdog enablement status. |
| `wifi_mode` | string | `STA` or `AP`. |
| `ip` | string | Current dashboard IP address. |
| `rssi` | integer | Station RSSI when connected in station mode. |

## Example responses

```json
{
  "temperature_c":24.5,
  "humidity":51.0,
  "dht_ok":true,
  "distance_cm":32,
  "distance_ok":true,
  "light_is_dark":true,
  "occupied":true,
  "mode":"AUTO",
  "state":"AUTO_MONITORING",
  "relay1":true,
  "relay2":false,
  "temperature_alert":false,
  "fault_reason":"",
  "uptime_s":120,
  "wifi_mode":"STA",
  "ip":"192.168.1.50",
  "heap_free":180000
}
```

```json
["boot complete","wifi connected","mode AUTO"]
```

## Command examples

```bash
curl http://192.168.4.1/api/status
curl http://192.168.4.1/api/logs
curl -X POST http://192.168.4.1/api/mode/away
curl -X POST http://192.168.4.1/api/relay1/on
```

## Response and caching notes

JSON and dashboard responses include no-store cache headers where practical so the browser and local clients request fresh state. The API is intended for trusted local networks or the fallback AP during setup; it does not include authentication in the current firmware.
