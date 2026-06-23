# Troubleshooting

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
