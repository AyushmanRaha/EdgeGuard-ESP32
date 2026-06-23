# Testing Plan

Automated checks:
- `bash scripts/run_host_tests.sh` builds and runs C++17 control, event log, diagnostics, and history tests.
- `python3 scripts/validate_repo.py` checks required files, frozen pin values, secrets policy, banned wording, and secret-key templates.
- `python3 scripts/check_firmware_structure.py` checks Arduino structure, required modules, forbidden dependency wording, and low-voltage safety wording.
- CI runs Arduino CLI compile for `esp32:esp32:esp32` without hardware upload.

Manual hardware validation should verify boot relay-off behavior, AUTO dark/occupied behavior, MANUAL relay control, AWAY alert behavior, fault behavior, fault reset, `/api/diagnostics`, and `/api/history.csv`.
