#!/usr/bin/env python3
"""Repository integrity checks for EdgeGuard ESP32."""
from __future__ import annotations

import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SKIP_DIRS = {".git", ".pio", "build", "dist", "__pycache__", ".pytest_cache", ".mypy_cache"}
REQUIRED = [
    "README.md", "platformio.ini", "requirements.txt", "AGENTS.md",
    "firmware/EdgeGuard_ESP32/EdgeGuard_ESP32.ino",
    "firmware/EdgeGuard_ESP32/config.h",
    "firmware/EdgeGuard_ESP32/secrets.h.example",
    "firmware/EdgeGuard_ESP32/app_state.cpp",
    "firmware/EdgeGuard_ESP32/sensors.cpp",
    "firmware/EdgeGuard_ESP32/control.cpp",
    "firmware/EdgeGuard_ESP32/wifi_manager.cpp",
    "firmware/EdgeGuard_ESP32/web_routes.cpp",
    "firmware/EdgeGuard_ESP32/tasks.cpp",
    "docs/architecture.md", "docs/api.md", "docs/development.md", "docs/hardware.md",
    "docs/ci.md", "docs/testing_plan.md", "docs/design_rationale.md", "docs/troubleshooting.md",
    "hardware/wiring_table.md",
    "test/test_control_logic/test_control_logic.cpp",
    "test/native_stubs/Arduino.h",
    "test/native_stubs/freertos/FreeRTOS.h",
]
FORBIDDEN_PATHS = ["firmware/EdgeGuard_ESP32/secrets" + ".example.h", "firmware/EdgeGuard_ESP32/secrets.h", "docs/" + "inter" + "view_notes.md"]
FORBIDDEN_PARTS = ["Qual" + "comm", "Nv" + "idia", "NVI" + "DIA", "In" + "tel", "inter" + "view", "jo" + "bs?", "hir" + "ing", "recruit" + "ers?", "built" + " for " + "inter" + "views", "align" + "ed for " + "jo" + "bs"]
FORBIDDEN_TERMS = re.compile(r"\b(" + "|".join(FORBIDDEN_PARTS) + r")\b", re.I)
PLACEHOLDER_OK = {"YOUR_WIFI_NAME", "YOUR_WIFI_PASSWORD", ""}
TEXT_SUFFIXES = {".md", ".h", ".hpp", ".cpp", ".ino", ".ini", ".yml", ".yaml", ".txt", ".py", ".gitignore"}


def tracked_files() -> list[Path]:
    try:
        out = subprocess.check_output(["git", "ls-files"], cwd=ROOT, text=True)
        return [ROOT / line for line in out.splitlines() if line]
    except Exception:
        files: list[Path] = []
        for base, dirs, names in os.walk(ROOT):
            dirs[:] = [d for d in dirs if d not in SKIP_DIRS]
            files.extend(Path(base) / n for n in names)
        return files


def is_text(path: Path) -> bool:
    return path.name in TEXT_SUFFIXES or path.suffix in TEXT_SUFFIXES


def fail(message: str, failures: list[str]) -> None:
    print(f"FAIL: {message}")
    failures.append(message)


def pass_msg(message: str) -> None:
    print(f"PASS: {message}")


def check_local_links(failures: list[str]) -> None:
    link_pattern = re.compile(r"\[[^\]]+\]\((?!https?://|#|mailto:)([^)]+)\)")
    for doc in [ROOT / "README.md", *sorted((ROOT / "docs").glob("*.md"))]:
        text = doc.read_text(encoding="utf-8")
        base = doc.parent
        for match in link_pattern.finditer(text):
            target = match.group(1).split("#", 1)[0].strip()
            if not target or target.startswith("media/"):
                continue
            if not (base / target).resolve().exists() and not (ROOT / target).exists():
                fail(f"local link target missing in {doc.relative_to(ROOT).as_posix()}: {target}", failures)


def main() -> int:
    failures: list[str] = []
    for rel in REQUIRED:
        if not (ROOT / rel).exists(): fail(f"required file missing: {rel}", failures)
    if not failures: pass_msg("required files exist")
    for rel in FORBIDDEN_PATHS:
        if (ROOT / rel).exists(): fail(f"forbidden file present: {rel}", failures)
    if not any((ROOT / rel).exists() for rel in FORBIDDEN_PATHS): pass_msg("duplicate examples and committed secrets are absent")

    for path in tracked_files():
        rel = path.relative_to(ROOT).as_posix()
        if any(part in SKIP_DIRS for part in path.relative_to(ROOT).parts): continue
        if not is_text(path): continue
        if not path.exists(): continue
        try: text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError: continue
        filtered = "\n".join(line for line in text.splitlines() if not (rel == ".github/workflows/ci.yml" and line.strip() == ("jo" + "bs:")))
        if FORBIDDEN_TERMS.search(filtered): fail(f"forbidden positioning term in {rel}", failures)
        for name in ("WIFI_SSID", "WIFI_PASSWORD"):
            for m in re.finditer(rf"#define\s+{name}\s+\"([^\"]*)\"", text):
                if m.group(1) not in PLACEHOLDER_OK and rel != "firmware/EdgeGuard_ESP32/wifi_manager.cpp":
                    fail(f"possible real Wi-Fi credential in {rel}", failures)
    if not failures: pass_msg("text policy and credential checks passed")
    check_local_links(failures)
    if not failures: pass_msg("README and docs local links resolve")
    return 1 if failures else 0

if __name__ == "__main__":
    sys.exit(main())
