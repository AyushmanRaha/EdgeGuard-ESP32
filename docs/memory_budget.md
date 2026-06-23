# Memory Budget

EdgeGuard uses fixed-size firmware data structures where practical:

| Item | Size strategy |
|---|---|
| Event log | `EVENT_LOG_SIZE` fixed entries, each with bounded message storage. |
| Sensor history | `SENSOR_HISTORY_SIZE` fixed entries in RAM. |
| Status JSON | 1024-byte fixed buffer. |
| Diagnostics JSON | 3072-byte fixed buffer. |
| Logs JSON | 4096-byte fixed buffer. |
| History JSON/CSV | 8192-byte fixed buffers. |
| Task stacks | Sensor 4096, Control 4096, Web 8192, Heartbeat 2048 bytes. |

Heap monitoring is reported through `/api/diagnostics`. Reusable modules avoid unbounded allocation and remain host-testable.
