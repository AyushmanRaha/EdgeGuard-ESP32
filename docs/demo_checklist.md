# Demo Checklist

## Functional validation
- Open dashboard over Wi-Fi or fallback AP.
- Verify AUTO mode with dark + occupied behavior.
- Verify MANUAL relay control.
- Verify AWAY nearby-motion alert behavior.
- Verify FAULT after repeated sensor failure.
- Verify fault reset.
- Open `/api/diagnostics`.
- Download `/api/history.csv`.

## Evidence capture
- Hero build photo.
- Wiring overview photo.
- HC-SR04 divider close-up.
- Dashboard screenshot.
- Diagnostics screenshot.
- CI passing screenshot.
- Serial Monitor runtime log screenshot.

## Safety verification
- Confirm relay loads are low-voltage LEDs only.
- Confirm HC-SR04 Echo uses the divider.
- Confirm common GND across modules.
