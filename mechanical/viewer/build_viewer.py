#!/usr/bin/env python3
"""Builds the interactive 3D viewer's data from the real design files (run after any board/enclosure change).

Outputs (mechanical/viewer/site/): scene.json (every board part + off-board modules, positions in the
enclosure frame: x right, y up, z towards the viewer, mm), enclosure.json (CadQuery base, lid, plungers),
board.json (KiCad board with copper, pads, mask and silkscreen) and schematic.svg (KiCad). Meshes are compact
JSON because the artifact host serves .json but not .stl/.glb.
"""
import json
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..", "..")
SITE = os.path.join(HERE, "site")
sys.path.insert(0, os.path.join(ROOT, "mechanical", "cadquery"))
sys.path.insert(0, os.path.join(ROOT, "hardware", "kicad", "gen"))

import base64  # noqa: E402
import struct  # noqa: E402

import cadquery as cq  # noqa: E402
import numpy as np  # noqa: E402
import pcbnew  # noqa: E402

import carrier  # noqa: E402
import enclosure as E  # noqa: E402

PCB = os.path.join(ROOT, "hardware", "kicad", "carrier", "carrier.kicad_pcb")

# What each part does, in plain words (shown when a part is clicked). Passives get a generic text.
ROLES = {
    "J1": "12 V DC input jack (adapter)", "F1": "Resettable fuse, 1.5 A", "D1": "Reverse-polarity protection diode",
    "D2": "Surge clamp (TVS) on the 12 V input", "C1": "12 V input smoothing capacitor",
    "J4": "To the 12 V to 5 V buck module (input)", "J5": "From the buck module (5 V out)",
    "U2": "Battery charger (TP4056, 500 mA)", "D3": "Red LED: battery charging", "JP1": "Charger temperature check jumper",
    "TH1": "Optional battery temperature sensor (not fitted)", "U3": "Battery protection chip (DW01A)",
    "Q1": "Battery protection switch (FS8205A)", "Q2": "Battery protection switch (FS8205A)", "Q11": "Battery protection switch (FS8205A)",
    "BT1": "18650 cell 1 (fitted)", "BT2": "18650 cell 2 (fitted)", "BT3": "18650 cell 3 (option)", "BT4": "18650 cell 4 (option)",
    "D4": "Mains-to-system diode (power path)", "Q3": "Battery-to-system switch (power path)", "Q12": "Battery-to-system switch (power path)",
    "C9": "Modem burst capacitor 1000 uF", "C26": "Modem burst capacitor 1000 uF",
    "U4": "3.3 V regulator for the ESP32 and sensors (AP2112K)", "JP2": "3.3 V jumper: remove before plugging in USB",
    "Q4": "Printer power switch (on only while printing on battery)", "Q5": "Printer power switch driver",
    "J6": "To the 5 V boost module (input)", "J7": "From the boost module (5 V for the printer)",
    "C20": "Printer supply capacitor 470 uF", "J8": "Printer power cable", "J9": "Printer data cable (TX, RX, GND)",
    "J10": "Harness to the 4G modem (A7670C)", "JP3": "Modem logic level jumper (1.8 V / 3.3 V)",
    "Q6": "Level shifter: ESP32 to modem", "Q7": "Level shifter: modem to ESP32", "Q8": "Modem power-key driver",
    "J2": "ESP32 DevKitC socket (left row)", "J3": "ESP32 DevKitC socket (right row)",
    "J11": "Temperature/humidity probe cable (SHT31, 2 m)", "J12": "Optional second temperature probe (DS18B20)",
    "U5": "Real-time clock (DS3231)", "BT5": "Clock backup coin cell (CR2032)", "J13": "microSD card: event log and upload queue",
    "J14": "Cable to the OLED screen on the lid", "SW1": "Button UP", "SW2": "Button OK (also silences the buzzer)", "SW3": "Button DOWN",
    "D5": "Green LED: all OK", "D6": "Red LED: alarm or fault", "BZ1": "Alarm buzzer", "D7": "Buzzer protection diode",
    "Q9": "Buzzer driver", "Q10": "Relay output driver (future fan/heater control)", "J15": "Relay header (future control)",
}
GROUP = {  # which subsystem lights up in the working demo
    "mains": ["J1", "F1", "D1", "D2", "C1", "C2", "J4", "J5", "C4", "R1", "R2", "C3", "D4", "C8", "module:buck"],
    "charge": ["U2", "R3", "C5", "C6", "D3", "R5", "JP1"],
    "battery": ["BT1", "BT2", "U3", "R6", "C7", "R7", "Q1", "Q2", "Q11", "Q3", "Q12", "R8", "R9", "R10", "C18"],
    "rail3v3": ["U4", "C16", "C17", "JP2"],
    "modem": ["J10", "C9", "C26", "C10", "C11", "C12", "C13", "C14", "C15", "JP3", "Q6", "Q7", "Q8", "R15", "R16", "R17", "R18",
              "R19", "R20", "R21", "module:modem"],
    "printer": ["Q4", "R11", "Q5", "R12", "R13", "C19", "J6", "J7", "C20", "J8", "J9", "R14", "module:boost", "printer"],
    "sensor": ["J11", "R22", "R23", "R24", "R25", "C21", "probe"],
    "esp": ["J2", "J3", "devkitc"],
    "ui": ["SW1", "SW2", "SW3", "D5", "D6", "R32", "R33", "BZ1", "D7", "Q9", "R34", "R35", "J14", "oled"],
    "store": ["U5", "C22", "C23", "BT5", "J13", "R27", "R28", "R29", "R30", "R31", "C24", "C25"],
}


# ---- compact geometry: artifact pages serve .json but not .stl/.glb, so meshes are shipped as JSON with
# positions quantised to 0.01 mm (int16, +-327 mm) and uint32 indices, base64-encoded; normals are rebuilt in the browser.
Q = 100.0


def _pack(pos_mm, idx, name, color, opacity=1.0):
    pos_mm = np.asarray(pos_mm, dtype=np.float64)
    assert np.abs(pos_mm).max() < 327, name
    q = np.round(pos_mm * Q).astype("<i2")
    return {"name": name, "color": color, "opacity": round(float(opacity), 3), "n": int(len(q)),
            "pos": base64.b64encode(q.tobytes()).decode(), "idx": base64.b64encode(np.asarray(idx, dtype="<u4").tobytes()).decode()}


def _dedupe(tri):
    flat = np.round(tri.reshape(-1, 3), 3)
    uniq, inv = np.unique(flat, axis=0, return_inverse=True)
    return uniq, inv.reshape(-1)


def stl_mesh(path, name, color, opacity=1.0):
    data = open(path, "rb").read()
    n = struct.unpack("<I", data[80:84])[0]
    rec = np.frombuffer(data, dtype=np.dtype([("n", "<f4", 3), ("v", "<f4", (3, 3)), ("a", "<u2")]), count=n, offset=84)
    pos, idx = _dedupe(rec["v"].astype(np.float64))
    return _pack(pos, idx, name, color, opacity)


def glb_meshes(path, z0):
    """KiCad GLB (metres, y = board thickness, z = PCB y down) -> enclosure frame (mm): (x, -z, y) + z0."""
    data = open(path, "rb").read()
    jlen = struct.unpack("<I", data[12:16])[0]
    j = json.loads(data[20:20 + jlen])
    boff = 20 + jlen + 8
    binbuf = data[boff:]
    def acc(i):
        a = j["accessors"][i]; bv = j["bufferViews"][a["bufferView"]]
        dt = {5126: "<f4", 5125: "<u4", 5123: "<u2", 5121: "u1"}[a["componentType"]]
        nc = {"SCALAR": 1, "VEC3": 3, "VEC2": 2}[a["type"]]
        start = bv.get("byteOffset", 0) + a.get("byteOffset", 0)
        arr = np.frombuffer(binbuf, dtype=dt, count=a["count"] * nc, offset=start)
        return arr.reshape(-1, nc) if nc > 1 else arr
    groups = {}
    for m in j["meshes"]:
        for pr in m["primitives"]:
            mat = j["materials"][pr.get("material", 0)]
            c = mat.get("pbrMetallicRoughness", {}).get("baseColorFactor", [0.5, 0.5, 0.5, 1])
            key = tuple(round(v, 3) for v in c)
            p = acc(pr["attributes"]["POSITION"]).astype(np.float64)
            i = acc(pr["indices"]).astype(np.int64) if "indices" in pr else np.arange(len(p))
            g = groups.setdefault(key, [[], [], 0])
            g[0].append(p); g[1].append(i + g[2]); g[2] += len(p)
    out = []
    for k, (ps, iss, _) in groups.items():
        p = np.concatenate(ps) * 1000.0
        p = np.stack([p[:, 0], -p[:, 2], p[:, 1] + z0], axis=1)
        tri = p[np.concatenate(iss)].reshape(-1, 3, 3)
        pos, idx = _dedupe(tri)
        col = "#%02x%02x%02x" % tuple(int(round(min(1, v) * 255)) for v in k[:3])
        out.append(_pack(pos, idx, "pcb", col, k[3]))
    return out


def kind_of(fp):
    rules = [("R_0805", "resistor"), ("C_0805", "mlcc"), ("C_1206", "mlcc"), ("CP_Radial", "ecap"), ("SOIC", "ic"),
             ("TSSOP", "ic"), ("SOT-23", "sot"), ("D_SMA", "diode"), ("SOD-123", "diode"), ("LED_D3", "led"),
             ("SW_PUSH", "switch"), ("Buzzer", "buzzer"), ("BarrelJack", "jack"), ("microSD", "sd"), ("PinSocket", "socket"),
             ("PinHeader", "header"), ("JST_XH", "jst"), ("Keystone_1043", "holder"), ("Keystone_3002", "coin"),
             ("Fuse", "fuse"), ("SolderJumper", "jumper"), ("MountingHole", "hole")]
    for k, v in rules:
        if k in fp:
            return v
    return "ic"


def Y(pcb_y):
    return -pcb_y


def bbox_of(wp):
    b = wp.val().BoundingBox()
    return [round(b.xmin, 2), round(b.ymin, 2), round(b.zmin, 2), round(b.xmax, 2), round(b.ymax, 2), round(b.zmax, 2)]


def main():
    os.makedirs(SITE, exist_ok=True)
    feats = json.load(open(os.path.join(ROOT, "mechanical", "cadquery", "board_features.json")))
    board = pcbnew.LoadBoard(PCB)
    values = {p[2]: (p[3], p[6]) for p in carrier.PARTS}
    parts = []
    for ref, p in sorted(feats["parts"].items()):
        kind = kind_of(p["footprint"])
        if kind == "hole":
            continue
        x0, y0, x1, y1 = p["bbox"]
        value, fields = values.get(ref, ("", {}))
        fp = board.FindFootprintByReference(ref)
        pads = [[round(pcbnew.ToMM(pd.GetPosition().x), 2), round(Y(pcbnew.ToMM(pd.GetPosition().y)), 2)]
                for pd in fp.Pads() if pd.GetNumber()] if kind in ("header", "socket", "jst") else []
        m = re.search(r"CP_Radial_D([0-9.]+)mm", p["footprint"])
        role = ROLES.get(ref) or {"resistor": f"Resistor {value}", "mlcc": f"Capacitor {value}", "ecap": f"Capacitor {value}",
                                  "sot": f"Transistor {value}", "diode": f"Diode {value}"}.get(kind, value)
        parts.append({"ref": ref, "value": value, "kind": kind, "role": role, "dnp": fields.get("dnp") == "yes",
                      "box": [x0, Y(y1), x1, Y(y0)], "h": p["height"], "rot": p["rotation"],
                      "center": [p["center"][0], Y(p["center"][1])], "pads": pads,
                      "dia": float(m.group(1)) if m else None,
                      "group": next((g for g, refs in GROUP.items() if ref in refs), None)})
    ref = E.reference_bodies()
    off = {
        "devkitc": {"box": bbox_of(ref["DevKitC"]), "role": "ESP32-DevKitC (the brain): reads the sensor, decides alarms, sends SMS"},
        "printer": {"box": bbox_of(ref["printer"]), "bezel": bbox_of(ref["printer_bezel"]), "role": "58 mm thermal printer (CSN-A2): prints every alert and a daily summary"},
        "oled": {"box": bbox_of(ref["OLED"]), "role": "1.3 inch OLED screen on the lid: readings in the farmer's language"},
        "oled_cable": {"box": bbox_of(ref["OLED_cable"])},
        "module:modem": {"box": bbox_of(ref["module:modem_breakout"]), "role": "4G modem (A7670C): SMS alerts and data upload"},
        "module:buck": {"box": bbox_of(ref["module:buck_xl4015"]), "role": "12 V to 5 V buck module (XL4015)"},
        "module:boost": {"box": bbox_of(ref["module:boost_module"]), "role": "5 V boost module for the printer (TPS61023)"},
    }
    gland_x = feats["parts"]["J11"]["center"][0]
    scene = {
        "units": "mm", "frame": "x right, y up (wall), z towards the viewer; PCB y is -y",
        "pcb": {"z": E.PCB_Z, "t": E.PCB_T, "w": feats["board"]["w"], "h": feats["board"]["h"]},
        "enclosure": {"depth": E.DEPTH, "lid_t": E.T_LID, "inner": [E.X0, E.Y(E.PCB_Y_BOT), E.X1, E.Y(E.PCB_Y_TOP)],
                      "wall": E.T_WALL, "floor": E.T_FLOOR, "gland": [gland_x, E.Y(E.PCB_Y_BOT) - E.T_WALL, 15.0],
                      "window": [E.OLED_CENTER[0], E.Y(E.OLED_CENTER[1] + E.OLED_WINDOW_OFFSET_Y), *E.OLED_WINDOW]},
        "parts": parts, "offboard": off, "groups": list(GROUP),
        "firmware": "v0.3.1", "source": "hardware/kicad/carrier + mechanical/cadquery (routed PCB and enclosure)",
    }
    json.dump(scene, open(os.path.join(SITE, "scene.json"), "w"), separators=(",", ":"))
    printed = E.printed_parts()
    plungers = printed["plunger_SW1"].union(printed["plunger_SW2"]).union(printed["plunger_SW3"])
    tmpd = os.path.join(SITE, "_tmp")
    os.makedirs(tmpd, exist_ok=True)
    enc = []
    for name, wp, tol, color in (("base", printed["base"], 0.2, "#dfe5e2"), ("lid", printed["lid"], 0.2, "#dfe5e2"),
                                 ("plunger", plungers, 0.1, "#8a2c63")):
        f = os.path.join(tmpd, name + ".stl")
        cq.exporters.export(wp, f, tolerance=tol, angularTolerance=0.3)
        enc.append(stl_mesh(f, name, color))
    json.dump({"meshes": enc, "q": Q}, open(os.path.join(SITE, "enclosure.json"), "w"), separators=(",", ":"))
    glb = os.path.join(tmpd, "pcb.glb")
    subprocess.run(["kicad-cli", "pcb", "export", "glb", "--board-only", "--include-tracks", "--include-pads",
                    "--include-silkscreen", "--include-soldermask", "--user-origin", "0x0mm", "-f", "-o", glb, PCB],
                   check=True, capture_output=True)
    json.dump({"meshes": glb_meshes(glb, E.PCB_Z), "q": Q}, open(os.path.join(SITE, "board.json"), "w"), separators=(",", ":"))
    shutil.rmtree(tmpd)
    for old_file in ("base.stl", "lid.stl", "plunger.stl", "pcb.glb"):
        if os.path.exists(os.path.join(SITE, old_file)):
            os.remove(os.path.join(SITE, old_file))
    tmp = os.path.join(SITE, "_sch")
    subprocess.run(["kicad-cli", "sch", "export", "svg", "--exclude-drawing-sheet", "-o", tmp,
                    os.path.join(ROOT, "hardware", "kicad", "carrier", "carrier.kicad_sch")], check=True, capture_output=True)
    svg = [f for f in os.listdir(tmp) if f.endswith(".svg")][0]
    txt = open(os.path.join(tmp, svg), encoding="utf-8").read()
    txt = re.sub(r"<!DOCTYPE[^>]*>", "", txt, count=1)   # the artifact host refuses XML files with DTDs
    open(os.path.join(SITE, "schematic.svg"), "w", encoding="utf-8").write(txt)
    shutil.rmtree(tmp)
    sizes = {f: os.path.getsize(os.path.join(SITE, f)) for f in os.listdir(SITE)}
    print(f"{len(parts)} board parts, {len(off)} off-board bodies ->", ", ".join(f"{k} {v / 1e6:.2f} MB" for k, v in sizes.items()))


if __name__ == "__main__":
    main()
