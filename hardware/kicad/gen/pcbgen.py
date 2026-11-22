"""Builds the carrier PCB (2 layers, hand-solder) from the same netlist as the schematic (carrier.py),
then autoroutes it with Freerouting and pours ground. Usage:
  pcbgen.py place   -> carrier.kicad_pcb (placed, unrouted) + carrier.dsn
  pcbgen.py route   -> runs Freerouting on carrier.dsn, imports the routes, pours zones, saves
Freerouting: tools/.cache/freerouting.jar (downloaded by tools/phase4_gate.sh if missing).
"""
import os
import subprocess
import sys

import pcbnew

sys.path.insert(0, os.path.dirname(__file__))
import carrier  # noqa: E402
from schgen import Library, uid  # noqa: E402

LIB = Library([os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "lib")])

HERE = os.path.dirname(os.path.abspath(__file__))
OUTDIR = os.path.join(HERE, "..", "carrier")
PCB = os.path.join(OUTDIR, "carrier.kicad_pcb")
DSN = os.path.join(OUTDIR, "carrier.dsn")
SES = os.path.join(OUTDIR, "carrier.ses")
JAR = os.path.join(HERE, "..", "..", "..", "tools", ".cache", "freerouting.jar")
FPDIR = "/usr/share/kicad/footprints/"
W, H = 190.0, 152.0
SPACING = 1.5  # mm between packed parts: room for the autorouter

# ---- net classes: width / clearance in mm ----
CLASSES = {
    "Power": (0.8, 0.25, ["VIN_RAW", "VIN_F", "VIN_12V", "+5V_BUS", "VSYS", "+5V_PRN", "BOOST_IN", "BAT_P"]),
    "Rail3V3": (0.5, 0.2, ["+3V3", "ESP_3V3", "GND"]),
    "Cell": (0.35, 0.2, ["CELL_N", "PROT_D"]),   # TSSOP-8 pins; bulk current carried by the CELL_N pour
}
DEFAULT = (0.25, 0.2)

# ---- fixed placements: ref -> (x, y, rotation_deg, side) ; everything else is packed into regions ----
FIXED = {
    "J1": (9, 14, 0, "F"),
    "BT1": (15, 99, 90, "F"), "BT2": (41, 99, 90, "F"), "BT3": (67, 99, 90, "F"), "BT4": (93, 99, 90, "F"),
    # DevKitC: pin 1 at the antenna end, 1.6 mm from the edge. The antenna starts 1.29 mm past pin 1 and is
    # 6.04 mm long (Espressif DevKitC V4 dimension drawing), so it lies entirely beyond the carrier edge.
    "J2": (188.4, 57.0, 270, "F"), "J3": (188.4, 82.4, 270, "F"),
    "J10": (110.0, 52.0, 0, "F"),
    "JP2": (182.0, 50.0, 90, "F"),   # "remove before USB" jumper right next to the DevKitC 3V3 pin
    "H1": (5, 5, 0, "F"), "H2": (185, 5, 0, "F"), "H3": (5, 147, 0, "F"), "H4": (185, 147, 0, "F"),
}


# regions: (x0, y0, x1, y1, [refs]) packed left-to-right, top-to-bottom
def regions():
    return [
        (16, 3, 50, 46, ["F1", "D1", "D2", "C1", "C2", "R1", "R2", "C3", "J4", "J5", "C4"]),
        (51, 3, 84, 46, ["U2", "R3", "C5", "C6", "JP1", "R4", "TH1", "D3", "R5"]),
        (85, 3, 118, 46, ["U3", "R6", "C7", "R7", "Q1", "Q2", "Q11", "R9", "R10", "C18"]),
        (119, 3, 150, 46, ["D4", "Q3", "Q12", "R8", "C8", "U4", "C16", "C17"]),
        (151, 9, 186, 46, ["Q4", "R11", "Q5", "R12", "R13", "C19", "J6", "J7", "C20", "J8", "R14", "J9"]),
        (115, 49, 139, 71, ["JP3", "Q6", "R15", "R16", "Q7", "R17", "R18", "R19", "Q8", "R20", "R21"]),
        (106, 72, 138, 98, ["C9", "C26", "C10", "C11", "C12", "C13", "C14", "C15"]),
        (106, 99, 140, 124, ["J11", "R22", "R23", "R24", "R25", "C21", "J12", "R26", "Q10", "R36", "R37", "R38", "J15"]),
        (106, 125, 140, 150, ["BT5"]),
        (141, 86, 188, 98, ["R27", "R28", "R29", "R30", "R31", "C24", "C25", "C22", "C23", "D7", "Q9", "R34", "R35"]),
        (141, 99, 188, 124, ["J13", "U5", "J14"]),
        (141, 125, 182, 150, ["SW1", "SW2", "SW3", "BZ1", "D5", "D6", "R32", "R33"]),
    ]


def mm(v):
    return pcbnew.FromMM(v)


def load(fp):
    lib, name = fp.split(":")
    m = pcbnew.FootprintLoad(FPDIR + lib + ".pretty", name)
    if m is None:
        raise FileNotFoundError(fp)
    return m


def courtyard(m):
    bb = m.GetBoundingBox(False)
    return pcbnew.ToMM(bb.GetWidth()) + SPACING, pcbnew.ToMM(bb.GetHeight()) + SPACING


def place_at(m, x, y, rot=0, side="F"):
    if side == "B":
        m.Flip(m.GetPosition(), pcbnew.FLIP_DIRECTION_TOP_BOTTOM)
    m.SetOrientationDegrees(rot)
    # position so that the courtyard's top-left lands at the packing cursor is done by the caller
    m.SetPosition(pcbnew.VECTOR2I(mm(x), mm(y)))


def build():
    b = pcbnew.BOARD()
    nets = {}

    def net(name):
        name = name if name.startswith("unconnected-(") else "/" + name   # KiCad's name for a local label
        if name not in nets:
            n = pcbnew.NETINFO_ITEM(b, name)
            b.Add(n)
            nets[name] = n
        return nets[name]

    fps = {}
    for lib, sym, ref, value, fp, pinnets, fields in carrier.PARTS:
        if ref.startswith("#"):
            continue
        m = load(fp)
        m.SetReference(ref)
        m.SetValue(value)
        m.SetPath(pcbnew.KIID_PATH("/" + uid("carrier/" + ref)))   # links the footprint to its schematic symbol
        m.SetSheetname("Root")
        m.SetSheetfile("carrier.kicad_sch")
        m.SetFPIDAsString(fp)
        if fields.get("dnp") == "yes":
            m.SetDNP(True)
        pinnames = {k: v[3] for k, v in Library.pins(LIB.symbol(lib, sym)).items()} if sym != "MountingHole" else {}
        for p in m.Pads():
            num = p.GetNumber()
            n = pinnets.get(num)
            if n == "NC":   # same placeholder net name as KiCad's netlister, so schematic parity holds
                pn = pinnames[num].replace("/", "{slash}")
                n = f"unconnected-({ref}-Pad{num})" if pn == "~" else f"unconnected-({ref}-{pn}-Pad{num})"
            if n:
                p.SetNet(net(n))
        b.Add(m)
        fps[ref] = m

    placed = set()
    for ref, (x, y, rot, side) in FIXED.items():
        place_at(fps[ref], x, y, rot, side)
        placed.add(ref)
    for x0, y0, x1, y1, refs in regions():
        cx, cy, row = x0, y0, 0.0
        for ref in refs:
            if ref in placed:
                continue
            m = fps[ref]
            w, h = courtyard(m)
            if cx + w > x1:
                cx, cy, row = x0, cy + row, 0.0
            if cy + h > y1:
                raise RuntimeError(f"region {x0},{y0} full at {ref}")
            # move so that the bounding box's top-left is at (cx, cy)
            m.SetPosition(pcbnew.VECTOR2I(0, 0))
            bb = m.GetBoundingBox(False)
            m.SetPosition(pcbnew.VECTOR2I(mm(cx + 0.5) - bb.GetX(), mm(cy + 0.5) - bb.GetY()))
            cx += w
            row = max(row, h)
            placed.add(ref)
    missing = set(fps) - placed
    if missing:
        raise RuntimeError(f"unplaced: {sorted(missing)}")

    pts = [(0, 0), (W, 0), (W, H), (0, H), (0, 0)]
    for (x1, y1), (x2, y2) in zip(pts, pts[1:]):
        s = pcbnew.PCB_SHAPE(b)
        s.SetShape(pcbnew.SHAPE_T_SEGMENT)
        s.SetLayer(pcbnew.Edge_Cuts)
        s.SetStart(pcbnew.VECTOR2I(mm(x1), mm(y1)))
        s.SetEnd(pcbnew.VECTOR2I(mm(x2), mm(y2)))
        s.SetWidth(mm(0.1))
        b.Add(s)

    ds = b.GetDesignSettings()
    ns = ds.m_NetSettings
    d = ns.GetDefaultNetclass()
    d.SetTrackWidth(mm(DEFAULT[0]))
    d.SetClearance(mm(DEFAULT[1]))
    d.SetViaDiameter(mm(0.8))
    d.SetViaDrill(mm(0.4))
    for name, (w, c, members) in CLASSES.items():
        nc = pcbnew.NETCLASS(name)
        nc.SetTrackWidth(mm(w))
        nc.SetClearance(mm(c))
        nc.SetViaDiameter(mm(1.0 if w >= 0.5 else 0.8))
        nc.SetViaDrill(mm(0.5 if w >= 0.5 else 0.4))
        ns.SetNetclass(name, nc)
        for n in members:
            ns.SetNetclassPatternAssignment("/" + n, name)
    ns.RecomputeEffectiveNetclasses()
    ds.m_MinClearance = mm(0.2)
    ds.m_CopperEdgeClearance = mm(0.5)
    ds.m_MinResolvedSpokes = 1  # a pad with one thermal spoke plus its tracks is accepted
    # 0.6 mm keep-out ring for tracks/vias along the edge, so the autorouter honours the 0.5 mm edge clearance
    # (narrow enough that no edge pad reaches into it, or Freerouting would refuse to connect that pad)
    E = 0.6
    for x0, y0, x1, y1 in ((0, 0, W, E), (0, H - E, W, H), (0, 0, E, H), (W - E, 0, W, H)):
        k = pcbnew.ZONE(b)
        k.SetIsRuleArea(True)
        k.SetDoNotAllowTracks(True)
        k.SetDoNotAllowVias(True)
        k.SetDoNotAllowPads(False)
        k.SetDoNotAllowFootprints(False)
        k.SetLayerSet(pcbnew.LSET.AllCuMask())
        ol = k.Outline()
        ol.NewOutline()
        for x, y in ((x0, y0), (x1, y0), (x1, y1), (x0, y1)):
            ol.Append(mm(x), mm(y))
        b.Add(k)
    ds.m_TrackMinWidth = mm(0.2)
    b.BuildConnectivity()
    for n in b.GetNetInfo().NetsByName().values():
        pass
    return b


def place():
    os.makedirs(OUTDIR, exist_ok=True)
    b = build()
    b.Save(PCB)
    ok = pcbnew.ExportSpecctraDSN(b, DSN)
    print(f"placed {len(b.GetFootprints())} footprints; DSN export: {ok}")


def pour(b, gnd_only=False, fill=True):
    def zone(netname, layer, poly, priority=0, clearance=0.3):
        z = pcbnew.ZONE(b)
        z.SetLayer(layer)
        z.SetNet(b.FindNet("/" + netname))
        ol = z.Outline()
        ol.NewOutline()
        for x, y in poly:
            ol.Append(mm(x), mm(y))
        z.SetLocalClearance(mm(clearance))
        z.SetMinThickness(mm(0.25))
        z.SetAssignedPriority(priority)
        z.SetPadConnection(pcbnew.ZONE_CONNECTION_THERMAL)
        z.SetThermalReliefSpokeWidth(mm(0.5))
        z.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_ALWAYS)
        b.Add(z)
    full = [(0.5, 0.5), (W - 0.5, 0.5), (W - 0.5, H - 0.5), (0.5, H - 0.5)]
    # the WROOM antenna lies wholly beyond the board edge (see FIXED), so the ground pour can be full
    if not any(z.GetNetname() == "/GND" for z in b.Zones()):
        for layer in (pcbnew.F_Cu, pcbnew.B_Cu):
            zone("GND", layer, full, 0)
    if gnd_only:
        return
    # battery current paths on the top layer: cells -> protection, and cells -> modem bulk caps
    zone("BAT_P", pcbnew.F_Cu, [(2, 48), (136, 48), (136, 99), (105, 99), (105, 146), (2, 146)], 1)
    if fill:
        pcbnew.ZONE_FILLER(b).Fill(b.Zones())


def _freeroute(passes):
    cmd = ["java", "-jar", JAR, "-de", DSN, "-do", SES, "-mp", str(passes), "-mt", "1", "--gui.enabled=false"]
    if os.path.exists(SES):
        os.unlink(SES)
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=3600)
    with open(os.path.join(OUTDIR, "freerouting.log"), "a") as f:
        f.write(r.stdout + r.stderr)
    if not os.path.exists(SES):
        raise SystemExit("Freerouting produced no session file; see freerouting.log")


def _unconnected(b):
    b.BuildConnectivity()
    return b.GetConnectivity().GetUnconnectedCount(False)


def route(passes=40, rounds=1):
    """Route, then re-run Freerouting on the routed board (it keeps existing wires) until nothing is left."""
    if not os.path.exists(JAR):
        raise SystemExit(f"missing {JAR}")
    open(os.path.join(OUTDIR, "freerouting.log"), "w").close()
    b = pcbnew.LoadBoard(PCB)
    for i in range(rounds):
        pcbnew.ExportSpecctraDSN(b, DSN)
        _freeroute(passes)
        b = pcbnew.LoadBoard(PCB) if i == 0 else b
        if not pcbnew.ImportSpecctraSES(b, SES):
            raise SystemExit("SES import failed")
        left = _unconnected(b)
        print(f"round {i + 1}: {left} unconnected")
        if left == 0:
            break
    pour(b)
    b.Save(PCB)
    print(f"routed: {len(b.GetTracks())} track/via items, unconnected after pour: {_unconnected(b)}")


def all_steps():
    """Place and route; Freerouting's result depends on the pass budget, so try a few until complete."""
    for passes in (100, 150, 60, 200):
        place()
        route(passes=passes)
        if _unconnected(pcbnew.LoadBoard(PCB)) == 0:
            print(f"complete with {passes} passes")
            return 0
    print("still unconnected after all attempts")
    return 1


if __name__ == "__main__":
    sys.exit({"place": place, "route": route, "all": all_steps}[sys.argv[1]]() or 0)
