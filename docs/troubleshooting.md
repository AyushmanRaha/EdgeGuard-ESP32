# Troubleshooting

## Upload fails with `The chip stopped responding`

This usually indicates an unstable serial flashing session, not a control-logic problem.

Checklist:

1. Close Serial Monitor and other serial terminals.
2. Use a short data-capable USB cable directly connected to the computer.
3. Keep upload speed at `115200`.
4. Hold `BOOT` while upload connects if auto-reset is unreliable.
5. Disconnect external low-voltage module power while the board is unpowered, upload, then reconnect safely.
6. Press `EN` after upload and open Serial Monitor at `115200`.

## Native tests failing

Run `pio test -e native`. If compilation fails, check that the native environment includes `test/native_stubs` and only builds `control_logic.cpp`. If an assertion fails, inspect the named test for the expected mode, state, or boundary behavior.

## CI failing

Run the failing CI command locally: `python tools/verify_repo.py`, `pio test -e native`, or `pio run -e esp32doit-devkit-v1`. Verification messages identify missing files, broken links, content guardrail hits, or possible secrets.

## Dashboard not refreshing

Confirm the browser is on the same network or fallback AP, reload `/`, and check `/api/status` directly. The firmware sends no-store headers, so persistent stale data usually indicates Wi-Fi loss, a stopped web task, or a device/network mismatch.

## Stale sensor fault

`FAULT` with `Sensor snapshot stale` means the control loop did not receive a fresh sensor timestamp within `SENSOR_STALE_TIMEOUT_MS`. Check sensor task heartbeats, sensor wiring, and serial logs.

## Inverted LDR behavior

If the dashboard reports dark in bright light, flip only `LDR_DARK_WHEN_HIGH` in `config.h` after confirming the module output with a meter or serial logs.

## Inverted relay behavior

Most modules are active-low. If a relay energizes opposite to the dashboard state, validate with a safe low-voltage load and change only `RELAY_ACTIVE_LOW` if the module requires it.

## Wi-Fi fallback

If station connection fails or credentials are placeholders, connect to AP `EdgeGuard-ESP32` with password `edgeguard123`, then open `http://192.168.4.1/`.

## DHT errors

Confirm DHT11 DATA is GPIO 4, power and ground are stable, and dependencies are installed. Repeated read failures are expected to drive conservative fault behavior.

## Ultrasonic no echo

Confirm TRIG is GPIO 5, ECHO is GPIO 18 through a divider, grounds are common, and the target is within range. Echo timeouts are bounded by firmware configuration.

## Serial monitor issues

Use `115200` baud. Close duplicate serial sessions before upload. If logs are unreadable, reset the board after opening the monitor.
