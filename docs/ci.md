# CI

The workflow has three checks:

## host-tests
Installs CMake and build tools, runs host C++ tests, repository validation, and firmware structure checks.

## formatting-check
Installs clang-format and performs a dry-run check on firmware `.h` and `.cpp` files.

## arduino-compile
Installs Arduino CLI, installs the ESP32 platform and required libraries, creates a temporary compile-only `secrets.h`, and compiles `firmware/EdgeGuard_ESP32` for `esp32:esp32:esp32`. It does not upload to hardware.

Credentials are represented by placeholders during CI. Real local credentials belong only in ignored `firmware/EdgeGuard_ESP32/secrets.h`.
