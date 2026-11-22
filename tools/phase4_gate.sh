#!/usr/bin/env bash
# Phase 4 gate: electronics designed and checked in software.
#  1. unit tests: power budget, SPICE sweep guards, schematic generator, carrier netlist vs board.h
#  2. schematic regenerated from hardware/kicad/gen/carrier.py; KiCad ERC must report 0 violations
#  3. ngspice sweep of the battery power stage: the design envelope must pass every case
#  4. battery budget for 5,000 and 10,000 mAh: no pending sources, >= 24 h on the fitted pack
#  5. PCB (committed, routed): KiCad DRC 0 errors, 0 unconnected, 0 schematic-parity issues
#  6. firmware still passes its native tests and still builds for the ESP32 (pin map changed in phase 4)
# Writes docs/evidence/phase4-gate.txt. Exit 0 = gate passed. Routing itself (pcbgen.py all) is not
# re-run here: Freerouting's result varies run to run, so the gate checks the committed board.
set -u
cd "$(dirname "$0")/.."
ev=docs/evidence
k=hardware/kicad/carrier
fail=0
{
  echo "Phase 4 gate run: $(date -u +%Y-%m-%dT%H:%M:%SZ)  commit $(git rev-parse --short HEAD)"
  echo "== 1. Unit tests"
  for t in hardware/power_budget/test_budget.py hardware/spice/test_sweep.py hardware/kicad/gen/test_schgen.py hardware/kicad/gen/test_carrier.py; do
    if python3 -m unittest "$t" > /tmp/p4ut.log 2>&1; then echo "  PASS $t ($(grep -o 'Ran [0-9]* tests' /tmp/p4ut.log))"; else echo "  FAIL $t"; tail -5 /tmp/p4ut.log | sed 's/^/    /'; fail=1; fi
  done
  echo "== 2. Schematic + ERC"
  python3 hardware/kicad/gen/symgen.py && python3 hardware/kicad/gen/carrier.py | sed 's/^/  /'
  kicad-cli sch erc --format json --severity-all -o $k/erc.json $k/carrier.kicad_sch > /dev/null
  n=$(python3 -c "import json;r=json.load(open('$k/erc.json'));print(sum(len(s['violations']) for s in r['sheets']))")
  echo "  ERC violations (errors + warnings): $n"; [ "$n" = 0 ] || fail=1
  echo "== 3. SPICE sweep"
  python3 hardware/spice/sweep.py $ev/phase4-spice-sweep.md | tail -2 | sed 's/^/  /'; [ ${PIPESTATUS[0]} -eq 0 ] || fail=1
  echo "== 4. Battery budget"
  python3 hardware/power_budget/budget.py hardware/power_budget/loads.yaml $ev/phase4-power-budget.md > /tmp/p4b.log; rc=$?
  grep -E "Status|Average battery|x 18650|Target" /tmp/p4b.log | sed 's/^/  /'; [ $rc -eq 0 ] || fail=1
  echo "== 5. PCB DRC + schematic parity"
  kicad-cli pcb drc --schematic-parity --format json --severity-all -o $k/drc.json $k/carrier.kicad_pcb > /dev/null
  python3 - "$k/drc.json" <<'PY' || fail=1
import json, sys, collections
r = json.load(open(sys.argv[1]))
err = [v for v in r["violations"] if v["severity"] == "error"]
warn = collections.Counter(v["type"] for v in r["violations"] if v["severity"] != "error")
print(f"  DRC errors: {len(err)}; unconnected: {len(r['unconnected_items'])}; schematic parity issues: {len(r['schematic_parity'])}")
print(f"  DRC warnings (cosmetic, reported only): {dict(warn)}")
for v in err[:10]: print("   ", v["description"])
sys.exit(1 if err or r["unconnected_items"] or r["schematic_parity"] else 0)
PY
  python3 - <<'PY'
import pcbnew
b = pcbnew.LoadBoard("hardware/kicad/carrier/carrier.kicad_pcb")
bb = b.GetBoardEdgesBoundingBox()
print(f"  board {pcbnew.ToMM(bb.GetWidth()):.0f} x {pcbnew.ToMM(bb.GetHeight()):.0f} mm, 2 layers, "
      f"{len(b.GetFootprints())} footprints, {len(b.GetNetInfo().NetsByName()) - 1} nets, {len(b.GetTracks())} track/via items")
PY
  echo "== 6. Firmware"
  (cd firmware && pio test -e native > /tmp/p4fw.log 2>&1); grep -E "test cases" /tmp/p4fw.log | sed 's/^/  /'; grep -q "test cases:.*succeeded" /tmp/p4fw.log && ! grep -q "FAILED" /tmp/p4fw.log || fail=1
  if (cd firmware && pio run -e esp32 > /tmp/p4b32.log 2>&1); then echo "  esp32 build: SUCCESS"; else echo "  esp32 build: FAILED"; fail=1; fi
  echo
  [ $fail -eq 0 ] && echo "GATE: PASSED" || echo "GATE: FAILED"
  exit $fail
} 2>&1 | tee $ev/phase4-gate.txt
exit ${PIPESTATUS[0]}
