# Architecture

EdgeGuard-ESP32 uses an Arduino-compatible sketch with FreeRTOS-style tasks. The design keeps hardware I/O, control decisions, web serving, and LED heartbeat work separated so slow sensor reads do not block HTTP handling.

## Runtime blocks

```text
DHT11 / HC-SR04 / LDR
        ↓
SensorTask → shared SensorSnapshot
        ↓
ControlTask → relay driver + shared SystemSnapshot + event log
        ↓
WebTask → dashboard and JSON API
        ↓
HeartbeatTask → green/red status LEDs
```

## Tasks

| Task | Period | Responsibility |
|---|---:|---|
| `SensorTask` | `SENSOR_TASK_PERIOD_MS` | Read DHT11, HC-SR04, and LDR; publish `SensorSnapshot`. |
| `ControlTask` | `CONTROL_TASK_PERIOD_MS` | Apply AUTO/MANUAL/AWAY behavior, alert/fault rules, occupancy hold, temperature hysteresis, and relay outputs. |
| `WebTask` | 10 ms loop delay | Call `server.handleClient()` frequently for dashboard/API responsiveness. |
| `HeartbeatTask` | state-dependent | Blink green/red LEDs for normal, alert, and fault indication. |

## Shared-state policy

- `gMutex` protects sensor snapshots, system snapshots, and the event ring buffer.
- Snapshot helpers copy shared data quickly and release the mutex before web response sending, Serial output, sensor reads, and relay writes.
- The event log is bounded by `EVENT_LOG_SIZE`; no unbounded history is kept in RAM.

## Control behavior preserved

- AUTO: Relay 1 is on only when dark and occupied; Relay 2 follows the temperature alert latch.
- MANUAL: Relay commands set MANUAL mode and immediately write relays.
- AWAY: Relay 1 remains off; instant occupancy triggers ALERT and Relay 2.
- FAULT: repeated sensor failures force both relays off.
