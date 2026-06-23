# Development Guide

## Arduino IDE

Open `firmware/EdgeGuard_ESP32/EdgeGuard_ESP32.ino` in Arduino IDE. Install ESP32 board support, `DHT sensor library`, and `Adafruit Unified Sensor`. Copy `secrets.h.example` to `secrets.h` for station Wi-Fi credentials or leave placeholders for fallback AP mode.

Recommended Arduino IDE upload settings:

- Board: ESP32 DevKit/DOIT ESP32 DevKit V1 or the matching board profile for your module.
- Port: the detected ESP32 serial port, for example `/dev/cu.usbserial-0001` on macOS.
- Upload Speed: `115200` for maximum reliability.
- Serial Monitor: `115200` baud.
- Close Serial Monitor before uploading.

Stable Arduino upload workflow: use a short data-capable USB cable, avoid USB hubs while flashing, start at `115200`, hold `BOOT` until writing starts if upload sticks at `Connecting...`, retry at `115200` if upload drops mid-write, and press `EN`/reset once after upload. If the serial link remains unstable, disconnect external low-voltage relay/sensor VCC wires while the board is unpowered, upload to the bare ESP32, then reconnect with power removed.

## PlatformIO

Use the repo-level `platformio.ini`:

```bash
pio run
pio run --target upload
pio device monitor -b 115200
```

The PlatformIO environment keeps `board = esp32doit-devkit-v1`, `framework = arduino`, `monitor_speed = 115200`, and `upload_speed = 115200`. The conservative upload speed prioritizes reliable flashing over maximum speed on CH340/CP210x USB-serial boards, USB hubs, and breadboarded low-voltage circuits. PlatformIO is optional; Arduino IDE users can use the settings above instead.

## Configuration

- Keep GPIO constants in `config.h` unchanged unless intentionally adapting hardware in a fork.
- `RELAY_ACTIVE_LOW` defaults to `true` for common low-cost relay modules.
- `LDR_DARK_WHEN_HIGH` can be flipped if the LDR module reports opposite polarity.
- `secrets.h` is ignored and must not be committed.

## Coding guidelines

- Preserve existing API routes and JSON fields.
- Avoid slow I/O while holding `gMutex`.
- Keep event logging bounded.
- Do not add cloud dependencies or external dashboard assets.
