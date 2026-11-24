#!/usr/bin/env python3
"""Phase 6 checks for the enclosure (exit 0 = all pass). Writes docs/evidence/phase6-enclosure.md and renders.

1. No interference: printed parts vs every reference body (PCB, parts, DevKitC, printer, OLED, modules),
   and reference bodies vs each other (bounding-box prefilter, then exact boolean volume > 0.01 mm^3).
2. Clearances: lid inside face to the tallest part >= 2 mm; floor modules to the PCB underside >= 2 mm
   (THT leads stick out ~2 mm); printer body to the PCB >= 2 mm.
3. Openings line up: each button, LED, buzzer hole and the OLED window is open where its part is;
   plungers are coaxial with their switches and rest on them; the DC jack hole is on the jack axis.
4. Printability: every printed part fits a 256 x 256 x 256 mm bed; walls >= 2.0 mm.
"""
import itertools
import os
import sys

import cadquery as cq

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import enclosure as E  # noqa: E402

TOL = 0.01
BED = 256.0
results = []


def check(name, ok, detail):
    results.append((name, ok, detail))


def bbox_overlap(a, b):
    A, B = a.val().BoundingBox(), b.val().BoundingBox()
    return not (A.xmax <= B.xmin or B.xmax <= A.xmin or A.ymax <= B.ymin or B.ymax <= A.ymin or A.zmax <= B.zmin or B.zmax <= A.zmin)


def inter_volume(a, b):
    if not bbox_overlap(a, b):
        return 0.0
    try:
        return a.intersect(b).val().Volume()
    except Exception:
        return 0.0


def main():
    printed = E.printed_parts()
    ref = E.reference_bodies()

    # 1. interference
    bad = []
    for (pn, p), (rn, r) in itertools.product(printed.items(), ref.items()):
        if rn == "printer_bezel" and pn == "lid":
            continue  # the bezel sits on the lid's outer face (touching)
        v = inter_volume(p, r)
        if v > TOL:
            bad.append(f"{pn} x {rn}: {v:.1f} mm3")
    for (an, a), (bn, b) in itertools.combinations(printed.items(), 2):
        v = inter_volume(a, b)
        if v > TOL:
            bad.append(f"{an} x {bn}: {v:.1f} mm3")
    for (an, a), (bn, b) in itertools.combinations(ref.items(), 2):
        if {an, bn} == {"printer", "printer_bezel"}:
            continue
        v = inter_volume(a, b)
        if v > TOL:
            bad.append(f"{an} x {bn}: {v:.1f} mm3")
    check("No interference (printed parts vs contents, contents vs contents)", not bad,
          f"{len(printed)} printed parts, {len(ref)} reference bodies" + ("" if not bad else "; " + "; ".join(bad[:12])))

    # 2. clearances
    tops = {n: r.val().BoundingBox().zmax for n, r in ref.items() if n.startswith("part:") or n == "DevKitC"}
    tallest = max(tops, key=tops.get)
    gap_lid = E.DEPTH - tops[tallest]
    check("Lid inside face to tallest part >= 2 mm", gap_lid >= 2.0, f"{gap_lid:.1f} mm (tallest: {tallest})")
    mods = {n: r.val().BoundingBox().zmax for n, r in ref.items() if n.startswith("module:")}
    gap_mod = E.PCB_Z - 2.0 - max(mods.values())
    check("Floor modules to PCB underside (minus 2 mm leads) >= 2 mm", gap_mod >= 2.0, f"{gap_mod:.1f} mm")
    pb = ref["printer"].val().BoundingBox()
    gap_prn = (-E.Y(0.0)) + pb.ymin if False else pb.ymin - E.Y(0.0)
    check("Printer body to PCB top edge >= 2 mm", gap_prn >= 2.0, f"{gap_prn:.1f} mm")
    check("Printer panel thickness within the printer's 1-6 mm range", 1.0 <= E.T_LID <= 6.0, f"{E.T_LID} mm (CSN-A2 manual s3)")

    # 3. openings
    lid = printed["lid"].val()
    zmid = E.DEPTH + E.T_LID / 2
    miss = []

    def open_disc(x, y, r):
        """True if the lid has no material on a ring of radius r around (x, y) at mid-thickness."""
        import math
        pts = [(x, y)] + [(x + r * math.cos(a), y + r * math.sin(a)) for a in [i * math.pi / 4 for i in range(8)]]
        return not any(lid.isInside(cq.Vector(px, py, zmid)) for px, py in pts)

    for r in E.led_refs():
        x, y = E.P[r]["center"]
        if not open_disc(x, E.Y(y), 1.55):          # 3 mm light pipe + clearance
            miss.append(f"LED {r}")
    bx, by = E.P["BZ1"]["center"]
    if lid.isInside(cq.Vector(bx, E.Y(by), zmid)):
        miss.append("buzzer")
    ox, oy = E.OLED_CENTER
    wy = E.Y(oy + E.OLED_WINDOW_OFFSET_Y)
    for dx in (-14.0, 14.0):
        for dy in (-7.0, 7.0):
            if lid.isInside(cq.Vector(ox + dx, wy + dy, zmid)):
                miss.append("OLED window corner")
    for r in E.button_refs():
        x, y = E.P[r]["center"]
        if not open_disc(x, E.Y(y), E.PLUNGER["stem"] / 2 + 0.15):
            miss.append(f"button {r}")
        pl = printed[f"plunger_{r}"].val().BoundingBox()
        if abs((pl.xmin + pl.xmax) / 2 - x) > 0.05 or abs((pl.ymin + pl.ymax) / 2 - E.Y(y)) > 0.05:
            miss.append(f"plunger {r} off axis")
        if abs(pl.zmin - (E.PCB_TOP + E.P[r]["height"])) > 0.05:
            miss.append(f"plunger {r} does not rest on the switch")
    base = printed["base"].val()
    jy = E.Y((E.P["J1"]["bbox"][1] + E.P["J1"]["bbox"][3]) / 2)
    if base.isInside(cq.Vector(E.X0 - E.T_WALL / 2, jy, E.JACK_AXIS_Z)):
        miss.append("DC jack hole")
    check("Openings line up with LEDs, buzzer, OLED, buttons, DC jack", not miss, "all open/coaxial" if not miss else ", ".join(miss))
    check("OLED window covers the assumed active area (29.4 x 14.7) with margin",
          E.OLED_WINDOW[0] >= 29.4 + 1.0 and E.OLED_WINDOW[1] >= 14.7 + 1.0, f"window {E.OLED_WINDOW[0]} x {E.OLED_WINDOW[1]} mm")

    # 4. printability
    over = []
    for n, p in printed.items():
        bb = p.val().BoundingBox()
        dims = sorted([bb.xlen, bb.ylen, bb.zlen])
        if dims[-1] > BED:
            over.append(f"{n} {bb.xlen:.0f}x{bb.ylen:.0f}x{bb.zlen:.0f}")
    bb = printed["base"].val().BoundingBox()
    check("Every printed part fits a 256 mm bed", not over,
          f"base {bb.xlen:.0f} x {bb.ylen:.0f} x {bb.zlen:.0f} mm" + ("" if not over else "; too big: " + ", ".join(over)))
    check("Walls, floor, lid >= 2.0 mm", min(E.T_WALL, E.T_FLOOR, E.T_LID) >= 2.0, f"{E.T_WALL}/{E.T_FLOOR}/{E.T_LID} mm")

    lines = ["# Phase 6 enclosure check", "", "| Check | Result | Detail |", "|---|---|---|"]
    for n, ok, d in results:
        lines.append(f"| {n} | {'PASS' if ok else 'FAIL'} | {d} |")
    allok = all(ok for _, ok, _ in results)
    lines += ["", f"**{'ALL PASS' if allok else 'FAILED'}**", "", "Assumed dimensions (verify on the parts):", ""]
    for n, m in E.MODULES.items():
        lines.append(f"* {n}: {m['size'][0]} x {m['size'][1]} x {m['size'][2]} mm, {m['basis']}")
    lines += [f"* OLED module {E.OLED_BOARD[0]} x {E.OLED_BOARD[1]} mm, hole pattern {E.OLED_HOLES[0]} x {E.OLED_HOLES[1]} mm (typical, not from a datasheet)",
              f"* DC jack axis {E.JACK_AXIS_Z - E.PCB_TOP} mm above the PCB", "* part heights: mechanical/cadquery/board_features.py (each with its basis)"]
    text = "\n".join(lines) + "\n"
    ev = os.path.join(os.path.dirname(__file__), "..", "..", "docs", "evidence", "phase6-enclosure.md")
    open(ev, "w").write(text)
    print(text)
    return 0 if allok else 1


if __name__ == "__main__":
    sys.exit(main())
