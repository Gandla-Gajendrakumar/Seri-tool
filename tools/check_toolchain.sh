#!/usr/bin/env bash
# Phase 0 toolchain check. Each tool is EXERCISED, not just located.
# Usage: tools/check_toolchain.sh   (exit code = number of failed checks)
set -u
cd "$(dirname "$0")/.."
export PLATFORMIO_SETTING_ENABLE_TELEMETRY=no
fail=0
check() {  # name, command...
  local name=$1; shift
  if out=$("$@" 2>&1); then printf 'PASS  %-34s %s\n' "$name" "$(echo "$out" | tail -1)"
  else printf 'FAIL  %-34s %s\n' "$name" "$(echo "$out" | tail -1)"; fail=$((fail+1)); fi
}
scratch=$(mktemp -d)

check "platformio version"            pio --version
check "native unit tests (Unity)"     bash -c 'cd firmware && pio test -e native | grep -E "test cases:"'
check "ESP32 cross-compile"           bash -c 'cd firmware && pio run -e esp32 | grep -E "^Flash:"'
check "wokwi-cli version"             wokwi-cli --version
check "kicad-cli >= 8 (has ERC/DRC)"  bash -c 'v=$(kicad-cli version); [ "${v%%.*}" -ge 8 ] && kicad-cli sch erc --help >/dev/null && kicad-cli pcb drc --help >/dev/null && echo "kicad-cli $v"'
check "ngspice divider 12V->6V"       bash -c "printf 't\nV1 1 0 12\nR1 1 2 1k\nR2 2 0 1k\n.op\n.print op v(2)\n.end\n' > $scratch/t.cir && ngspice -b $scratch/t.cir 2>/dev/null | grep -E '^v\(2\)|V\(2\)' | grep -q '6.000000e+00' && ngspice --version | grep -o 'ngspice-[0-9]*'"
check "cadquery box + STEP export"    python3 -c "import cadquery as cq; b=cq.Workplane().box(10,10,10); cq.exporters.export(b,'$scratch/b.step'); import os; assert abs(b.val().Volume()-1000)<1e-6 and os.path.getsize('$scratch/b.step')>0; print('cadquery', cq.__version__)"
check "host C/C++ compiler"           bash -c 'g++ --version | head -1'
if [ -n "${WOKWI_CLI_TOKEN:-}" ]; then echo "INFO  WOKWI_CLI_TOKEN is set"; else echo "INFO  WOKWI_CLI_TOKEN not set (needed from phase 2; simulation runs cannot be checked yet)"; fi
rm -rf "$scratch"
echo "---- $fail check(s) failed"
exit $fail
