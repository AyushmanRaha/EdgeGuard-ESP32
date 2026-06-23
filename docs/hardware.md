# Hardware Guide

This project is intended only for low-voltage LED relay demonstrations. Do not connect mains AC.

## Pin map

| ESP32 pin | Connected module pin | Purpose | Notes |
|---|---|---|---|
| GPIO 4 | DHT11 DATA | Temperature/humidity input | Keep the configured DHT11 type. |
| GPIO 5 | HC-SR04 TRIG | Ultrasonic trigger | ESP32 output. |
| GPIO 18 | HC-SR04 ECHO via divider | Ultrasonic echo input | Use a divider; HC-SR04 echo is typically 5 V. |
| GPIO 34 | LDR DO | Digital light input | Input-only pin. |
| GPIO 26 | Relay IN1 | Low-voltage room LED load | Active-low default. |
| GPIO 27 | Relay IN2 | Low-voltage alert LED load | Active-low default. |
| GPIO 23 | Green LED | Normal heartbeat | GPIO output. |
| GPIO 22 | Red LED | Alert/fault heartbeat | GPIO output. |

## Power and grounding

- Use common ground between ESP32, sensors, relay module, and LED supply.
- Power modules according to their board requirements.
- Keep relay loads low-voltage only.

## HC-SR04 echo divider

Do not connect HC-SR04 ECHO directly to ESP32 GPIO. Use the documented divider from `hardware/wiring_table.md` or another safe level-shifting method.
