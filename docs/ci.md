# CI

The GitHub Actions workflow at `.github/workflows/ci.yml` runs on pushes to `main`, pull requests to `main`, and manual dispatch.

## Firmware job

- Checks out the repo.
- Sets up Python.
- Caches pip and PlatformIO package cache directories.
- Installs the pinned PlatformIO Core dependency from `requirements.txt`.
- Runs `pio run` without requiring ESP32 hardware or Wi-Fi credentials.
- Uploads firmware binary, ELF, and map artifacts when generated.

## Documentation check job

- Verifies `README.md` and required docs exist.
- Verifies no `secrets.h` file is committed.
- Performs a lightweight scan for obvious real-secret placeholder mistakes.

The workflow uses `contents: read` permissions and concurrency cancellation to avoid redundant runs on the same branch.
