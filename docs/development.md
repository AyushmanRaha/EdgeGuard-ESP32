# Development Guide

## Arduino IDE

Open `firmware/EdgeGuard_ESP32/EdgeGuard_ESP32.ino` in Arduino IDE. Install ESP32 board support, `DHT sensor library`, and `Adafruit Unified Sensor`. Copy `secrets.h.example` to `secrets.h` for station Wi-Fi credentials or leave placeholders for fallback AP mode.

Recommended Arduino IDE upload settings:

- Board: ESP32 DevKit/DOIT ESP32 DevKit V1 or the matching board profile for your module.
- Port: the detected ESP32 serial port, for example `/dev/cu.usbserial-0001` on macOS.
- Upload Speed: `115200` for maximum reliability.
- Serial Monitor: `115200` baud.

If upload fails while writing the main firmware image, close Serial Monitor, use a short data USB cable, connect directly to the Mac, retry at `115200`, and use the `BOOT` button if the board does not enter flashing mode reliably.

## PlatformIO

Use the repo-level `platformio.ini`:

```bash
pio run
pio run --target upload
pio device monitor -b 115200
```

The PlatformIO environment sets `upload_speed = 115200` to prioritize reliable flashing over maximum speed.

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
