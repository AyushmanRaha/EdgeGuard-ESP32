# EdgeGuard wiring table

| Module | Signal | ESP32 GPIO | Notes |
|---|---|---:|---|
| DHT11 | DATA | 4 | Keep original wiring. |
| HC-SR04 | TRIG | 5 | ESP32 output to sensor trigger. |
| HC-SR04 | ECHO | 18 | Must pass through voltage divider. |
| LDR digital module | DO | 34 | Input only pin is fine for digital output. |
| Relay channel 1 | IN1 | 26 | Low-voltage LED load only. |
| Relay channel 2 | IN2 | 27 | Low-voltage LED load only. |
| Green LED | Anode/control | 23 | Heartbeat. |
| Red LED | Anode/control | 22 | Alert/fault. |

All modules must share a common GND with the ESP32.

## HC-SR04 Echo divider

```text
HC-SR04 Echo (5 V) --- R1 ---+--- ESP32 GPIO18
                             |
                             R2
                             |
                            GND
```

Use R1 = 2.2 kΩ and R2 = 2.2 kΩ for about 2.5 V at GPIO18, which is safely read as HIGH by the ESP32. If you want closer to 3.0 V, use about R1 = 2.2 kΩ and R2 = 3.2 kΩ. A practical build can approximate R2 = 3.2 kΩ by wiring 2.2 kΩ + 1 kΩ in series on the lower side.

## Relay safety
Relays in this project switch only low-voltage LED loads. Do not use this project as AC mains wiring guidance.
