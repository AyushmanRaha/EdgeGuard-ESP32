# State Machine

```mermaid
stateDiagram-v2
  [*] --> BOOT
  BOOT --> CALIBRATING
  CALIBRATING --> AUTO_MONITORING
  AUTO_MONITORING --> ALERT: Temperature high
  ALERT --> AUTO_MONITORING: Temperature below hysteresis threshold
  AUTO_MONITORING --> MANUAL_OVERRIDE: Manual mode
  MANUAL_OVERRIDE --> AUTO_MONITORING: Auto mode
  AUTO_MONITORING --> ALERT: Away mode + nearby motion
  ALERT --> AUTO_MONITORING: Away clear
  AUTO_MONITORING --> FAULT: Repeated sensor failures
  MANUAL_OVERRIDE --> FAULT: Sensor fault priority
  ALERT --> FAULT: Sensor fault priority
  FAULT --> CALIBRATING: Manual fault reset
```

Priority order: software watchdog fault, repeated sensor faults, manual mode, away alert, temperature alert, normal automatic monitoring. Temperature alert uses hysteresis: latch at `TEMP_ALERT_ON_C`, clear at `TEMP_ALERT_OFF_C`. Occupancy is held until `UNOCCUPIED_TIMEOUT_MS` after the last nearby HC-SR04 reading.

Manual fault reset clears counters, clears watchdog state, turns relays off, and returns through calibration. FAULT always forces both relay outputs off.
