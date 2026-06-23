# Architecture

The sketch is split into Arduino-compatible modules under `firmware/EdgeGuard_ESP32/`.

| Module | Responsibility |
| --- | --- |
| `EdgeGuard_ESP32.ino` | Minimal `setup()`/`loop()` orchestration |
| `config.h` | Pin map, thresholds, periods, relay and LDR polarity |
| `types.h` | Modes, states, sensor snapshot, system snapshot |
| `app_state.*` | Shared snapshots, mutex-protected copies, bounded event log |
| `sensors.*` | DHT11, LDR, and HC-SR04 reads with echo timeout |
| `control_logic.*` | Pure control decision helper for state-machine testing without hardware writes |
| `control.*` | Relay driver, fault handling integration, and applying computed decisions |
| `diagnostics.*` | Firmware metadata, sensor/task/Wi-Fi/runtime diagnostics, and watchdog status |
| `wifi_manager.*` | Station connection and fallback AP |
| `web_routes.*` | Dashboard route, JSON builders, and API handlers |
| `tasks.*` | FreeRTOS task loops and task creation checks |
| `dashboard.h` | Embedded local dashboard HTML |

## Task responsibilities
- Sensor task reads hardware on `SENSOR_TASK_PERIOD_MS` and publishes a bounded snapshot.
- Control task copies the latest sensor snapshot, updates the state machine, and applies relays.
- Web task services the local HTTP server.
- Heartbeat task drives LEDs according to normal, alert, or fault state.

## Shared-state policy
Code copies state under a mutex and releases the mutex before sensor reads, relay writes, serial output, HTTP responses, or longer JSON construction. If critical state access fails, relay outputs are forced off where appropriate and API command routes return a JSON error.
