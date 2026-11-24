#!/usr/bin/env python3
"""Seri-tool v1 enclosure (phase 6), built with CadQuery from the routed PCB (board_features.json).

Frame: X = PCB x (mm, right), Y = -PCB y (up), Z = up from the inside of the base floor (Z = 0) to the
inside of the front lid (Z = DEPTH). The lid is the front face the farmer sees; the base screws to the wall.

Layout (front view): thermal printer at the top, carrier PCB below it on four posts, OLED + 3 buttons +
2 LEDs + buzzer at the PCB's lower right. Modem breakout, buck and boost modules sit on the base floor
behind the PCB (D-056, D-057). Every number that is not from a datasheet is labelled ASSUMED.
"""
import json
import os

import cadquery as cq

HERE = os.path.dirname(os.path.abspath(__file__))
F = json.load(open(os.path.join(HERE, "board_features.json")))
P = F["parts"]

# ---------------------------------------------------------------- parameters
T_WALL, T_FLOOR, T_LID = 2.5, 3.0, 3.0      # mm; >= 2.0 for FDM PETG/ASA
R_CORNER = 4.0
X0, X1 = -5.5, 200.0                        # inner X: 0.5 mm beyond the DC jack front; DevKitC antenna clear on the right
PCB_Y_TOP, PCB_Y_BOT = -80.0, 162.0         # inner extent in PCB y (printer zone above; 10 mm below the board for the lid screw bosses)
DEPTH = 58.0                                # floor (Z=0) to lid inside face
PCB_Z = 30.4                                # underside of the PCB (post height)
PCB_T = F["board"]["thickness"]
PCB_TOP = PCB_Z + PCB_T

# CSN-A2 panel printer (CSN-A2 manual v4.0 s1, s3: bezel 111 x 65, cut-out 103 x 57, embedded depth 50, roll <= 39 mm)
PRN_CUT = (103.0, 57.0)
PRN_BEZEL = (111.0, 65.0)
PRN_EMBED = 50.0
PRN_CENTER = (98.5, -41.0)                  # PCB coords

# 1.3" SH1106 OLED module on the lid (ASSUMED typical module: board 35.4 x 33.5, active 29.4 x 14.7,
# M2 holes 2 mm in from the corners, header along the top edge). Window is generous; verify on the real module.
OLED_CENTER = (158.0, 104.0)                # board centre, PCB coords
OLED_BOARD = (35.4, 33.5)
OLED_WINDOW = (31.0, 17.0)
OLED_WINDOW_OFFSET_Y = -3.0                 # window centre relative to board centre, in PCB y (towards the header)
OLED_POST_H = 2.0                           # board front sits this far behind the lid
OLED_HOLES = (31.4, 29.5)

# floor-mounted modules (ASSUMED envelopes until the parts are in hand; double-sided VHB pads)
MODULES = {
    "modem_breakout": {"center": (82.0, 62.0), "size": (45.0, 45.0, 16.0), "basis": "ASSUMED A7670C breakout incl. SIM + u.FL"},
    "buck_xl4015": {"center": (38.0, 25.0), "size": (51.5, 26.5, 14.0), "basis": "ASSUMED common XL4015 module"},
    "boost_module": {"center": (163.0, 25.0), "size": (40.0, 30.0, 12.0), "basis": "ASSUMED 5 V boost module"},
}

PLUNGER = {"hole": 7.0, "stem": 6.4, "flange": 10.0, "flange_t": 1.5, "proud": 1.0}
LED_PIPE_HOLE = 3.2                         # 3 mm acrylic light pipe from LED top to the lid face
JACK_HOLE = 10.0
JACK_AXIS_Z = PCB_TOP + 6.5                 # ASSUMED DC-005 jack axis height
GLAND_HOLE = 12.5                           # PG7 cable gland for the probe
VENT_HOLE = 12.2                            # M12 breathable membrane vent (pressure equalisation, no condensation pumping)
INSERT_HOLE, INSERT_DEPTH = 4.0, 6.0        # M3 heat-set inserts

INNER_W = X1 - X0
INNER_H = PCB_Y_BOT - PCB_Y_TOP


def Y(pcb_y):
    return -pcb_y


def cyl(origin, direction, r, length):
    return cq.Workplane("XY").add(cq.Solid.makeCylinder(r, length, cq.Vector(*origin), cq.Vector(*direction)))


def inner_center():
    return ((X0 + X1) / 2, Y((PCB_Y_TOP + PCB_Y_BOT) / 2))


def button_refs():
    return [r for r in ("SW1", "SW2", "SW3")]


def led_refs():
    return [r for r in ("D5", "D6")]


def lid_screw_points():
    cx, cy = inner_center()
    dx, dy = INNER_W / 2 - 5.0, INNER_H / 2 - 5.0
    return [(cx + sx * dx, cy + sy * dy) for sx in (-1, 1) for sy in (-1, 1)]


def pcb_post_points():
    return [(P[h]["center"][0], Y(P[h]["center"][1])) for h in ("H1", "H2", "H3", "H4")]


# ---------------------------------------------------------------- base
def base():
    cx, cy = inner_center()
    ow, oh = INNER_W + 2 * T_WALL, INNER_H + 2 * T_WALL
    outer = (cq.Workplane("XY").workplane(offset=-T_FLOOR).center(cx, cy)
             .rect(ow, oh).extrude(DEPTH + T_FLOOR).edges("|Z").fillet(R_CORNER))
    cavity = (cq.Workplane("XY").center(cx, cy).rect(INNER_W, INNER_H).extrude(DEPTH + 1)
              .edges("|Z").fillet(max(R_CORNER - T_WALL, 0.5)))
    b = outer.cut(cavity)
    # PCB posts with M3 insert holes
    for x, y in pcb_post_points():
        post = cq.Workplane("XY").center(x, y).circle(3.5).extrude(PCB_Z)
        post = post.cut(cq.Workplane("XY").workplane(offset=PCB_Z - INSERT_DEPTH).center(x, y).circle(INSERT_HOLE / 2).extrude(INSERT_DEPTH))
        b = b.union(post)
    # lid screw bosses in the corners (full height), M3 inserts at the top
    for x, y in lid_screw_points():
        boss = cq.Workplane("XY").center(x, y).circle(4.5).extrude(DEPTH)
        boss = boss.cut(cq.Workplane("XY").workplane(offset=DEPTH - INSERT_DEPTH - 2).center(x, y).circle(INSERT_HOLE / 2).extrude(INSERT_DEPTH + 2))
        b = b.union(boss)
    # DC jack hole in the left wall
    jy = Y((P["J1"]["bbox"][1] + P["J1"]["bbox"][3]) / 2)
    b = b.cut(cyl((X0 - T_WALL - 1, jy, JACK_AXIS_Z), (1, 0, 0), JACK_HOLE / 2, T_WALL + 2))
    # probe cable gland and breather vent in the bottom wall
    for x, d in ((P["J11"]["center"][0], GLAND_HOLE), (60.0, VENT_HOLE)):
        b = b.cut(cyl((x, Y(PCB_Y_BOT) + 1, 15.0), (0, -1, 0), d / 2, T_WALL + 2))
    # keyholes in the floor for wall mounting (8 mm head, 4 mm slot)
    for x in (30.0, 170.0):
        y = Y(-70.0)
        k = cq.Workplane("XY").workplane(offset=-T_FLOOR - 1).center(x, y).circle(4.0).extrude(T_FLOOR + 2)
        k = k.union(cq.Workplane("XY").workplane(offset=-T_FLOOR - 1).center(x, y + 5).slot2D(10.0, 4.0, 90).extrude(T_FLOOR + 2))
        b = b.cut(k)
    b = b.cut(cq.Workplane("XY").workplane(offset=-T_FLOOR - 1).center(100.0, Y(150.0)).circle(2.2).extrude(T_FLOOR + 2))
    return b


# ---------------------------------------------------------------- lid
def lid():
    cx, cy = inner_center()
    ow, oh = INNER_W + 2 * T_WALL, INNER_H + 2 * T_WALL
    l = (cq.Workplane("XY").workplane(offset=DEPTH).center(cx, cy).rect(ow, oh).extrude(T_LID)
         .edges("|Z").fillet(R_CORNER))
    # locating tongue that drops 3 mm inside the walls (splash seal; IP rating not claimed)
    r_in = max(R_CORNER - T_WALL, 0.5)        # the base cavity's corner radius: the tongue corners follow it
    tongue = (cq.Workplane("XY").workplane(offset=DEPTH - 3.0).center(cx, cy).rect(INNER_W - 0.4, INNER_H - 0.4).extrude(3.0)
              .edges("|Z").fillet(r_in)
              .cut(cq.Workplane("XY").workplane(offset=DEPTH - 3.0).center(cx, cy).rect(INNER_W - 0.4 - 2.4, INNER_H - 0.4 - 2.4).extrude(3.0)))
    l = l.union(tongue)
    # clear the corner bosses through the tongue, then screw holes
    for x, y in lid_screw_points():
        l = l.cut(cq.Workplane("XY").workplane(offset=DEPTH - 3.0).center(x, y).circle(5.0).extrude(3.0))
        l = l.cut(cq.Workplane("XY").workplane(offset=DEPTH - 1).center(x, y).circle(1.7).extrude(T_LID + 2))
        l = l.cut(cq.Workplane("XY").workplane(offset=DEPTH + T_LID - 1.8).center(x, y).circle(3.0).extrude(2))
    # printer cut-out
    l = l.cut(cq.Workplane("XY").workplane(offset=DEPTH - 1).center(PRN_CENTER[0], Y(PRN_CENTER[1]))
              .rect(*PRN_CUT).extrude(T_LID + 2))
    # OLED window + mounting posts
    ox, oy = OLED_CENTER
    l = l.cut(cq.Workplane("XY").workplane(offset=DEPTH - 1).center(ox, Y(oy + OLED_WINDOW_OFFSET_Y))
              .rect(*OLED_WINDOW).extrude(T_LID + 2))
    for sx in (-1, 1):
        for sy in (-1, 1):
            x, y = ox + sx * OLED_HOLES[0] / 2, Y(oy) + sy * OLED_HOLES[1] / 2
            post = cq.Workplane("XY").workplane(offset=DEPTH - OLED_POST_H).center(x, y).circle(2.5).extrude(OLED_POST_H)
            l = l.union(post.cut(cq.Workplane("XY").workplane(offset=DEPTH - OLED_POST_H).center(x, y).circle(0.9).extrude(OLED_POST_H + 1)))
    # button holes, LED light-pipe holes, buzzer sound holes
    for r in button_refs():
        x, y = P[r]["center"]
        l = l.cut(cq.Workplane("XY").workplane(offset=DEPTH - 1).center(x, Y(y)).circle(PLUNGER["hole"] / 2).extrude(T_LID + 2))
    for r in led_refs():
        x, y = P[r]["center"]
        l = l.cut(cq.Workplane("XY").workplane(offset=DEPTH - 1).center(x, Y(y)).circle(LED_PIPE_HOLE / 2).extrude(T_LID + 2))
    bx, by = P["BZ1"]["center"]
    for dx, dy in ((0, 0), (3.5, 0), (-3.5, 0), (0, 3.5), (0, -3.5)):
        l = l.cut(cq.Workplane("XY").workplane(offset=DEPTH - 1).center(bx + dx, Y(by) + dy).circle(1.0).extrude(T_LID + 2))
    return l


def plunger(ref):
    x, y = P[ref]["center"]
    sw_top = PCB_TOP + P[ref]["height"]
    stem = cq.Workplane("XY").workplane(offset=sw_top).center(x, Y(y)).circle(PLUNGER["stem"] / 2).extrude(DEPTH + T_LID + PLUNGER["proud"] - sw_top)
    flange = (cq.Workplane("XY").workplane(offset=DEPTH - PLUNGER["flange_t"]).center(x, Y(y))
              .circle(PLUNGER["flange"] / 2).extrude(PLUNGER["flange_t"]))
    return stem.union(flange)


# ---------------------------------------------------------------- reference bodies (not printed)
def box(x0, y0_pcb, x1, y1_pcb, z0, z1):
    """Axis box from PCB-coordinate corners."""
    return (cq.Workplane("XY").workplane(offset=z0).center((x0 + x1) / 2, Y((y0_pcb + y1_pcb) / 2))
            .rect(abs(x1 - x0), abs(y1_pcb - y0_pcb)).extrude(z1 - z0))


def reference_bodies():
    """Everything that must fit inside: PCB, every part as its courtyard box, DevKitC, printer, OLED, floor modules."""
    out = {}
    b = F["board"]
    out["PCB"] = box(b["x0"], b["y0"], b["x0"] + b["w"], b["y0"] + b["h"], PCB_Z, PCB_TOP)
    for ref, p in P.items():
        if p["height"] <= 0:
            continue
        x0, y0, x1, y1 = p["bbox"]
        out[f"part:{ref}"] = box(x0, y0, x1, y1, PCB_TOP, PCB_TOP + p["height"])
    # DevKitC on its sockets: board 48.3 x 27.9 (Espressif drawing), antenna 6.04 beyond the board end at pin 1
    j2y, j3y = P["J2"]["center"][1], P["J3"]["center"][1]
    pin1_x = max(P["J2"]["bbox"][2], P["J3"]["bbox"][2]) - 1.8
    out["DevKitC"] = box(pin1_x - 45.72 - 1.29, (j2y + j3y) / 2 - 13.97, pin1_x + 1.29 + 6.04, (j2y + j3y) / 2 + 13.97,
                         PCB_TOP + 8.5, PCB_TOP + 14.0)
    cx, cy = PRN_CENTER
    out["printer"] = box(cx - PRN_CUT[0] / 2, cy - PRN_CUT[1] / 2, cx + PRN_CUT[0] / 2, cy + PRN_CUT[1] / 2,
                         DEPTH + T_LID - PRN_EMBED, DEPTH + T_LID)
    out["printer_bezel"] = box(cx - PRN_BEZEL[0] / 2, cy - PRN_BEZEL[1] / 2, cx + PRN_BEZEL[0] / 2, cy + PRN_BEZEL[1] / 2,
                               DEPTH + T_LID, DEPTH + T_LID + 2.0)
    ox, oy = OLED_CENTER
    z_board = DEPTH - OLED_POST_H - 1.6
    out["OLED"] = box(ox - OLED_BOARD[0] / 2, oy - OLED_BOARD[1] / 2, ox + OLED_BOARD[0] / 2, oy + OLED_BOARD[1] / 2,
                      z_board, DEPTH - OLED_POST_H)
    # header + Dupont housings hanging back from the module's top edge (ASSUMED 2.5 + 14 mm)
    out["OLED_cable"] = box(ox - 6.0, oy - OLED_BOARD[1] / 2 + 0.5, ox + 6.0, oy - OLED_BOARD[1] / 2 + 3.5,
                            z_board - 16.5, z_board)
    for name, m in MODULES.items():
        (mx, my), (w, h, t) = m["center"], m["size"]
        out[f"module:{name}"] = box(mx - w / 2, my - h / 2, mx + w / 2, my + h / 2, 0.0, t)
    return out


def printed_parts():
    parts = {"base": base(), "lid": lid()}
    for r in button_refs():
        parts[f"plunger_{r}"] = plunger(r)
    return parts


if __name__ == "__main__":
    import sys
    out = os.path.join(HERE, "out")
    os.makedirs(out, exist_ok=True)
    parts = printed_parts()
    for n, p in parts.items():
        cq.exporters.export(p, os.path.join(out, f"{n}.step"))
        cq.exporters.export(p, os.path.join(out, f"{n}.stl"))
    print("exported:", ", ".join(parts))
    sys.exit(0)
