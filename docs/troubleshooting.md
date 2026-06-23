# Troubleshooting

## Upload fails with `A fatal error occurred: The chip stopped responding`

This usually means compilation succeeded, the ESP32 entered flashing mode, and the serial flashing session dropped before the main firmware image finished writing. It does **not** mean the sketch logic directly caused the mid-upload failure.

Typical signs:

- Arduino IDE reports sketch size successfully and the image is within the available flash limit.
- esptool connects to the ESP32.
- Bootloader and partition images write and verify.
- Failure happens while writing the main application `.bin` at `0x00010000`, often around a partial percentage such as 61%.
- The log ends with `StopIteration`, `The chip stopped responding`, or `Failed uploading`.

A larger firmware image can make an unstable USB serial link more likely to fail because the write takes longer, but the image is still acceptable when the IDE reports it is within flash limits.

Most likely causes:

- Upload speed is too high for the USB-serial adapter or cable.
- USB cable is charge-only, too long, or unreliable.
- ESP32 is connected through a weak USB hub.
- Breadboard wiring is loose.
- Relay/sensor modules are drawing power from the ESP32 USB supply during flashing.
- The BOOT/EN auto-reset circuit is unreliable and needs manual `BOOT` help.
- Another app, such as Serial Monitor, is holding the serial port.

Recovery checklist:

1. Close Serial Monitor and any other serial terminal.
2. Use a short data-capable USB cable.
3. Plug directly into the Mac, not through a hub.
4. In Arduino IDE, set Upload Speed to `115200`.
5. Start upload. If it sticks at `Connecting...`, hold `BOOT` until writing starts, then release it.
6. If it drops mid-upload, unplug the ESP32, wait a few seconds, and try again at `115200`.
7. If still unstable, disconnect external relay/sensor VCC wires while the board is unpowered, upload to the bare ESP32, then reconnect the low-voltage circuit with power removed.
8. Keep the project low-voltage only; do not connect mains AC.
9. After upload, press `EN`/reset and open Serial Monitor at `115200`.

For PlatformIO, the repo uses a conservative `upload_speed = 115200` in `platformio.ini`.

## DHT error

- Confirm DHT11 data is on GPIO 4.
- Check sensor power and ground.
- Confirm Arduino/PlatformIO installed the DHT library and Adafruit Unified Sensor dependency.
- FAULT appears after repeated DHT failures by design.

## HC-SR04 no echo

- Confirm TRIG is GPIO 5 and ECHO is GPIO 18 through a divider.
- Confirm common ground.
- Verify the target is within range and the sensor is powered correctly.
- FAULT appears after repeated echo timeouts by design.

## Relay behavior inverted

Most relay modules are active-low. If your relay module behaves opposite, change only `RELAY_ACTIVE_LOW` in `config.h`.

## Dashboard not opening

- Check Serial Monitor at 115200 baud for the station IP.
- If station connection fails, connect to AP `EdgeGuard-ESP32` with password `edgeguard123` and open `http://192.168.4.1/`.
- Ensure your phone/laptop is on the same network as the ESP32.

## LDR inverted behavior

If the dashboard says DARK when the room is bright, flip `LDR_DARK_WHEN_HIGH` in `config.h`.
