# Design Rationale

## Local-first design

The ESP32 serves the dashboard and API directly. This keeps basic monitoring and relay decisions available on a local network or fallback AP without external services.

## FreeRTOS task split

Sensor reads, control decisions, web handling, and LED heartbeat behavior have different timing needs. Separate tasks keep a slow client or sensor timeout from blocking the entire firmware.

## Fail-safe relay design

Relays default off, stale sensor snapshots enter `FAULT`, and fault paths force both relays off. This conservative behavior is appropriate for a prototype that makes decisions from environmental sensors.

## Hysteresis

Temperature uses separate alert-on and alert-off thresholds. This prevents rapid relay changes when the measured value moves around a single threshold.

## Occupancy hold

Ultrasonic readings can briefly drop out due to angle, surface, or timing. Occupancy hold keeps a recent valid detection active for a short window, reducing relay chatter.

## Pure function testability

`computeControlDecision(...)` isolates core decisions from GPIO and network code. Host-side tests can validate boundary behavior, faults, and mode handling quickly in CI.

## Fallback AP

Station Wi-Fi is convenient when credentials are available. The fallback AP keeps the dashboard reachable during setup, credential mistakes, or isolated lab use.

## Limitations and extensions

The current firmware intentionally omits authentication, MQTT, OTA, and cloud integrations. Future work can add an authentication layer for non-lab networks, optional MQTT bridge, optional OTA, richer dashboard history, and more host tests.
