# EdgeGuard-ESP32

EdgeGuard-ESP32 is a local ESP32 smart-room firmware project for monitoring temperature, humidity, distance-based occupancy, and room light state while controlling two low-voltage relay-driven LED loads. It keeps the original GPIO map, AUTO/MANUAL/AWAY behavior, ALERT/FAULT semantics, local dashboard, REST-style JSON API, event ring buffer, and FreeRTOS-style task separation.

## What it does

- Reads a DHT11 temperature/humidity module.
- Reads an HC-SR04 ultrasonic sensor with a bounded echo timeout.
- Reads a digital LDR light/dark module.
- Controls two active-low relay outputs for low-voltage LED demonstration loads.
- Serves a dependency-free dashboard from the ESP32 at `GET /`.
- Exposes JSON status/log endpoints and POST command routes for local clients.
- Falls back to a local access point if station Wi-Fi is not configured or unavailable.

## Features

- AUTO, MANUAL, and AWAY operating modes.
- BOOT, CALIBRATING, AUTO_MONITORING, MANUAL_OVERRIDE, ALERT, and FAULT states.
- Temperature alert hysteresis using `TEMP_ALERT_ON_C` and `TEMP_ALERT_OFF_C`.
- Occupancy hold behavior using `OCCUPIED_DISTANCE_CM` and `UNOCCUPIED_TIMEOUT_MS`.
- Mutex-protected shared sensor, system, and event-log data.
- Responsive local dashboard with cards, badges, relay controls, device details, fault/alert visibility, and event log.
- PlatformIO build support and GitHub Actions CI.
- Arduino IDE path remains supported.

## Safety note

This repository is for a low-voltage LED relay demonstration only. Do not connect mains AC or high-voltage loads to this project. Use common ground, suitable sensor voltage levels, and an HC-SR04 echo voltage divider for the ESP32 input.

## Hardware list

- ESP32 DevKit board
- DHT11 module
- HC-SR04 ultrasonic sensor
- Digital LDR module
- 2-channel relay module
- Low-voltage LEDs and resistors
- Breadboard and jumper wires
- 2.2 kΩ resistors for the HC-SR04 echo divider

## Pin map

| ESP32 pin | Connected module pin | Purpose | Notes |
|---|---|---|---|
| GPIO 4 | DHT11 DATA | Temperature/humidity input | DHT11 library input |
| GPIO 5 | HC-SR04 TRIG | Ultrasonic trigger | Output pulse |
| GPIO 18 | HC-SR04 ECHO through divider | Ultrasonic echo input | Do not connect 5 V echo directly |
| GPIO 34 | LDR DO | Digital light/dark input | Input-only ESP32 pin |
| GPIO 26 | Relay IN1 | Low-voltage room LED load | Active-low by default |
| GPIO 27 | Relay IN2 | Low-voltage alert LED load | Active-low by default |
| GPIO 23 | Green LED | Normal heartbeat | GPIO output |
| GPIO 22 | Red LED | Alert/fault heartbeat | GPIO output |

## Operating modes

- `AUTO`: Relay 1 follows dark-and-occupied logic. Relay 2 follows temperature alert status.
- `MANUAL`: Relay buttons immediately select relay states and the system reports `MANUAL_OVERRIDE`.
- `AWAY`: Relay 1 stays off. Movement/occupancy triggers `ALERT` and Relay 2; otherwise Relay 2 stays off.

## System states

- `BOOT`: Initial startup state.
- `CALIBRATING`: Startup state before the control task begins normal monitoring.
- `AUTO_MONITORING`: Normal automatic monitoring.
- `MANUAL_OVERRIDE`: Manual relay/mode control is active.
- `ALERT`: Temperature alert in AUTO or occupancy alert in AWAY.
- `FAULT`: Repeated DHT11 or HC-SR04 failures; relays are forced off.

## Dashboard usage

- Station mode: configure `secrets.h`, flash the ESP32, then open the IP printed in Serial Monitor.
- Fallback AP mode: if Wi-Fi is not configured or connection times out, connect to SSID `EdgeGuard-ESP32` with password `edgeguard123` and open `http://192.168.4.1/`.
- Dashboard route: `GET /`.
- The dashboard is fully local and uses no CDN, external CSS, external JS, remote fonts, or cloud telemetry.

## API

All existing routes are preserved:

| Method | Route | Description |
|---|---|---|
| GET | `/` | Local dashboard |
| GET | `/api/status` | Live JSON status |
| GET | `/api/logs` | Event log array |
| POST | `/api/mode/auto` | Select AUTO mode |
| POST | `/api/mode/manual` | Select MANUAL mode |
| POST | `/api/mode/away` | Select AWAY mode |
| POST | `/api/relay1/on` | Manual Relay 1 ON |
| POST | `/api/relay1/off` | Manual Relay 1 OFF |
| POST | `/api/relay2/on` | Manual Relay 2 ON |
| POST | `/api/relay2/off` | Manual Relay 2 OFF |

Example `/api/status` response:

```json
{
  "temperature_c": 27.4,
  "humidity": 58.0,
  "dht_ok": true,
  "distance_cm": 32,
  "distance_ok": true,
  "light_is_dark": true,
  "occupied": true,
  "mode": "AUTO",
  "state": "AUTO_MONITORING",
  "relay1": true,
  "relay2": false,
  "temperature_alert": false,
  "fault_reason": "",
  "uptime_s": 120,
  "wifi_mode": "STA",
  "ip": "192.168.1.50",
  "heap_free": 210000,
  "rssi": -55
}
```

See [docs/api.md](docs/api.md) for field details.

## Build instructions

### Arduino IDE

1. Install ESP32 board support in Arduino IDE.
2. Install the `DHT sensor library` and `Adafruit Unified Sensor` libraries.
3. Open `firmware/EdgeGuard_ESP32/EdgeGuard_ESP32.ino`.
4. Copy `firmware/EdgeGuard_ESP32/secrets.h.example` to `firmware/EdgeGuard_ESP32/secrets.h` and edit Wi-Fi credentials, or leave the example placeholders to use fallback AP mode.
5. Select an ESP32 DevKit board, build, and upload.

Recommended Arduino IDE upload settings for reliability:

- Board: ESP32 DevKit / DOIT ESP32 DevKit V1, or the exact matching board profile for your module.
- Port: the detected ESP32 serial port, for example `/dev/cu.usbserial-0001` on macOS.
- Upload Speed: start with `115200`. If this is stable, `230400` may also work. Avoid high upload speeds when the board is on a breadboard, connected through a hub, or connected to relay/sensor wiring.
- Serial Monitor: `115200` baud.
- Close Serial Monitor before uploading.

### PlatformIO

```bash
pio run
pio run --target upload
pio device monitor -b 115200
```

The `platformio.ini` target uses `esp32doit-devkit-v1`, Arduino framework, the DHT dependencies, and a conservative `upload_speed = 115200` for reliable flashing.

## Configuration

- `firmware/EdgeGuard_ESP32/config.h`: GPIO map, relay polarity, LDR polarity, thresholds, task periods, and event-log size.
- `firmware/EdgeGuard_ESP32/secrets.h.example`: placeholder Wi-Fi credentials.
- `firmware/EdgeGuard_ESP32/secrets.h`: local untracked credentials file. It is ignored by Git.
- `firmware/EdgeGuard_ESP32/secrets.example.h`: alternate example name for users who prefer that convention.

## CI

GitHub Actions builds firmware with PlatformIO, checks required documentation files, rejects committed `secrets.h`, caches pip and PlatformIO package caches, and uploads firmware artifacts when available. CI does not need physical ESP32 hardware or Wi-Fi credentials.

## Troubleshooting

### Upload fails with `A fatal error occurred: The chip stopped responding`

If the compile succeeds, the reported sketch size is within flash limits, esptool connects, bootloader/partition data verifies, and the upload fails while writing the main `.bin` at `0x00010000`, this is not a sketch-size problem and the application code is not the immediate cause. The ESP32 entered flashing mode, then the serial flashing session dropped mid-transfer. Common causes are an unstable USB cable, weak USB power, a USB hub, high upload speed, loose breadboard wiring, connected relay/sensor modules drawing power during flashing, or the BOOT/EN auto-reset circuit behaving unreliably.

A larger firmware image can make an already-unstable link more likely to fail because flashing takes longer, but the image is still valid when the Arduino IDE reports it is within the available flash limit.

Try this recovery sequence:

1. Unplug the ESP32.
2. Close Serial Monitor and any other program using the same serial port.
3. Use a short data-capable USB cable and connect directly to the Mac, not through a hub.
4. Keep the project connected only to low-voltage wiring. Do not use mains AC.
5. In Arduino IDE, set Upload Speed to `115200`.
6. If upload sticks at `Connecting...`, press and hold `BOOT` until writing starts, then release it.
7. If upload drops mid-write, retry at `115200`; if it still fails, disconnect external relay/sensor VCC wires while the board is unpowered, upload to the bare ESP32, then reconnect the low-voltage circuit with power removed.
8. After upload, press `EN`/reset once and open Serial Monitor at `115200`.

### Sensor and dashboard issues

- DHT error: check wiring, power, the DHT data pin on GPIO 4, and library installation.
- HC-SR04 no echo: confirm trigger GPIO 5, echo GPIO 18, common ground, sensor power, and the echo divider.
- Relay inverted behavior: most modules are active-low; only change `RELAY_ACTIVE_LOW` in `config.h` if your module behaves opposite.
- Dashboard not opening: check the Serial Monitor IP, phone/laptop network, and fallback AP URL `http://192.168.4.1/`.
- AP mode fallback: expected when station Wi-Fi credentials are placeholders or the network cannot be reached within the bounded connect window.
- LDR inverted behavior: change `LDR_DARK_WHEN_HIGH` in `config.h` if dark/bright reads opposite.

## Architecture overview

`SensorTask` reads sensors and publishes a short shared snapshot. `ControlTask` consumes that snapshot, applies occupancy, temperature hysteresis, mode, fault, and relay decisions, then publishes system state. `WebTask` keeps `server.handleClient()` responsive and serves dashboard/API data from copied snapshots. `HeartbeatTask` drives the status LEDs.

## Repository structure

```text
firmware/EdgeGuard_ESP32/   Arduino sketch, config, dashboard, secrets examples
docs/                       Architecture, API, hardware, development, CI, troubleshooting
hardware/                   Wiring table and hardware notes
.github/workflows/ci.yml    PlatformIO CI workflow
platformio.ini              Reproducible PlatformIO build target
```

## Limitations and future improvements

- DHT11 accuracy and update rate are limited compared with newer sensors.
- Settings are compile-time constants; NVS persistence could be added later.
- No OTA update, authentication, MQTT, or cloud telemetry is included.
- Unit tests for pure helper/state-machine logic would be useful future work.

## License

This repository includes an explicit [LICENSE](LICENSE) file. Review it before reuse or redistribution.
