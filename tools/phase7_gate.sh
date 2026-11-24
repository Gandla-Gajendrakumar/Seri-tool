#!/usr/bin/env bash
# Phase 7 gate: hand-off pack complete and consistent.
#  1. re-run the gates of phases 1, 3, 4, 6 (must pass) and 5 (status reported: ordering is blocked while NOT FINAL)
#  2. build the release outputs (fab files, firmware binaries + checksums), the farmer card, the traceability matrix
#  3. every file path / link mentioned in docs/handoff/*.md exists
# Phase 2 (Wokwi) runs in GitHub Actions on demand (D-036); its last result is reported, not re-run here.
# Writes docs/evidence/phase7-gate.txt. Exit 0 = hand-off pack passes.
set -u
cd "$(dirname "$0")/.."
fail=0
{
  echo "Phase 7 gate run: $(date -u +%Y-%m-%dT%H:%M:%SZ)  commit $(git rev-parse --short HEAD)"
  echo "== 1. Earlier gates"
  for n in 1 3 4 6; do
    if timeout 1200 tools/phase${n}_gate.sh > /tmp/p7_$n.log 2>&1; then echo "  phase $n: PASSED"; else echo "  phase $n: FAILED (see docs/evidence)"; fail=1; fi
  done
  tools/phase5_gate.sh > /tmp/p7_5.log 2>&1 && echo "  phase 5: FINAL" || echo "  phase 5: NOT FINAL -> $(grep -m1 'out of stock\|without a price' /tmp/p7_5.log | sed 's/^ *//')"
  echo "  phase 2: Wokwi in GitHub Actions; last recorded: $(grep -m1 -o '[0-9]/[0-9] scenarios[^|]*\|8 Wokwi scenarios' docs/evidence/phase2-gate.md || echo see docs/evidence/phase2-gate.md); re-run pending (B-27)"
  echo "== 2. Release outputs"
  tools/make_release.sh 2>&1 | sed 's/^/  /'; [ ${PIPESTATUS[0]} -eq 0 ] || fail=1
  ver=$(grep -o 'kFirmwareVersion = "[^"]*"' firmware/include/board.h | cut -d'"' -f2)
  (cd firmware/release/v$ver && sha256sum -c --quiet SHA256SUMS) && echo "  firmware v$ver checksums OK" || fail=1
  python3 tools/gen_card.py | sed 's/^/  /' || fail=1
  python3 tools/gen_trace.py | sed 's/^/  /'; [ ${PIPESTATUS[0]} -eq 0 ] || fail=1
  echo "== 3. Links and paths in the hand-off docs"
  python3 - <<'PY' || fail=1
import os, re, sys
missing, n = [], 0
for f in sorted(os.listdir("docs/handoff")):
    if not f.endswith(".md"):
        continue
    text = open(os.path.join("docs/handoff", f), encoding="utf-8").read()
    refs = set(re.findall(r"\]\(([^)#]+)\)", text)) | {m for m in re.findall(r"`([A-Za-z0-9_./*-]+/[A-Za-z0-9_.*-]+)`", text)}
    for r in refs:
        if r.startswith("http") or "<" in r:
            continue
        n += 1
        base = os.path.join("docs/handoff", r) if not r.startswith(("docs/", "bom/", "hardware/", "firmware/", "mechanical/", "tools/", "sim/")) else r
        path = base.split(" ")[0]
        if "*" in path:
            import glob
            ok = bool(glob.glob(path))
        else:
            ok = os.path.exists(path)
        if not ok:
            missing.append(f"{f}: {r}")
print(f"  {n} references checked, {len(missing)} missing")
for m in missing: print("   ", m)
sys.exit(1 if missing else 0)
PY
  echo
  [ $fail -eq 0 ] && echo "GATE: PASSED" || echo "GATE: FAILED"
  exit $fail
} 2>&1 | tee docs/evidence/phase7-gate.txt
exit ${PIPESTATUS[0]}
