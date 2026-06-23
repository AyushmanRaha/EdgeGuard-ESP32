# Development Guide

## Arduino IDE workflow

1. Open `firmware/EdgeGuard_ESP32/EdgeGuard_ESP32.ino`.
2. Keep all sibling `.h` and `.cpp` modules in the same sketch directory.
3. Install ESP32 board support plus the DHT sensor dependencies.
4. Select an ESP32 DevKit target and Serial Monitor speed `115200`.
5. Upload, then confirm boot logs and dashboard IP.

## PlatformIO workflow

```bash
python -m pip install -r requirements.txt
pio run -e esp32doit-devkit-v1
pio run -e esp32doit-devkit-v1 -t upload
pio device monitor -b 115200
```

## Testing workflow

```bash
python tools/verify_repo.py
pio test -e native
pio run -e esp32doit-devkit-v1
```

Native tests run on the host and compile only the pure control logic plus lightweight Arduino/FreeRTOS stubs.

## Secrets handling

Copy `firmware/EdgeGuard_ESP32/secrets.h.example` to `firmware/EdgeGuard_ESP32/secrets.h` for local station credentials. The local file is ignored and must not be committed. Placeholder values keep fallback AP behavior available.

## Module conventions

| Area | Convention |
| --- | --- |
| Pins and thresholds | Keep constants in `config.h`. |
| Shared types | Keep API-visible names stable in `types.h` and JSON builders. |
| Shared state | Copy under mutex, then release before slow work. |
| Control decisions | Prefer pure helpers that can be tested without hardware. |
| Dashboard | Keep assets embedded and local; avoid external runtime dependencies. |
| Safety | Preserve low-voltage DC assumptions and fail-safe relay defaults. |

## Safe-change checklist

- Preserve documented routes and JSON field names.
- Preserve pin map, relay polarity, and LDR polarity unless hardware docs are intentionally updated.
- Run repository verification, native tests, and the ESP32 firmware build.
- Update README and relevant docs for behavior, build, route, or safety changes.
- Confirm `secrets.h`, build artifacts, and local logs are not staged.
