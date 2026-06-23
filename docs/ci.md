# Continuous Integration

The workflow in `.github/workflows/ci.yml` validates the repository without physical hardware.

## Triggers and permissions

- Runs on pushes to `main`, pull requests to `main`, and manual dispatch.
- Uses `contents: read` permissions only.
- Cancels older runs for the same ref through workflow concurrency.

## Dependency and cache strategy

CI uses Python 3.12, installs dependencies from `requirements.txt`, and caches PlatformIO cache, package, and platform directories using keys derived from `platformio.ini` and `requirements.txt`.

## Stages

| Stage | Command or action | Purpose |
| --- | --- | --- |
| Repository verification | `python tools/verify_repo.py` | Required files, links, guardrails, and secrets checks. |
| Native tests | `pio test -e native` | Host-side Unity tests for pure control logic. |
| Firmware build | `pio run -e esp32doit-devkit-v1` | ESP32 Arduino build validation. |
| Artifact upload | upload action | Stores `firmware.bin`, `firmware.elf`, and `firmware.map` for 14 days. |

## Troubleshooting CI failures

- Verification failure: read the exact missing file, broken link, or content-policy message.
- Native test failure: run `pio test -e native` locally and inspect `test/test_control_logic/test_control_logic.cpp`.
- Firmware build failure: run `pio run -e esp32doit-devkit-v1` locally with verbose output if needed.
- Artifact failure: confirm the ESP32 build completed and `.pio/build/esp32doit-devkit-v1/` contains the expected files.
