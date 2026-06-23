# Architecture

EdgeGuard separates hardware-facing Arduino code from pure C++ modules that can be compiled on a host machine. The Arduino integration layer owns Wi-Fi/AP fallback, `WebServer` routes, GPIO drivers, DHT11 reads, HC-SR04 timing, LDR reads, and relay writes.

The RTOS-style task model uses SensorTask, ControlTask, WebTask, and HeartbeatTask. Shared sensor and system snapshots are copied under `gMutex`; slow work such as HTTP response generation and serial output is kept outside critical sections where practical.

Core modules:
- `edgeguard_control.cpp`: deterministic state machine, hysteresis, occupancy hold, manual mode, away mode, and fault priority.
- `edgeguard_event_log.cpp`: fixed-capacity severity log.
- `edgeguard_diagnostics.cpp`: task heartbeat, loop duration, jitter, deadline miss, heap, reset, and Wi-Fi status snapshot.
- `edgeguard_sensor_history.cpp`: fixed-capacity sample history for JSON and CSV export.

API routes expose status, logs, diagnostics, history, and token-protected control endpoints. FAULT remains the highest-priority state and always drives both relay outputs off.
