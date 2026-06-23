# Wiring Table

This project is for low-voltage LED relay demonstrations only. Do not connect mains AC.

| ESP32 Pin | Connected Module Pin | Purpose | Notes |
|---|---|---|---|
| GPIO 4 | DHT11 DATA | Temperature/humidity input | DHT11 data line |
| GPIO 5 | HC-SR04 TRIG | Ultrasonic trigger | ESP32 output |
| GPIO 18 | HC-SR04 ECHO through divider | Ultrasonic echo input | Level shift from 5 V echo |
| GPIO 34 | LDR DO | Digital light/dark input | Input-only ESP32 pin |
| GPIO 26 | Relay IN1 | Room light LED load | Active-low by default |
| GPIO 27 | Relay IN2 | Alert LED load | Active-low by default |
| GPIO 23 | Green LED | Normal heartbeat | GPIO output |
| GPIO 22 | Red LED | Alert/fault indication | GPIO output |
| 3V3 | DHT11, LDR | Sensor power | Use module-compatible voltage |
| VIN/5V | HC-SR04, relay module | Module power | Confirm module requirements |
| GND | All modules | Common ground | Required for reliable readings |

## HC-SR04 echo divider

HC-SR04 ECHO is typically 5 V and must not be connected directly to the ESP32 input. Use a voltage divider or suitable level shifter before GPIO 18.

```text
Echo ---- 2.2 kΩ ---- GPIO18 ---- 2.2 kΩ ---- GND
```
