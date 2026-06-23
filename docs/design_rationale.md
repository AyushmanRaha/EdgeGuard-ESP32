# Design Rationale

## Local-first operation
The firmware serves a local dashboard and API directly from the ESP32. This keeps the system useful without cloud services and avoids additional network dependencies.

## FreeRTOS task split
Sensor reads, control decisions, web handling, and LED heartbeat run in separate tasks so slow web clients or sensor timing do not block the whole application. Shared data is exchanged through bounded snapshots protected by a mutex.

## Fail-safe relay behavior
Relays default off at boot and are forced off during repeated DHT11 or HC-SR04 failures. Control also enters `FAULT` and forces both relays off if the latest sensor snapshot becomes stale. This keeps output behavior conservative when environmental data is stale or unavailable.

## Hysteresis and occupancy hold
Temperature alert hysteresis avoids relay chatter around the threshold. Occupancy hold keeps short ultrasonic dropouts from immediately turning lights off.

## Wi-Fi fallback
Station mode is used when local credentials are provided. Otherwise the ESP32 starts a documented fallback AP so the dashboard remains reachable during setup.

## Limitations
The project intentionally avoids cloud telemetry, OTA updates, MQTT, authentication, and external dashboard CDNs. Future work can add deeper diagnostics, more isolated control logic tests, and enclosure-ready wiring diagrams while preserving the existing low-voltage safety assumptions.
