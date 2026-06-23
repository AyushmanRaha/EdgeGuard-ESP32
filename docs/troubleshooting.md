# Troubleshooting

## ESP32 port not showing on Mac
Install the correct USB serial driver if your board needs one, try a data-capable USB cable, and check Arduino IDE port selection after reconnecting.

## Arduino compile fails
Install ESP32 board support plus `DHT sensor library` and `Adafruit Unified Sensor`. Confirm the sketch path is `firmware/EdgeGuard_ESP32/EdgeGuard_ESP32.ino`.

## Dashboard not opening
Check Serial Monitor for the STA IP address or fallback AP details. Join the fallback AP SSID and password configured in `secrets.h` if Wi-Fi credentials are missing or wrong.

## DHT11 reads null
Check DATA on GPIO4, power, ground, and any required pull-up. DHT11 is slow; wait between readings.

## HC-SR04 shows no echo
Check TRIG on GPIO5, Echo divider into GPIO18, 5 V sensor power, common ground, and object distance.

## LDR inverted
If DARK/BRIGHT appears backward, flip `LDR_DARK_WHEN_HIGH` in `config.h` without changing GPIO pins.

## Relay active-low behavior
Most relay modules are active-low. If behavior is reversed, change `RELAY_ACTIVE_LOW` in `config.h`.

## Wi-Fi fallback AP mode
If STA connection fails, the ESP32 starts the fallback AP configured in `secrets.h` and serves the dashboard from the AP IP printed in Serial Monitor.
