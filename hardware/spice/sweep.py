#!/usr/bin/env python3
"""Sweeps the power-stage netlist over battery state, pack size, printer peak current and whether the
printer overlaps a modem burst, and checks each case against the datasheet limits:
  modem VBAT >= 3.40 V   (A7670 HD V1.03 p26)
  3.3 V rail  >= 3.00 V  (ESP32-WROOM-32E v2.1 p28)
  cell current <= 3.9 A per cell (Great Power 18650 spec, 1.5C continuous)
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
LIMITS = {"modem": 3.40, "v3v3": 3.00, "cell_a": 3.9}


def run(vocv, cells, iprn, overlap):
    net = BASE
    net = re.sub(r"\.param VOCV=\S+ RCELL=\{[^}]*\}", f".param VOCV={vocv} RCELL={{0.060/{cells}}}", net)
    net = re.sub(r"IPRN=\S+", f"IPRN={iprn}", net)
    if not overlap:  # printer runs only between modem activity: start heating after the burst train
        net = net.replace("Vdots  ndots 0 PULSE(0 1 1.5m 10u 10u 3m 4m)", "Vdots  ndots 0 PULSE(0 1 40m 10u 10u 3m 4m)")
    with tempfile.NamedTemporaryFile("w", suffix=".cir", delete=False) as f:
        f.write(net)
    out = subprocess.run(["ngspice", "-b", f.name], capture_output=True, text=True).stdout
    os.unlink(f.name)
    m = re.search(r"RESULT vmodem_min=(\S+) vsys_min=(\S+) v3v3_min=(\S+) ibat_min=(\S+)", out)
    if not m:
        raise RuntimeError(out[-800:])
    vm, vs, v3, ib = (float(x) for x in m.groups())
    cell_a = -ib / cells
    ok = vm >= LIMITS["modem"] and v3 >= LIMITS["v3v3"] and cell_a <= LIMITS["cell_a"]
    return vm, v3, cell_a, ok


# The design envelope the board must meet (D-038/D-039): firmware keeps printing and modem bursts apart on
# battery, and stops the modem below 3.60 V. Overlapping cases are reported to justify that interlock.
ENVELOPE = lambda vocv, cells, iprn, overlap: (not overlap) and vocv >= 3.6


def main(argv):
    rows = ["| Cell OCV | Cells | Printer peak | Printer during modem burst | Modem min | 3V3 min | Per-cell A | Result |",
            "|---|---|---|---|---|---|---|---|"]
    results = {}
    for vocv, cells, iprn, overlap in itertools.product((3.6, 3.8, 4.0), (2, 4), (1.0, 1.5, 2.0), (True, False)):
        vm, v3, ca, ok = run(vocv, cells, iprn, overlap)
        results[(vocv, cells, iprn, overlap)] = ok
        rows.append(f"| {vocv:.1f} V | {cells} | {iprn:.1f} A | {'yes' if overlap else 'no'} | {vm:.2f} V | {v3:.2f} V | "
                    f"{ca:.2f} | {'PASS' if ok else 'FAIL'} |")
    bad = [k for k, ok in results.items() if ENVELOPE(*k) and not ok]
    n = sum(1 for k in results if ENVELOPE(*k))
    verdict = (f"\nDesign envelope (interlock on, battery >= 3.60 V, 2 or 4 cells, printer up to 2 A): "
               f"{n - len(bad)}/{n} cases pass -> {'PASS' if not bad else 'FAIL'}\n")
    text = "\n".join(rows) + "\n" + verdict
    print(text)
    if len(argv) > 1:
        open(argv[1], "w").write(text)
    return 0 if not bad else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
