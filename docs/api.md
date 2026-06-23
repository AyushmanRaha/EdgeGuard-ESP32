# HTTP API

Read endpoints: `GET /`, `GET /api/status`, `GET /api/logs`, `GET /api/diagnostics`, `GET /api/history`, `GET /api/history.csv`.

Control endpoints: `POST /api/mode/auto`, `POST /api/mode/manual`, `POST /api/mode/away`, `POST /api/relay1/on`, `POST /api/relay1/off`, `POST /api/relay2/on`, `POST /api/relay2/off`, `POST /api/fault/reset`.

When `EDGEGUARD_API_TOKEN` is configured, send:

```bash
curl -X POST http://edgeguard.local/api/mode/auto -H 'X-EdgeGuard-Token: change-this-local-token'
```

Unauthorized response:

```json
{"ok":false,"error":"unauthorized"}
```

Status response includes temperature, humidity, distance, light, occupancy, mode, state, relay states, temperature alert, fault reason, and uptime.

History CSV header:

```text
timestamp_ms,temperature_c,humidity,dht_ok,distance_cm,distance_ok,light_is_dark,occupied,relay1,relay2,state,mode
```
