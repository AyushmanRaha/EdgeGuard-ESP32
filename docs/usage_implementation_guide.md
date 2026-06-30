# Usage and Implementation Guide

## 1. Purpose of this guide

This guide walks through using, configuring, building, flashing, testing, validating, and safely adapting EdgeGuard ESP32 on your own low-voltage prototype. It is written for first-time ESP32 users who need a chronological path and for reviewers who want the setup details tied back to the firmware and repository checks.

## 2. What the project does

EdgeGuard is an ESP32 smart-room node that reads a DHT11 temperature/humidity module, an HC-SR04 ultrasonic distance module, and a digital LDR module. The firmware uses those readings to maintain a shared system state, make local relay decisions, serve an onboard dashboard, and expose a small HTTP API.

The firmware is local-first. It can join a configured station Wi-Fi network, or it can start a fallback access point when station credentials are placeholders, absent, or unavailable. It does not implement cloud services, MQTT, OTA updates, mobile app provisioning, or built-in authentication.

## 3. Hardware required

The documented prototype uses only these verified parts:

- ESP32 DevKit board matching the PlatformIO `esp32doit-devkit-v1` target.
- DHT11 temperature/humidity module.
- HC-SR04 ultrasonic distance module.
- Digital LDR module with a digital output pin.
- Two low-voltage relay channels.
- Green and red LEDs with suitable resistors or LED modules.
- Jumper wires, breadboard or equivalent low-voltage prototyping hardware, and a stable low-voltage DC supply.

## 4. Safety boundaries

Use this project for low-voltage DC prototyping only. Do not connect mains or other high-voltage loads to a breadboard, exposed relay module, or unfinished prototype.

Important electrical boundaries:

- Keep all module grounds common with ESP32 ground.
- Protect ESP32 GPIO inputs from 5 V signals. The HC-SR04 ECHO signal must be shifted or divided to 3.3 V before GPIO 18.
- Validate relay polarity with a safe low-voltage load before relying on relay state labels.
- Use relay outputs only within the safe low-voltage limits of your relay module and wiring.
- Disconnect external low-voltage module power while the board is unpowered if upload stability is affected by the attached circuit.

## 5. Repository layout

Start with these files in this order:

| Path | Purpose |
| --- | --- |
| `README.md` | High-level overview, quick start, routes, hardware summary, and links. |
| `docs/hardware.md` | Hardware notes and pin map. |
| `hardware/wiring_table.md` | Concise wiring reference. |
| `firmware/EdgeGuard_ESP32/EdgeGuard_ESP32.ino` | Arduino sketch entry point. |
| `firmware/EdgeGuard_ESP32/config.h` | Pins, thresholds, timings, task constants, and polarity settings. |
| `docs/api.md` | Full HTTP route and status-field reference. |
| `docs/development.md` | Short development workflow reference. |
| `docs/testing_plan.md` | Automated and manual validation checklist. |
| `tools/verify_repo.py` | Repository integrity, local link, content guardrail, and secrets checks. |

## 6. Firmware architecture overview

The firmware keeps the Arduino sketch and sibling modules in `firmware/EdgeGuard_ESP32/` so the same source tree works in Arduino IDE and PlatformIO.

| Module | Role |
| --- | --- |
| `EdgeGuard_ESP32.ino` | Initializes Serial, diagnostics, pins, app state, sensors, Wi-Fi, web routes, and tasks. |
| `config.h` | Defines GPIOs, relay/LDR polarity, sensor thresholds, stale timeouts, task periods, stack sizes, and event log size. |
| `types.h` | Defines shared `Mode`, `State`, `SensorSnapshot`, and `SystemSnapshot` types. |
| `app_state.*` | Stores mutex-protected sensor/system snapshots and a fixed-size event log. |
| `sensors.*` | Reads DHT11, HC-SR04, and LDR inputs and records sensor health. |
| `control_logic.*` | Computes pure control decisions from snapshots and small retained memory. |
| `control.*` | Applies relay GPIO outputs, integrates diagnostics faults, and updates shared system state. |
| `diagnostics.*` | Tracks sensor failure counters, task heartbeats, heap, reset reason, Wi-Fi state, and watchdog state. |
| `wifi_manager.*` | Attempts station Wi-Fi and starts fallback AP when needed. |
| `web_routes.*` | Serves `/`, `/api/status`, `/api/logs`, mode routes, and relay routes. |
| `tasks.*` | Creates and runs sensor, control, web, and heartbeat FreeRTOS tasks. |
| `dashboard.h` | Embeds the dashboard HTML, CSS, and JavaScript served from `/`. |

## 7. Wiring overview

Verified firmware pin map:

| Function | ESP32 GPIO | Notes |
| --- | ---: | --- |
| DHT11 DATA | 4 | Temperature and humidity data. |
| HC-SR04 TRIG | 5 | ESP32 output. |
| HC-SR04 ECHO | 18 | Use a voltage divider or level shifter to protect the 3.3 V input. |
| LDR DO | 34 | Digital input only. |
| Relay 1 | 26 | Active-low by default. |
| Relay 2 | 27 | Active-low by default. |
| Green LED | 23 | Normal/status heartbeat. |
| Red LED | 22 | Alert/fault indication. |

The firmware defaults are `RELAY_ACTIVE_LOW = true` and `LDR_DARK_WHEN_HIGH = true`. See [Hardware Guide](hardware.md) and the [Wiring Table](../hardware/wiring_table.md) before wiring.

## 8. Software prerequisites

You can use either PlatformIO or Arduino IDE.

For the repository validation and PlatformIO workflow, install Python and then install the Python dependencies from `requirements.txt`:

```bash
python -m pip install -r requirements.txt
```

`requirements.txt` installs PlatformIO Core. `platformio.ini` defines the ESP32 Arduino environment and the host-side native test environment.

For Arduino IDE, install ESP32 board support and compatible DHT sensor dependencies. The repository does not pin Arduino IDE library versions, so use the library manager versions compatible with the included Arduino code.

## 9. Wi-Fi configuration

Station Wi-Fi is optional.

1. Copy the example file only if you want station Wi-Fi:

   ```bash
   cp firmware/EdgeGuard_ESP32/secrets.h.example firmware/EdgeGuard_ESP32/secrets.h
   ```

2. Edit only your local `secrets.h`:

   ```cpp
   #define WIFI_SSID "YOUR_WIFI_NAME"
   #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
   ```

3. Do not commit `firmware/EdgeGuard_ESP32/secrets.h`.

If `secrets.h` is absent, the firmware includes `secrets.h.example`. If the SSID is empty or remains `YOUR_WIFI_NAME`, station connection is skipped and fallback AP mode starts. If a non-placeholder SSID is configured but connection does not complete within the firmware connection window, fallback AP mode also starts.

Fallback AP details implemented in the firmware:

- SSID: `EdgeGuard-ESP32`
- Password: `edgeguard123`
- Dashboard address: the firmware prints `WiFi.softAPIP()` to Serial after AP startup; typical ESP32 AP configurations use `192.168.4.1`, but follow the serial log printed by your device.

## 10. PlatformIO workflow

Install dependencies, build, upload, and monitor from the repository root:

```bash
python -m pip install -r requirements.txt
pio run -e esp32doit-devkit-v1
pio run -e esp32doit-devkit-v1 -t upload
pio device monitor -b 115200
```

The ESP32 environment is `esp32doit-devkit-v1`, uses board `esp32doit-devkit-v1`, framework `arduino`, monitor speed `115200`, upload speed `115200`, and DHT dependencies declared in `platformio.ini`.

## 11. Arduino IDE workflow

1. Open `firmware/EdgeGuard_ESP32/EdgeGuard_ESP32.ino`.
2. Keep the sibling `.h` and `.cpp` files in the same sketch folder.
3. Install ESP32 board support.
4. Install DHT sensor dependencies compatible with the firmware includes.
5. Select an ESP32 DevKit target.
6. Upload the sketch.
7. Open Serial Monitor at `115200` baud.

## 12. First boot validation

On boot, the firmware initializes diagnostics, pins, shared state, sensors, Wi-Fi, web routes, and FreeRTOS tasks. In Serial Monitor, check for:

- Wi-Fi station connection and a printed dashboard URL, or fallback AP startup details.
- Sensor telemetry lines showing DHT status, distance status, and `DARK` or `BRIGHT` light state.
- Event log lines such as boot, web server, task creation, mode changes, state changes, and relay changes.

Open the dashboard using the printed station IP, or connect to the fallback AP and open the AP dashboard IP printed by the firmware.

## 13. Dashboard usage

The dashboard at `GET /` displays mode, state, temperature, humidity, distance, light state, occupancy, relay states, fault/alert text, uptime, Wi-Fi mode, IP address, heap, optional station RSSI, and recent event log entries.

Dashboard controls call the actual HTTP routes for AUTO, MANUAL, AWAY, Relay 1 ON/OFF, and Relay 2 ON/OFF. Relay button commands switch the system to MANUAL mode through the firmware command helper.

## 14. HTTP API usage

The implemented routes are:

| Method | Route |
| --- | --- |
| GET | `/` |
| GET | `/api/status` |
| GET | `/api/logs` |
| POST | `/api/mode/auto` |
| POST | `/api/mode/manual` |
| POST | `/api/mode/away` |
| POST | `/api/relay1/on` |
| POST | `/api/relay1/off` |
| POST | `/api/relay2/on` |
| POST | `/api/relay2/off` |

Examples, replacing the host with your printed IP if needed:

```bash
curl http://192.168.4.1/api/status
curl http://192.168.4.1/api/logs
curl -X POST http://192.168.4.1/api/mode/auto
curl -X POST http://192.168.4.1/api/mode/away
curl -X POST http://192.168.4.1/api/relay1/on
curl -X POST http://192.168.4.1/api/relay2/off
```

Normal command responses are `{"ok":true}`. See [HTTP API](api.md) for status fields and response notes.

## 15. Operating modes

- **AUTO:** Relay 1 turns on only when the interpreted light state is dark and occupancy is held. Relay 2 follows the temperature alert latch. Temperature alert latches at `TEMP_ALERT_ON_C` and clears at `TEMP_ALERT_OFF_C` when DHT readings are valid.
- **MANUAL:** Pure control logic preserves the current relay states and sets state to `MANUAL_OVERRIDE`. Relay command routes switch to MANUAL and directly apply the requested relay state.
- **AWAY:** Relay 1 remains off. Relay 2 turns on only for instant valid occupancy, not the held occupancy value. Instant occupancy sets state to `ALERT`; otherwise the state is `AUTO_MONITORING`.
- **FAULT:** Stale sensor snapshots force both relays off with fault reason `Sensor snapshot stale`. Repeated DHT11 or HC-SR04 failures integrated by `control.cpp` also force both relays off and set a fault reason based on the failing sensor path.

Occupancy is instant when `distance_ok` is true and the measured distance is greater than zero and less than or equal to `OCCUPIED_DISTANCE_CM`. Held occupancy remains true for less than `UNOCCUPIED_TIMEOUT_MS` after the last instant occupancy detection.

## 16. Sensor and relay validation

Use only safe low-voltage validation.

1. **DHT11:** Confirm `temperature_c`, `humidity`, and `dht_ok` in `/api/status`. A failed DHT read appears as `null` values and `dht_ok:false`.
2. **HC-SR04:** Move a target closer and farther from the sensor. Confirm `distance_cm`, `distance_ok`, and occupancy changes. No echo appears as `distance_cm:null` and `distance_ok:false`.
3. **LDR:** Change lighting at the module and confirm `light_is_dark` and dashboard `DARK`/`BRIGHT`. If the module is inverted, change only `LDR_DARK_WHEN_HIGH` after verifying the electrical output.
4. **Relay 1:** In MANUAL mode, use `/api/relay1/on` and `/api/relay1/off` with a safe low-voltage load or meter.
5. **Relay 2:** In MANUAL mode, use `/api/relay2/on` and `/api/relay2/off` with a safe low-voltage load or meter.

## 17. Running tests

Run these checks from the repository root:

```bash
python tools/verify_repo.py
pio test -e native
pio run -e esp32doit-devkit-v1
```

What they prove:

- `python tools/verify_repo.py` checks required files, forbidden duplicate secret/example paths, possible committed Wi-Fi credentials, text guardrails, and local README/docs links.
- `pio test -e native` builds and runs host-side Unity tests for `computeControlDecision(...)` using lightweight Arduino/FreeRTOS stubs.
- `pio run -e esp32doit-devkit-v1` confirms the firmware compiles for the ESP32 Arduino PlatformIO target.

What they do not prove: physical wiring correctness, sensor calibration, relay board polarity, Wi-Fi reachability in your location, or hardware upload success unless you actually run the upload command with a connected board.

## 18. Common troubleshooting path

Start with these checks:

1. Confirm USB cable and serial port stability.
2. Confirm Serial Monitor is set to `115200`.
3. Confirm `secrets.h` is absent, placeholder-based, or correctly configured.
4. Confirm fallback AP SSID and password from the serial log.
5. Check `/api/status` before debugging the dashboard UI.
6. Check pin wiring against `config.h`, [Hardware Guide](hardware.md), and [Wiring Table](../hardware/wiring_table.md).

See [Troubleshooting](troubleshooting.md) for issue-specific steps.

## 19. Customization points

Safe code-level customization points are centralized in `firmware/EdgeGuard_ESP32/config.h`:

- GPIO pin constants, if your wiring intentionally changes.
- `RELAY_ACTIVE_LOW`, if your relay module polarity is different.
- `LDR_DARK_WHEN_HIGH`, if your LDR module reports the opposite digital level.
- `OCCUPIED_DISTANCE_CM` and `UNOCCUPIED_TIMEOUT_MS` for occupancy behavior.
- `TEMP_ALERT_ON_C` and `TEMP_ALERT_OFF_C` for temperature hysteresis.
- `SENSOR_STALE_TIMEOUT_MS`, task periods, stack sizes, and event log size for timing and resource tuning.

Dashboard text or layout can be changed in `dashboard.h` if the route behavior and API field usage remain compatible. After behavior changes, update README/docs and tests before considering the change complete.

## 20. Implementation checklist

- [ ] Read the safety boundaries and hardware pin map.
- [ ] Clone or open the repository.
- [ ] Choose PlatformIO or Arduino IDE.
- [ ] Install required tooling and DHT dependencies for your chosen workflow.
- [ ] Copy `secrets.h.example` to `secrets.h` only if station Wi-Fi is needed.
- [ ] Wire the low-voltage prototype with common ground and protected HC-SR04 ECHO.
- [ ] Build the firmware.
- [ ] Upload to the ESP32.
- [ ] Open Serial Monitor at `115200`.
- [ ] Open the dashboard from the printed station IP or fallback AP IP.
- [ ] Validate DHT11, HC-SR04, LDR, Relay 1, and Relay 2 using safe low-voltage checks.
- [ ] Run `python tools/verify_repo.py`, `pio test -e native`, and `pio run -e esp32doit-devkit-v1` before committing documentation or behavior changes.
