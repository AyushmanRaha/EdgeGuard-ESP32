# Timing Analysis

Diagnostics records task heartbeat count, last heartbeat time, last loop duration, maximum loop duration, expected period, maximum jitter, and deadline misses. Jitter is the absolute difference between the measured heartbeat interval and expected period. A deadline miss is counted when the measured interval exceeds the expected period plus a tolerance band.

| Metric | How to capture | Measured value |
|---|---|---|
| SensorTask max jitter | /api/diagnostics | TODO: fill |
| ControlTask max jitter | /api/diagnostics | TODO: fill |
| Free heap after 10 min | /api/diagnostics | TODO: fill |
| Min free heap after 10 min | /api/diagnostics | TODO: fill |
