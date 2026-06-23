# EdgeGuard ESP32 RTOS Smart Room Node

[![CI](https://github.com/AyushmanRaha/edgeguard-esp32-rtos-smart-room-node/actions/workflows/ci.yml/badge.svg)](https://github.com/AyushmanRaha/edgeguard-esp32-rtos-smart-room-node/actions/workflows/ci.yml)

EdgeGuard is an embedded-software portfolio project: an ESP32 Arduino smart-room node with FreeRTOS-style tasks, local web dashboard, deterministic control logic, host unit tests, and CI. **No hardware changes are required for this reliability upgrade.**


EdgeGuard-ESP32 is an ESP32-based RTOS-style smart room monitoring node.

It uses sensors such as DHT11, HC-SR04, and LDR, along with a relay module and local web dashboard, to monitor environmental and room conditions and control low-voltage actuators.

## Repository Structure

```text
.
├── .github/
│   └── workflows/
├── docs/
├── firmware/
│   └── EdgeGuard_ESP32/
├── hardware/
├── scripts/
├── tests/
│   └── host_logic_tests/
├── .clang-format
├── .editorconfig
├── .gitignore
├── AGENTS.md
├── LICENSE
└── README.md
```

## Hardware used
- ESP32 DevKit
- DHT11 temperature/humidity sensor
- HC-SR04 ultrasonic sensor with Echo voltage divider
- LDR digital light module
- 2-channel relay module switching only low-voltage LED loads
- Green heartbeat LED and red alert/fault LED

> Safety: this project is for low-voltage LED loads only. Do not wire or document AC mains switching.

## Exact pin map
| Function | ESP32 GPIO |
|---|---:|
| DHT11 DATA | 4 |
| HC-SR04 TRIG | 5 |
| HC-SR04 ECHO through divider | 18 |
| LDR DO | 34 |
| Relay IN1 | 26 |
| Relay IN2 | 27 |
| Green heartbeat LED | 23 |
| Red alert/fault LED | 22 |

## HC-SR04 Echo voltage divider
HC-SR04 Echo is 5 V, while ESP32 GPIO is 3.3 V logic. Use a resistor divider before GPIO 18 and connect all grounds together. See `hardware/wiring_table.md` for diagrams and resistor options.

## Firmware architecture
- `EdgeGuard_ESP32.ino`: Arduino integration layer for Wi-Fi, web server, sensors, GPIO, FreeRTOS tasks, and dashboard.
- `edgeguard_control.*`: pure C++ state-machine logic with no Arduino hardware calls.
- `edgeguard_types.h`: shared enums and snapshots.
- `edgeguard_event_log.*`: fixed-size ring buffer for recent dashboard events.

## Task architecture
- Sensor task samples DHT11, HC-SR04, and LDR.
- Control task calls pure control logic and applies relay outputs.
- Web task serves dashboard and JSON endpoints.
- Heartbeat task drives green/red LEDs based on state.

## State machine
Priority order is FAULT, MANUAL, AWAY, then AUTO. AUTO turns Relay 1 on when dark and occupied, and Relay 2 on during temperature alert. AWAY triggers ALERT and Relay 2 for nearby movement. FAULT forces both relays off.

## Dashboard endpoints
- `GET /`
- `GET /api/status`
- `GET /api/logs`
- `POST /api/mode/auto`
- `POST /api/mode/manual`
- `POST /api/mode/away`
- `POST /api/relay1/on`, `POST /api/relay1/off`
- `POST /api/relay2/on`, `POST /api/relay2/off`

## Local setup with Arduino IDE
1. Install ESP32 board support in Arduino IDE.
2. Install `DHT sensor library` and `Adafruit Unified Sensor`.
3. Copy `firmware/EdgeGuard_ESP32/secrets.h.example` to `secrets.h` and enter Wi-Fi credentials.
4. Open `firmware/EdgeGuard_ESP32/EdgeGuard_ESP32.ino`.
5. Select an ESP32 Dev Module board and upload.

## Local host tests
```bash
bash scripts/run_host_tests.sh
python3 scripts/validate_repo.py
python3 scripts/check_firmware_structure.py
```

## CI explanation
GitHub Actions runs host C++ tests and validation scripts, then compiles the Arduino sketch with ESP32 core and dummy CI Wi-Fi credentials. CI does not upload firmware and does not need hardware.

## Demo checklist
- Show dashboard loading over Wi-Fi or fallback AP.
- Cover LDR and move hand near HC-SR04 to turn Relay 1 on in AUTO.
- Warm DHT11 carefully to trigger temperature alert and Relay 2.
- Switch MANUAL buttons and show direct relay control.
- Explain FAULT behavior and host tests.

## Troubleshooting
See `docs/troubleshooting.md` for Mac serial-port issues, compile failures, dashboard connection problems, DHT11 null readings, HC-SR04 no echo, LDR inversion, relay active-low behavior, and fallback AP mode.

## What changed in the reliability upgrade
- Split control logic from Arduino hardware IO for normal-computer tests.
- Added assert-style C++ host tests for state transitions and event logging.
- Added CI for host tests, repo validation, firmware structure, and Arduino compile.
- Expanded documentation for wiring, architecture, state machine, testing, demos, and interviews.

## Resume bullet
Built an ESP32 Arduino smart-room monitoring node with FreeRTOS-style tasks, local web dashboard, deterministic C++ control logic, host unit tests, GitHub Actions Arduino compile CI, and safe low-voltage relay control.
