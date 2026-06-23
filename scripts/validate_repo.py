#!/usr/bin/env python3
from pathlib import Path
import os
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]

def fail(message):
    print(f"ERROR: {message}")
    sys.exit(1)

def need(path):
    if not (ROOT / path).exists():
        fail(f"missing {path}")

required = [
    'firmware/EdgeGuard_ESP32/secrets.h.example', 'README.md', 'AGENTS.md',
    'docs/architecture.md', 'docs/state_machine.md', 'docs/testing_plan.md',
    'docs/engineering_overview.md', 'docs/ci.md', 'docs/security_model.md',
    'docs/timing_analysis.md', 'docs/memory_budget.md', 'docs/failure_modes_and_effects.md',
    'docs/api.md', 'docs/demo_checklist.md',
]
for item in required:
    need(Path(item))

if (ROOT / 'docs' / ('inter' + 'view_notes.md')).exists():
    fail('deprecated notes document must be replaced by a professional engineering document')

tracked = subprocess.run(['git', 'ls-files', 'firmware/EdgeGuard_ESP32/secrets.h'], cwd=ROOT, text=True, capture_output=True, check=True).stdout.strip()
if tracked:
    fail('firmware/EdgeGuard_ESP32/secrets.h is tracked')
if (ROOT / 'firmware/EdgeGuard_ESP32/secrets.h').exists() and os.environ.get('CI'):
    fail('secrets.h should not be present in CI before the compile step')

config = (ROOT / 'firmware/EdgeGuard_ESP32/config.h').read_text()
expected = {'PIN_DHT': 4, 'PIN_HCSR04_TRIG': 5, 'PIN_HCSR04_ECHO': 18, 'PIN_LDR_DO': 34, 'PIN_RELAY1': 26, 'PIN_RELAY2': 27, 'PIN_LED_GREEN': 23, 'PIN_LED_RED': 22}
for name, val in expected.items():
    if not re.search(rf'constexpr\s+uint8_t\s+{name}\s*=\s*{val}\s*;', config):
        fail(f'{name} must remain GPIO {val}')

example = (ROOT / 'firmware/EdgeGuard_ESP32/secrets.h.example').read_text()
for key in ['WIFI_SSID', 'WIFI_PASSWORD', 'EDGEGUARD_AP_SSID', 'EDGEGUARD_AP_PASSWORD', 'EDGEGUARD_API_TOKEN']:
    if key not in example:
        fail(f'secrets.h.example missing {key}')

banned = [
    'stud' + 'ent ' + 'proj' + 'ect',
    'port' + chr(102) + 'olio',
    'inter' + chr(118) + 'iew',
    'recru' + 'iter',
    'res' + chr(117) + 'me',
    'work request phrase'.replace('work request', 'job ' + 'app' + 'lication'),
    'Q' + 'ual' + 'comm',
    'Nv' + 'idia',
    'NV' + 'IDIA',
    'Texas ' + 'Instr' + 'uments',
]
skip_dirs = {'.git', 'build'}
for path in ROOT.rglob('*'):
    if any(part in skip_dirs for part in path.parts):
        continue
    if not path.is_file():
        continue
    try:
        data = path.read_bytes()
        if b'\0' in data:
            continue
        text = data.decode('utf-8')
    except UnicodeDecodeError:
        continue
    for term in banned:
        if term.lower() in text.lower():
            fail(f'banned wording "{term}" in {path.relative_to(ROOT)}')

print('Repository validation passed.')
