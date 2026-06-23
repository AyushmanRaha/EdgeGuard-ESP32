# Testing Plan

## Automated checks

| Command | Expected result | Coverage |
| --- | --- | --- |
| `python tools/verify_repo.py` | Required files, links, content guardrails, and secrets checks pass. | Repository integrity. |
| `pio test -e native` | Unity tests pass on the host. | Pure control decisions. |
| `pio run -e esp32doit-devkit-v1` | Firmware compiles for the ESP32 target. | Arduino/PlatformIO integration. |

## Native unit test coverage

| Area | Cases |
| --- | --- |
| AUTO mode | Dark occupied Relay 1 on; bright occupied Relay 1 off; invalid occupancy Relay 1 off; Relay 2 follows temperature latch. |
| Occupancy hold | Short ultrasonic dropouts remain occupied; timeout clears occupancy. |
| Temperature hysteresis | Alert latches at on threshold, remains between thresholds, clears at off threshold. |
| AWAY mode | Relay 1 off; instant occupancy enters `ALERT`; Relay 2 on for occupancy alert. |
| MANUAL mode | State becomes `MANUAL_OVERRIDE`; relay states are preserved by pure logic. |
| Fault handling | Stale or missing sensor timestamp enters `FAULT`, forces relays off, and reports `Sensor snapshot stale`. |
| Boundaries | Exact occupancy distance counts; just above does not; stale timeout uses strict greater-than behavior. |

## Manual boot checklist

1. Power the low-voltage circuit.
2. Open Serial Monitor at `115200`.
3. Confirm boot messages, Wi-Fi mode, IP address, and no immediate unexpected fault.
4. Open `/` in a browser on the same network or fallback AP.

## Sensor validation checklist

- DHT11: temperature and humidity become non-null; disconnecting the data line eventually reports failures.
- HC-SR04: close and far targets change `distance_cm`; no echo is bounded by timeout handling.
- LDR: `light_is_dark` changes with lighting; invert only `LDR_DARK_WHEN_HIGH` if the module polarity differs.

## Mode validation checklist

- AUTO: Relay 1 requires dark and occupied; Relay 2 follows temperature alert hysteresis.
- MANUAL: Relay commands switch to manual state and apply the requested relay.
- AWAY: Relay 1 remains off; instant occupancy raises alert and Relay 2 turns on.

## API validation checklist

```bash
curl http://192.168.4.1/api/status
curl http://192.168.4.1/api/logs
curl -X POST http://192.168.4.1/api/mode/auto
curl -X POST http://192.168.4.1/api/relay1/off
```

Confirm command responses are `{"ok":true}` and status fields match `docs/api.md`.

## Fail-safe and AP fallback checklist

- Force stale sensor data or repeated sensor failures and confirm `FAULT` with both relays off.
- Remove station credentials or use placeholders and confirm fallback AP `EdgeGuard-ESP32` is reachable.
- Capture serial output, dashboard state, and wiring photos only after checking that no credentials are visible.
