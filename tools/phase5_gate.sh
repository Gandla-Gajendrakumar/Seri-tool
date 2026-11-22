#!/usr/bin/env bash
# Phase 5 gate: bill of materials with Indian suppliers.
#  1. BOM unit tests
#  2. every board part (from the schematic netlist) and every off-board part maps to a researched item
#  3. every line priced with a link; chosen sources in stock at research time (else NOT FINAL)
# Writes bom/bom.md, bom/bom.csv and docs/evidence/phase5-gate.txt. Exit 0 = BOM final.
set -u
cd "$(dirname "$0")/.."
fail=0
{
  echo "Phase 5 gate run: $(date -u +%Y-%m-%dT%H:%M:%SZ)  commit $(git rev-parse --short HEAD)"
  echo "== 1. Unit tests"
  if python3 -m unittest bom/test_bom.py > /tmp/p5ut.log 2>&1; then echo "  PASS bom/test_bom.py ($(grep -o 'Ran [0-9]* tests' /tmp/p5ut.log))"; else echo "  FAIL"; tail -5 /tmp/p5ut.log; fail=1; fi
  echo "== 2-3. BOM"
  python3 bom/gen_bom.py > /tmp/p5bom.log; rc=$?
  grep -E "Status|out of stock|without a price|Subtotal|Per unit|Pilot batch" bom/bom.md | sed 's/^/  /'
  [ $rc -eq 0 ] || fail=1
  echo
  [ $fail -eq 0 ] && echo "GATE: PASSED" || echo "GATE: NOT PASSED (see status above)"
  exit $fail
} 2>&1 | tee docs/evidence/phase5-gate.txt
exit ${PIPESTATUS[0]}
