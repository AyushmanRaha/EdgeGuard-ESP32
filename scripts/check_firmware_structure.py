#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

def fail(message):
    print(f"ERROR: {message}")
    sys.exit(1)

required = [
    'firmware/EdgeGuard_ESP32/EdgeGuard_ESP32.ino',
    'firmware/EdgeGuard_ESP32/edgeguard_control.h',
    'firmware/EdgeGuard_ESP32/edgeguard_control.cpp',
    'firmware/EdgeGuard_ESP32/edgeguard_types.h',
    'firmware/EdgeGuard_ESP32/edgeguard_event_log.h',
    'firmware/EdgeGuard_ESP32/edgeguard_event_log.cpp',
    'firmware/EdgeGuard_ESP32/edgeguard_diagnostics.h',
    'firmware/EdgeGuard_ESP32/edgeguard_diagnostics.cpp',
    'firmware/EdgeGuard_ESP32/edgeguard_sensor_history.h',
    'firmware/EdgeGuard_ESP32/edgeguard_sensor_history.cpp',
]
for item in required:
    if not (ROOT / item).exists():
        fail(f'missing {item}')

forbidden = ['OLED display required', 'camera required', 'Firebase required', 'Blynk required', 'MQTT required', 'ESP-IDF required', 'PlatformIO required']
for path in [ROOT / 'README.md', *list((ROOT / 'docs').glob('*.md')), *list((ROOT / 'hardware').glob('*.md'))]:
    text = path.read_text(errors='ignore')
    lower = text.lower()
    for term in forbidden:
        if term.lower() in lower:
            fail(f'forbidden requirement phrase "{term}" in {path.relative_to(ROOT)}')
    if 'ac mains wiring support' in lower or 'connect ac mains' in lower:
        fail(f'unsafe mains wording in {path.relative_to(ROOT)}')

print('Firmware structure validation passed.')
