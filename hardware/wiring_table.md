# Wiring Table

| Device | Signal | ESP32 GPIO | Electrical note |
| --- | --- | ---: | --- |
| DHT11 | DATA | 4 | Temperature/humidity data with common ground. |
| HC-SR04 | TRIG | 5 | ESP32 output. |
| HC-SR04 | ECHO | 18 | Use a divider or level shifter from 5 V to 3.3 V. |
| LDR module | DO | 34 | Digital input; firmware assumes dark when high. |
| Relay module | IN1 | 26 | Active-low by default. |
| Relay module | IN2 | 27 | Active-low by default. |
| Green LED | Anode/control | 23 | Use a resistor or suitable LED module. |
| Red LED | Anode/control | 22 | Use a resistor or suitable LED module. |

All wiring examples assume low-voltage DC prototyping only. Keep grounds common, protect ESP32 inputs from 5 V signals, and validate relay polarity before connecting any load.
