#!/usr/bin/env python3
"""Reads the routed carrier PCB and writes board_features.json: every footprint's outline (courtyard-ish
bounding box), pad centroid and an assumed height above the board, for the enclosure model and its
interference check. Heights marked ASSUMED come from typical part drawings; MEASURED/DATASHEET where known.
PCB coordinates: x right, y down, mm, origin at the board's top-left corner.
"""
import json
import os
import re

import pcbnew

HERE = os.path.dirname(os.path.abspath(__file__))
PCB = os.path.join(HERE, "..", "..", "hardware", "kicad", "carrier", "carrier.kicad_pcb")

# height above the PCB top (mm) by footprint name pattern, first match wins
HEIGHTS = [
    (r"BatteryHolder_Keystone_1043", 21.0, "DATASHEET 18.0 holder (Keystone M65 p27) + cell standing proud, ASSUMED +3"),
    (r"BatteryHolder_Keystone_3002", 6.5, "ASSUMED CR2032 holder with cell"),
    (r"CP_Radial_D10", 13.5, "1000uF 10x12.5 + 1 mm leads"),
    (r"CP_Radial_D8", 12.5, "8x11.5 + 1 mm"),
    (r"CP_Radial_D6.3", 12.0, "6.3x11 + 1 mm"),
    (r"PinSocket_1x19", 8.5, "socket 8.5 (the DevKitC above it is modelled separately)"),
    (r"PinHeader", 11.5, "header 8.5 + Dupont housing 3 (cable plugs)"),
    (r"JST_XH", 7.0, "XH header 7.0 (cable plugs on top, +5)"),
    (r"SW_PUSH_6mm_H13mm", 13.0, "6x6x13 tactile switch (actuator top)"),
    (r"LED_D3.0mm", 5.3, "3 mm LED seated"),
    (r"Buzzer_12x9.5", 9.5, "12x9.5 buzzer"),
    (r"BarrelJack", 11.0, "DC jack body ~11 high, plug axis 6.5 above PCB (ASSUMED)"),
    (r"microSD", 2.0, "Hirose DM3AT 1.9"),
    (r"SOIC-16W", 2.65, "SOIC-16W"),
    (r"MountingHole", 0.0, "hole"),
    (r".*", 2.5, "small SMD (<= 2.5 mm)"),
]


def height_for(fpname):
    for pat, h, why in HEIGHTS:
        if re.search(pat, fpname):
            return h, why
    raise KeyError(fpname)


def extract():
    b = pcbnew.LoadBoard(PCB)
    out = {"board": {}, "parts": {}}
    bb = b.GetBoardEdgesBoundingBox()
    out["board"] = {"x0": pcbnew.ToMM(bb.GetX()), "y0": pcbnew.ToMM(bb.GetY()),
                    "w": pcbnew.ToMM(bb.GetWidth()), "h": pcbnew.ToMM(bb.GetHeight()), "thickness": 1.6}
    for f in b.GetFootprints():
        name = str(f.GetFPID().GetLibItemName())
        cbb = f.GetCourtyard(pcbnew.F_CrtYd).BBox() if f.GetCourtyard(pcbnew.F_CrtYd).OutlineCount() else f.GetBoundingBox(False)
        pads = [p.GetPosition() for p in f.Pads()]
        cx = sum(pcbnew.ToMM(p.x) for p in pads) / len(pads) if pads else pcbnew.ToMM(f.GetPosition().x)
        cy = sum(pcbnew.ToMM(p.y) for p in pads) / len(pads) if pads else pcbnew.ToMM(f.GetPosition().y)
        h, why = height_for(name)
        out["parts"][f.GetReference()] = {
            "footprint": name, "center": [round(cx, 3), round(cy, 3)],
            "bbox": [round(pcbnew.ToMM(v), 3) for v in (cbb.GetX(), cbb.GetY(), cbb.GetRight(), cbb.GetBottom())],
            "height": h, "height_basis": why, "rotation": f.GetOrientationDegrees()}
    return out


if __name__ == "__main__":
    data = extract()
    p = os.path.join(HERE, "board_features.json")
    json.dump(data, open(p, "w"), indent=1, sort_keys=True)
    print(f"{len(data['parts'])} parts -> {os.path.relpath(p)}")
