# Testing Plan

## Automated checks
- Run `python tools/verify_repo.py`.
- Run `pio run` from the repository root.

## Manual hardware checklist
1. Boot: verify serial output, LEDs, dashboard availability, and no unexpected fault.
2. DHT11 normal: confirm temperature and humidity become non-null.
3. DHT11 fault: disconnect data temporarily and confirm repeated failures lead to `FAULT` and relays off.
4. HC-SR04 normal: move a target within and beyond the threshold and confirm distance changes.
5. HC-SR04 fault: block echo or disconnect safely and confirm timeout protection plus fail-safe behavior after repeated failures.
6. LDR polarity: confirm `light_is_dark` is true in darkness with `LDR_DARK_WHEN_HIGH = true`.
7. AUTO behavior: Relay 1 turns on only when dark and occupied; Relay 2 follows temperature alert hysteresis.
8. MANUAL behavior: relay POST commands switch to `MANUAL` and immediately apply the requested relay state.
9. AWAY behavior: Relay 1 stays off; instant occupancy enters `ALERT` and turns Relay 2 on.
10. Dashboard/API: verify `/`, `/api/status`, `/api/logs`, and all command routes.
11. Relay fail-safe: verify `FAULT` forces both relay outputs off.
12. AP fallback: leave Wi-Fi placeholders in `secrets.h.example` or omit local credentials and connect to `EdgeGuard-ESP32`.
13. Diagnostics: verify `/api/status` includes firmware metadata, stale-data flags, task heartbeat timestamps, failure counters, heap minimum, reset reason, Wi-Fi fallback state, and watchdog status.
