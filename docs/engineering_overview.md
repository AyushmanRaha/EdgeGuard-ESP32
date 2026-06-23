# Engineering Overview

EdgeGuard is structured as a compact embedded firmware product for local smart-room monitoring and safe low-voltage relay control.

Engineering goals include deterministic control behavior, bounded memory use, host-testable logic, local diagnostics, and conservative fault behavior. Key decisions include keeping Arduino IDE compatibility, separating pure C++ control logic from hardware integration, using fixed-size event and history buffers, and protecting local control endpoints with a configurable token.

Reliability features include consecutive sensor-failure detection, temperature hysteresis, occupancy hold timing, task heartbeat diagnostics, software watchdog supervision, severity-coded events, and manual fault reset.

Verification combines host tests, repository validation scripts, firmware structure checks, Arduino compile CI, and manual hardware validation. The safety scope is intentionally limited to low-voltage LED relay loads. Future technical work can include measured timing data, expanded trace replay tests, and additional dashboard polish without changing the hardware map.
