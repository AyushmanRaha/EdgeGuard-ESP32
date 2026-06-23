# Failure Modes and Effects

| Failure mode | Cause | Detection | Behavior | Safe output | Recovery |
|---|---|---|---|---|---|
| DHT11 read failure | Sensor read returns invalid data | Consecutive DHT fail counter | WARN logs, then FAULT at threshold | Relays off in FAULT | Restore readings and reset fault |
| HC-SR04 timeout | No echo pulse | Consecutive ultrasonic fail counter | WARN logs, then FAULT at threshold | Relays off in FAULT | Restore readings and reset fault |
| Both sensor failures | DHT and ultrasonic fail together | Both counters reach threshold | Combined fault code | Relays off | Restore readings and reset fault |
| SensorTask stale | Task heartbeat timeout | ControlTask watchdog check | FAULT event | Relays off | Reset fault after task recovery |
| Wi-Fi unavailable | No station connection | Connection timeout | Fallback AP starts | Control remains local | Connect to AP or restore Wi-Fi |
| Wrong API token | Missing or invalid token | POST auth check | 401 JSON and SECURITY log | No control change | Send correct token |
| Relay active-low mismatch | Module polarity differs | Hardware observation | Relay behavior inverted | Set relays off before changing config | Adjust `RELAY_ACTIVE_LOW` |
| LDR inversion mismatch | Module digital output polarity differs | Dashboard shows reversed light state | Light logic inverted | No hardware change | Adjust `LDR_DARK_WHEN_HIGH` |
