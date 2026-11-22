#!/usr/bin/env bash
# Builds each custom Wokwi chip for this PC against a stand-in chip API and runs its tests.
# Also compiles every chip to WebAssembly (needs sim/wokwi/chips/wokwi-api.h).
set -u
cd "$(dirname "$0")/../sim/wokwi/chips"
out=$(mktemp -d)
fail=0
for chip in sht31 ds3231 a7670c printer; do
  if clang -std=c11 -Wall -Wno-unknown-attributes -include test/host_api.h -o "$out/$chip" \
       $chip.chip.c test/host_api.c test/test_$chip.c -lm 2>"$out/$chip.err"; then
    "$out/$chip" || fail=1
  else
    cat "$out/$chip.err"; fail=1
  fi
  wokwi-cli chip compile $chip.chip.c -o $chip.chip.wasm -q >/dev/null 2>&1 || { echo "wasm build failed: $chip"; fail=1; }
done
rm -rf "$out"
[ $fail -eq 0 ] && echo "CHIPS: ALL PASSED" || echo "CHIPS: FAILED"
exit $fail
