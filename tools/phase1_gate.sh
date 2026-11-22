#!/usr/bin/env bash
# Phase 1 gate: all native unit tests pass, core stays hardware-free, core cross-compiles for ESP32.
# Usage: tools/phase1_gate.sh   (writes docs/evidence/phase1-gate.txt; exit code 0 = gate passed)
set -u
cd "$(dirname "$0")/.."
export PLATFORMIO_SETTING_ENABLE_TELEMETRY=no
out=docs/evidence/phase1-gate.txt
fail=0
{
  echo "Phase 1 gate run: $(date -u +%Y-%m-%dT%H:%M:%SZ)  commit $(git rev-parse --short HEAD)"
  echo
  echo "== 1. Native unit tests (pio test -e native)"
  (cd firmware && pio test -e native 2>&1) | grep -E "^test/|^native |test cases" | sed 's/^/  /'
  (cd firmware && pio test -e native >/dev/null 2>&1) || fail=1
  echo
  echo "== 2. lib/core contains no Arduino / ESP-IDF headers"
  if grep -rnE '#include\s*[<"](Arduino|esp_|freertos|driver/|Wire|SPI)' firmware/lib/core; then echo "  FAIL"; fail=1; else echo "  PASS"; fi
  echo
  echo "== 3. ESP32 cross-compile with lib/core linked"
  if (cd firmware && pio run -e esp32 2>&1) | grep -E "^(RAM|Flash):|SUCCESS" | sed 's/^/  /'; then :; else fail=1; fi
  (cd firmware && pio run -e esp32 >/dev/null 2>&1) || fail=1
  echo
  if [ $fail -eq 0 ]; then echo "GATE: PASSED"; else echo "GATE: FAILED"; fi
} 2>&1 | tee "$out"
exit $fail
