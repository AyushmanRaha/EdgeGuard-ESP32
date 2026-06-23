# EdgeGuard ESP32 RTOS Smart Room Node

EdgeGuard is a local-first ESP32 smart-room controller built with Arduino and FreeRTOS tasks. It reads a DHT11, HC-SR04, and LDR module, controls two low-voltage relay outputs, exposes a local dashboard, and provides a small HTTP API for status, logs, mode selection, and manual relay commands.

## Hardware
- ESP32 DevKit board
- DHT11 temperature/humidity sensor
- HC-SR04 ultrasonic distance sensor with ECHO level shifted to 3.3 V through a divider
- Digital LDR module
- Two low-voltage relay channels
- Green and red status LEDs

## Pin map
| Function | ESP32 GPIO |
| --- | ---: |
| DHT11 DATA | 4 |
| HC-SR04 TRIG | 5 |
| HC-SR04 ECHO | 18 |
| LDR DO | 34 |
| Relay 1 | 26 |
| Relay 2 | 27 |
| Green LED | 23 |
| Red LED | 22 |

## Safety
This repository documents low-voltage DC prototyping only. Do not connect mains AC loads to a breadboard or exposed relay module. Use properly rated isolated hardware and qualified guidance for any high-voltage installation.

## Architecture
The firmware lives in `firmware/EdgeGuard_ESP32/` and keeps Arduino IDE compatibility while splitting responsibilities into modules: configuration, shared state, sensors, control, Wi-Fi, web routes, FreeRTOS tasks, and dashboard HTML. See [architecture](docs/architecture.md) and [design rationale](docs/design_rationale.md).

## Operating modes and states
Modes are `AUTO`, `MANUAL`, and `AWAY`. States are `BOOT`, `CALIBRATING`, `AUTO_MONITORING`, `MANUAL_OVERRIDE`, `ALERT`, and `FAULT`.

- `AUTO`: Relay 1 turns on only when the room is dark and occupancy is held; Relay 2 follows the temperature alert latch.
- `MANUAL`: Relay API commands switch to manual control and immediately apply the requested relay state.
- `AWAY`: Relay 1 stays off; instant occupancy raises `ALERT` and turns Relay 2 on.
- `FAULT`: repeated DHT11 or HC-SR04 failures, or stale sensor snapshots, force both relays off.

## API
| Method | Route | Purpose |
| --- | --- | --- |
| GET | `/` | Dashboard |
| GET | `/api/status` | Sensor, control, Wi-Fi, and heap status |
| GET | `/api/logs` | Bounded event log |
| POST | `/api/mode/auto` | Select AUTO mode |
| POST | `/api/mode/manual` | Select MANUAL mode |
| POST | `/api/mode/away` | Select AWAY mode |
| POST | `/api/relay1/on` | Relay 1 on and MANUAL mode |
| POST | `/api/relay1/off` | Relay 1 off and MANUAL mode |
| POST | `/api/relay2/on` | Relay 2 on and MANUAL mode |
| POST | `/api/relay2/off` | Relay 2 off and MANUAL mode |

Detailed field descriptions are in [API documentation](docs/api.md).

## Build and flash
### Arduino IDE
Open `firmware/EdgeGuard_ESP32/EdgeGuard_ESP32.ino`, install ESP32 board support plus the DHT sensor dependencies, select an ESP32 DevKit target, and upload.

### PlatformIO
```bash
python -m pip install -r requirements.txt
pio run
pio run -t upload
```

## Configuration
Copy `firmware/EdgeGuard_ESP32/secrets.h.example` to `firmware/EdgeGuard_ESP32/secrets.h` for station Wi-Fi. Leave placeholders unchanged to use fallback AP mode.

Fallback AP defaults:
- SSID: `EdgeGuard-ESP32`
- Password: `edgeguard123`

Never commit `secrets.h`.

## Repository structure
```text
firmware/EdgeGuard_ESP32/   Arduino-compatible firmware modules and dashboard
hardware/                   Wiring reference
media/                      Optional project media
docs/                       Architecture, API, hardware, CI, and testing docs
tools/verify_repo.py        Repository verification checks
.github/workflows/ci.yml    PlatformIO build and verification workflow
```

## Testing checklist
Run `python tools/verify_repo.py` and `pio run`, then use the manual checklist in [testing plan](docs/testing_plan.md) for boot, sensors, modes, API, relay fail-safe, and AP fallback.

## CI
GitHub Actions installs PlatformIO from `requirements.txt`, runs repository verification, builds firmware, and uploads `firmware.bin`, `firmware.elf`, and `firmware.map` when produced. See [CI documentation](docs/ci.md).

## Troubleshooting
See [troubleshooting](docs/troubleshooting.md) for Wi-Fi fallback, sensor readings, relay polarity, and build notes.

## Limitations and future work
The node is intentionally local-only and does not include MQTT, OTA, authentication, or cloud telemetry. Future improvements could include unit-testable pure control helpers, richer sensor diagnostics, and enclosure-ready wiring documentation.

## License
See [LICENSE](LICENSE).
