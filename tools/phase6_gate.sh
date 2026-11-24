#!/usr/bin/env bash
# Phase 6 gate: enclosure designed and checked in software (CadQuery).
#  1. extract part positions/heights from the routed PCB (mechanical/cadquery/board_features.json)
#  2. build and export the printed parts (STEP + STL)
#  3. interference, clearance, opening alignment, printability checks (check_enclosure.py)
#  4. renders (docs/evidence/phase6/*.png)
# Writes docs/evidence/phase6-gate.txt. Exit 0 = gate passed.
set -u
cd "$(dirname "$0")/.."
m=mechanical/cadquery
fail=0
{
  echo "Phase 6 gate run: $(date -u +%Y-%m-%dT%H:%M:%SZ)  commit $(git rev-parse --short HEAD)"
  echo "== 1. Board features"; python3 $m/board_features.py | sed 's/^/  /' || fail=1
  echo "== 2. Build + export"; python3 $m/enclosure.py | sed 's/^/  /'; [ ${PIPESTATUS[0]} -eq 0 ] || fail=1
  echo "== 3. Checks"; python3 $m/check_enclosure.py > /tmp/p6.log; rc=$?; grep -E "^\| |ALL PASS|FAILED" /tmp/p6.log | sed 's/^/  /'; [ $rc -eq 0 ] || fail=1
  echo "== 4. Renders"; python3 $m/render.py | sed 's/^/  /' || fail=1
  echo
  [ $fail -eq 0 ] && echo "GATE: PASSED" || echo "GATE: FAILED"
  exit $fail
} 2>&1 | tee docs/evidence/phase6-gate.txt
exit ${PIPESTATUS[0]}
