#!/usr/bin/env python3
"""Sweeps the power-stage netlist over battery state, pack size, printer peak current and whether the
printer overlaps a modem burst, and checks each case against the datasheet limits:
  modem VBAT >= 3.40 V   (A7670 HD V1.03 p26)
  3.3 V rail  >= 3.00 V  (ESP32-WROOM-32E v2.1 p28)
  cell current <= 3.9 A per cell (Great Power 18650 spec, 1.5C continuous)
  pack current  < 5.1 A   (DW01A over-current trip VOI1 >= 120 mV, DW01x-DS-17 p9, over 23 mOhm of FETs:
                          above this the protection may cut the battery off and reset everything)
Usage: sweep.py [out.md]   (exit 0 if the chosen design envelope passes)
"""
import itertools
import os
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
BASE = open(os.path.join(HERE, "power_stage.cir")).read()
LIMITS = {"modem": 3.40, "v3v3": 3.00, "cell_a": 3.9, "pack_a": 0.120 / 0.0233}


def run(vocv, cells, iprn, overlap):
    """overlap=True: printer heating during the modem bursts. overlap=False: the firmware keeps them apart
    (D-039), so the modem bursts and the printing are simulated separately and the worst of each is kept."""
    if overlap:
        return _run(vocv, cells, iprn, 2.0)
    a, b = _run(vocv, cells, 0.0, 2.0), _run(vocv, cells, iprn, 0.0)
    vm, v3, ca, pa = min(a[0], b[0]), min(a[1], b[1]), max(a[2], b[2]), max(a[3], b[3])
    return vm, v3, ca, pa, _ok(vm, v3, ca, pa)


def _ok(vm, v3, ca, pa):
    return vm >= LIMITS["modem"] and v3 >= LIMITS["v3v3"] and ca <= LIMITS["cell_a"] and pa < LIMITS["pack_a"]


def _run(vocv, cells, iprn, iburst):
    net = BASE
    net = re.sub(r"\.param VOCV=\S+ RCELL=\{[^}]*\}", f".param VOCV={vocv} RCELL={{0.060/{cells}}}", net)
    net = re.sub(r"IPRN=\S+", f"IPRN={iprn}", net)
    net = re.sub(r"IBURST=\S+", f"IBURST={iburst}", net)
    with tempfile.NamedTemporaryFile("w", suffix=".cir", delete=False) as f:
        f.write(net)
    out = subprocess.run(["ngspice", "-b", f.name], capture_output=True, text=True).stdout
    os.unlink(f.name)
    m = re.search(r"RESULT vmodem_min=(\S+) vsys_min=(\S+) v3v3_min=(\S+) ibat_min=(\S+)", out)
    if not m:
        raise RuntimeError(out[-800:])
    vm, vs, v3, ib = (float(x) for x in m.groups())
    cell_a = -ib / cells
    return vm, v3, cell_a, -ib, _ok(vm, v3, cell_a, -ib)


# The design envelope the board must meet (D-038/D-039): firmware keeps printing and modem bursts apart on
# battery, stops the modem below 3.60 V, and prints in low-power mode (24 of 80 heating dots) so the
# printer's 5 V peak stays <= 1.0 A (estimate; bench item). The other cases are reported to justify those rules.
ENVELOPE = lambda vocv, cells, iprn, overlap: (not overlap) and vocv >= 3.6 and iprn <= 1.0


def main(argv):
    rows = ["| Cell OCV | Cells | Printer peak (5 V) | Printer during modem burst | Modem min | 3V3 min | Per-cell A | Pack A | In envelope | Result |",
            "|---|---|---|---|---|---|---|---|---|---|"]
    results = {}
    for vocv, cells, iprn, overlap in itertools.product((3.6, 3.8, 4.0), (2, 4), (0.75, 1.0, 1.5, 2.0), (True, False)):
        vm, v3, ca, pa, ok = run(vocv, cells, iprn, overlap)
        results[(vocv, cells, iprn, overlap)] = ok
        env = "yes" if ENVELOPE(vocv, cells, iprn, overlap) else "no"
        rows.append(f"| {vocv:.1f} V | {cells} | {iprn:.2f} A | {'yes' if overlap else 'no'} | {vm:.3f} V | {v3:.3f} V | "
                    f"{ca:.2f} | {pa:.2f} | {env} | {'PASS' if ok else 'FAIL'} |")
    bad = [k for k, ok in results.items() if ENVELOPE(*k) and not ok]
    n = sum(1 for k in results if ENVELOPE(*k))
    verdict = (f"\nDesign envelope (interlock on, battery >= 3.60 V, 2 or 4 cells, low-power printing <= 1.0 A at 5 V, protection FETs included): "
               f"{n - len(bad)}/{n} cases pass -> {'PASS' if not bad else 'FAIL'}\n")
    text = "\n".join(rows) + "\n" + verdict
    print(text)
    if len(argv) > 1:
        open(argv[1], "w").write(text)
    return 0 if not bad else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
