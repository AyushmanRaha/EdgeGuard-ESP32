# EdgeGuard ESP32 RTOS Smart Room Node

A local-first ESP32 room-sensing and relay-control node built with Arduino-compatible firmware, FreeRTOS task separation, an onboard dashboard, HTTP API, diagnostics, and host-side control-logic tests.

[![CI](https://github.com/AyushmanRaha/edgeguard-esp32-rtos-smart-room-node/actions/workflows/ci.yml/badge.svg)](https://github.com/AyushmanRaha/edgeguard-esp32-rtos-smart-room-node/actions/workflows/ci.yml)
![PlatformIO](https://img.shields.io/badge/PlatformIO-ready-orange)
![ESP32](https://img.shields.io/badge/ESP32-DevKit-blue)
![Arduino](https://img.shields.io/badge/Arduino-compatible-teal)
![FreeRTOS](https://img.shields.io/badge/FreeRTOS-task_split-green)
![Local First](https://img.shields.io/badge/local--first-no_cloud_dependency-lightgrey)
[![License](https://img.shields.io/badge/license-MIT-blue)](LICENSE)

## Executive overview

EdgeGuard ESP32 RTOS Smart Room Node monitors a room with temperature, humidity, distance, and light sensors. It decides when low-voltage relay outputs should change, reports the current state through a local web dashboard, and exposes a compact HTTP API for local automation experiments.

The design is local-first: the ESP32 hosts the interface itself, so basic monitoring and control do not depend on external services. That makes the project useful for lab benches, classrooms, and offline prototyping where deterministic local behavior is more important than remote integrations.

## Key capabilities

| Capability | What it provides |
| --- | --- |
| Real-time room sensing | DHT11 temperature/humidity, HC-SR04 distance, and digital LDR readings. |
| Occupancy-aware relay control | Relay 1 turns on in AUTO only when the room is dark and occupancy is held. |
| Temperature alert hysteresis | Relay 2 follows a latched alert with separate on/off thresholds. |
| AUTO, MANUAL, and AWAY modes | Predictable behavior for autonomous, direct-command, and alert-oriented operation. |
| Local web dashboard | Browser view for mode, state, sensors, relays, IP, heap, and recent events. |
| HTTP API | Stable status, log, mode, and relay routes for local clients. |
| Fallback Wi-Fi access point | Dashboard remains reachable when station credentials are absent or unavailable. |
| FreeRTOS task separation | Sensor, control, web, and heartbeat loops run independently. |
| Watchdog and diagnostics | Heartbeats, reset reason, heap tracking, failure counters, and watchdog status. |
| Fail-safe relay behavior | Fault and stale-sensor paths force both relay outputs off. |
| Host-side unit tests | Pure control decisions are tested without physical ESP32 hardware. |


## What you can do with EdgeGuard

With the provided firmware and low-voltage hardware, you can:

- Monitor room temperature and humidity with a DHT11 module.
- Detect nearby presence with HC-SR04 ultrasonic distance readings.
- Detect dark or bright conditions with a digital LDR module.
- Control two low-voltage relay outputs locally.
- View live sensor, relay, mode, network, heap, and event data from the onboard browser dashboard.
- Use local HTTP API routes for status, logs, operating mode, and relay commands.
- Run host-side native tests for the pure control logic without ESP32 hardware.
- Study a compact ESP32 Arduino/FreeRTOS-style architecture with separated sensor, control, web, heartbeat, diagnostics, and shared-state modules.

## System overview

```mermaid
flowchart LR
  DHT[DHT11] --> FW[ESP32 firmware]
  US[HC-SR04] --> FW
  LDR[LDR digital module] --> FW
  FW --> Tasks[FreeRTOS tasks]
  Tasks --> Logic[control logic]
  Logic --> R1[Relay 1]
  Logic --> R2[Relay 2]
  Tasks --> LED[Status LEDs]
  Tasks --> Diag[diagnostics/event log]
  FW --> Dash[local dashboard]
  FW --> API[HTTP API]
  Diag --> Dash
  Diag --> API
```

## Hardware

Core parts are an ESP32 DevKit board, DHT11 sensor, HC-SR04 ultrasonic module with ECHO level shifted to 3.3 V, digital LDR module, two low-voltage relay channels, and red/green status LEDs.

| Function | ESP32 GPIO | Notes |
| --- | ---: | --- |
| DHT11 DATA | 4 | Temperature and humidity data. |
| HC-SR04 TRIG | 5 | ESP32 output. |
| HC-SR04 ECHO | 18 | Use a voltage divider to protect the 3.3 V GPIO. |
| LDR DO | 34 | Digital input only. |
| Relay 1 | 26 | Active-low by default. |
| Relay 2 | 27 | Active-low by default. |
| Green LED | 23 | Normal heartbeat/status. |
| Red LED | 22 | Alert/fault indication. |

> Safety: this repository documents low-voltage DC prototyping only. Do not connect high-voltage loads to a breadboard or exposed relay module.

Detailed references: [hardware guide](docs/hardware.md) and [wiring table](hardware/wiring_table.md).

## Firmware architecture

The firmware remains Arduino IDE compatible by keeping the sketch and sibling modules in `firmware/EdgeGuard_ESP32/`. PlatformIO builds the same directory from the repository root.

```mermaid
flowchart TD
  Setup[EdgeGuard_ESP32.ino] --> Tasks[tasks.cpp]
  Tasks --> Sensors[sensors.cpp]
  Tasks --> Control[control.cpp]
  Tasks --> Web[web_routes.cpp]
  Sensors --> State[app_state.cpp]
  Control --> Logic[control_logic.cpp]
  Logic --> State
  Web --> State
  Web --> Dashboard[dashboard.h]
  Control --> Relays[relay outputs]
  Tasks --> Diagnostics[diagnostics.cpp]
```

`control_logic.cpp` is intentionally pure: it receives snapshots and memory, then returns a decision without reading hardware or writing GPIO. This seam makes the riskiest state decisions testable on a host computer. See [architecture](docs/architecture.md) and [design rationale](docs/design_rationale.md).

## Operating modes

| Mode or state | Behavior |
| --- | --- |
| AUTO | Relay 1 follows dark-and-occupied logic; Relay 2 follows the temperature alert latch. |
| MANUAL | API relay commands select direct relay states and state becomes `MANUAL_OVERRIDE`. |
| AWAY | Relay 1 remains off; instant occupancy enters `ALERT` and turns Relay 2 on. |
| FAULT | Stale sensor data or repeated sensor failures force both relays off. |

## State machine

```mermaid
stateDiagram-v2
  [*] --> BOOT
  BOOT --> CALIBRATING
  CALIBRATING --> AUTO_MONITORING
  AUTO_MONITORING --> ALERT: temperature alert or AWAY occupancy
  ALERT --> AUTO_MONITORING: alert clears
  AUTO_MONITORING --> MANUAL_OVERRIDE: manual command
  MANUAL_OVERRIDE --> AUTO_MONITORING: AUTO selected
  AUTO_MONITORING --> FAULT: stale sensor or repeated failures
  ALERT --> FAULT: stale sensor or repeated failures
  MANUAL_OVERRIDE --> FAULT: stale sensor or repeated failures
```

## HTTP API quick reference

| Method | Route | Purpose |
| --- | --- | --- |
| GET | `/` | Dashboard. |
| GET | `/api/status` | Sensor, system, Wi-Fi, heap, and diagnostic status. |
| GET | `/api/logs` | Bounded event log as JSON array. |
| POST | `/api/mode/auto` | Select AUTO mode. |
| POST | `/api/mode/manual` | Select MANUAL mode. |
| POST | `/api/mode/away` | Select AWAY mode. |
| POST | `/api/relay1/on` | Relay 1 on and MANUAL mode. |
| POST | `/api/relay1/off` | Relay 1 off and MANUAL mode. |
| POST | `/api/relay2/on` | Relay 2 on and MANUAL mode. |
| POST | `/api/relay2/off` | Relay 2 off and MANUAL mode. |

Full route and field details are in [API documentation](docs/api.md).

## Dashboard

The local dashboard is served from `/` by the ESP32. It polls the status and log APIs, shows current mode/state, sensor readings, relay states, network details, heap information, and recent events, then provides buttons for mode and relay commands. The screenshots below show the local web dashboard in use.

### Dashboard screenshots

![EdgeGuard dashboard page 1](media/web-page-1.png)

![EdgeGuard dashboard page 2](media/web-page-2.png)

![EdgeGuard dashboard page 3](media/web-page-3.png)

### Prototype images

![EdgeGuard prototype image 1](media/image-1.jpeg)

![EdgeGuard prototype image 2](media/image-2.jpeg)

### Serial monitor validation

The Arduino IDE Serial Monitor screenshot below shows live EdgeGuard telemetry at `115200` baud, including DHT11 temperature/humidity readings, HC-SR04 distance readings, LDR state, manual relay commands, relay ON/OFF events, and the transition into `MANUAL_OVERRIDE`.

[![Arduino IDE Serial Monitor showing EdgeGuard ESP32 live sensor telemetry and manual relay override events](media/serial-monitor-manual-relay-validation.png)](media/serial-monitor-manual-relay-validation.png)

*Click the image to open the full-size serial monitor validation screenshot.*

## Quick Start

1. Clone or open this repository on your development computer.
2. Choose either the Arduino IDE workflow or the PlatformIO workflow.
3. For station Wi-Fi, copy `firmware/EdgeGuard_ESP32/secrets.h.example` to `firmware/EdgeGuard_ESP32/secrets.h` and edit only your local `secrets.h`. Leave placeholders unchanged or omit `secrets.h` to use fallback AP mode.
4. Build and upload with your chosen workflow: open `firmware/EdgeGuard_ESP32/EdgeGuard_ESP32.ino` in Arduino IDE, or use `pio run -e esp32doit-devkit-v1 -t upload` with PlatformIO.
5. Open Serial Monitor at `115200` baud.
6. Open the dashboard at the printed station IP, or connect to fallback AP `EdgeGuard-ESP32` with password `edgeguard123` and open the AP dashboard IP printed by the firmware.

For complete setup and implementation instructions, see the [Usage and Implementation Guide](docs/usage_implementation_guide.md).

## Testing

Automated checks:

```bash
python tools/verify_repo.py
pio test -e native
pio run -e esp32doit-devkit-v1
```

Native tests cover AUTO, MANUAL, AWAY, stale-sensor fault handling, occupancy hold, temperature hysteresis, invalid distance readings, and boundary behavior in `computeControlDecision(...)`. Manual validation covers boot, sensors, relays, dashboard/API, AP fallback, and safe fail states. See [testing plan](docs/testing_plan.md).

## Continuous integration

CI performs repository verification, native unit tests, firmware build, and firmware artifact upload. The workflow uses least-privilege read permissions, dependency caching, Python 3.12, and explicit PlatformIO environments. See [CI details](docs/ci.md).

## Repository structure

```text
firmware/EdgeGuard_ESP32/        Arduino-compatible firmware modules and dashboard
hardware/wiring_table.md         Concise wiring reference
docs/                            Architecture, API, hardware, testing, CI, and rationale
test/native_stubs/               Minimal Arduino/FreeRTOS stubs for native tests
test/test_control_logic/         Host-side Unity tests for pure control logic
tools/verify_repo.py             Repository integrity and content-policy checks
.github/workflows/ci.yml         Verification, test, build, and artifact workflow
platformio.ini                   ESP32 and native PlatformIO environments
```

## Safety and limitations

- Low-voltage DC prototyping only.
- No cloud dependency.
- No built-in authentication; use trusted lab or isolated local networks.
- No OTA, MQTT, or cloud integration in the current firmware.
- Relay modules vary, so validate polarity with safe low-voltage loads only.

## Troubleshooting

Common issues include USB upload instability, fallback AP discovery, stale sensor faults, inverted LDR modules, inverted relay modules, DHT read errors, HC-SR04 no echo, dashboard refresh problems, and serial monitor settings. See [troubleshooting](docs/troubleshooting.md).

## Acknowledgements

This project uses Arduino-compatible ESP32 firmware tooling, PlatformIO, FreeRTOS concepts, DHT sensor library support, and open-source embedded development tools.

## License

See [LICENSE](LICENSE).
