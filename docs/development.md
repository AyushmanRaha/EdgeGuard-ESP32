# Development Guide

## Arduino IDE

Open `firmware/EdgeGuard_ESP32/EdgeGuard_ESP32.ino` in Arduino IDE. Install ESP32 board support, `DHT sensor library`, and `Adafruit Unified Sensor`. Copy `secrets.h.example` to `secrets.h` for station Wi-Fi credentials or leave placeholders for fallback AP mode.

## PlatformIO

Use the repo-level `platformio.ini`:

```bash
pio run
pio run --target upload
pio device monitor -b 115200
```

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
