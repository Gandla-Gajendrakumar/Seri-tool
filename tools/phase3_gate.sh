#!/usr/bin/env bash
# Phase 3 gate: whole rearing cycle (38 days, synthetic) replayed through the real device logic.
#  1. regenerate the cycle data (deterministic) and confirm its fingerprint
#  2. independent oracle (from the written rules) must reproduce the committed expected.txt exactly
#  3. replay with the real Monitor on fake hardware; actual must equal expected line for line
#  4. caps and invariants (check_cycle.py)
# Writes docs/evidence/phase3-gate.txt. Exit 0 = gate passed.
set -u
cd "$(dirname "$0")/.."
out=sim/replay/out
mkdir -p $out
fail=0
{
  echo "Phase 3 gate run: $(date -u +%Y-%m-%dT%H:%M:%SZ)  commit $(git rev-parse --short HEAD)"
  echo "== 1. Generate synthetic cycle"
  python3 sim/replay/gen_cycle.py $out/cycle.csv | sed 's/^/  /'
  echo "  sha256 $(sha256sum $out/cycle.csv | cut -c1-16)"
  echo "== 2. Oracle (rules) vs committed expected.txt"
  python3 sim/replay/oracle.py $out/cycle.csv $out/expected.regen.txt | sed 's/^/  /'
  if cmp -s $out/expected.regen.txt sim/replay/expected.txt; then echo "  expected.txt reproduced exactly"; else echo "  FAIL expected.txt differs from oracle output"; fail=1; fi
  echo "== 3. Replay: real Monitor on fake hardware"
  g++ -std=gnu++17 -O2 -Ifirmware/lib/core/include -Ifirmware/lib/core/src -Ifirmware/test/fakes \
      sim/replay/replay.cpp firmware/lib/core/src/*.cpp -o $out/replay || fail=1
  $out/replay $out/cycle.csv $out/actual.txt $out/stats.txt | sed 's/^/  /'
  if diff <(tail -n +2 sim/replay/expected.txt) <(tail -n +2 $out/actual.txt) > $out/diff.txt; then
    echo "  actual == expected: $(grep -c '^E ' $out/actual.txt) events, $(grep -c '^Q ' $out/actual.txt) SMS decisions, line for line"
  else
    echo "  FAIL actual differs from expected:"; head -20 $out/diff.txt | sed 's/^/    /'; fail=1
  fi
  echo "== 4. Caps and invariants"
  python3 sim/replay/check_cycle.py $out/actual.txt $out/stats.txt || fail=1
  echo
  [ $fail -eq 0 ] && echo "GATE: PASSED" || echo "GATE: FAILED"
  exit $fail
} 2>&1 | tee docs/evidence/phase3-gate.txt
exit ${PIPESTATUS[0]}
