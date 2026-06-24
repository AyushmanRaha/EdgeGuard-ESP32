# State Machine

## States

```text
BOOT
  ↓
CALIBRATING
  ↓
AUTO_MONITORING
  ↓       ↓
ALERT   MANUAL_OVERRIDE
  ↓       ↓
FAULT ← sensor failure
```
