"""Writes the project symbol library (hardware/kicad/lib/seri.kicad_sym) for parts KiCad's stock
libraries do not have. Pin numbers and names come from the datasheets cited per symbol."""
from sexp import Q, dump

GRID = 2.54


def _pin(num, name, ptype, x, y, angle):
    return ["pin", ptype, "line", ["at", f"{x:g}", f"{y:g}", str(angle)], ["length", "2.54"],
            ["name", Q(name), ["effects", ["font", ["size", "1.27", "1.27"]]]],
            ["number", Q(num), ["effects", ["font", ["size", "1.27", "1.27"]]]]]


def ic(name, ref, left, right, datasheet, half_width=10.16):
    """left/right: lists of (number, name, type) from top to bottom; None leaves a gap."""
    rows = max(len(left), len(right))
    top = (rows - 1) * GRID / 2
    body = ["symbol", Q(f"{name}_0_1"),
            ["rectangle", ["start", f"{-half_width:g}", f"{top + GRID:g}"], ["end", f"{half_width:g}", f"{-top - GRID:g}"],
             ["stroke", ["width", "0.254"], ["type", "default"]], ["fill", ["type", "background"]]]]
    pins = ["symbol", Q(f"{name}_1_1")]
    for side, items in ((-1, left), (1, right)):
        for i, it in enumerate(items):
            if it is None:
                continue
            num, pname, ptype = it
            y = top - i * GRID
            x = side * (half_width + GRID)
            pins.append(_pin(num, pname, ptype, x, y, 0 if side < 0 else 180))
    eff = lambda hide=False: ["effects", ["font", ["size", "1.27", "1.27"]]] + ([["hide", "yes"]] if hide else [])
    return ["symbol", Q(name), ["pin_names", ["offset", "1.016"]], ["exclude_from_sim", "no"],
            ["in_bom", "yes"], ["on_board", "yes"],
            ["property", Q("Reference"), Q(ref), ["at", "0", f"{top + 2 * GRID:g}", "0"], eff()],
            ["property", Q("Value"), Q(name), ["at", "0", f"{-top - 2 * GRID:g}", "0"], eff()],
            ["property", Q("Footprint"), Q(""), ["at", "0", "0", "0"], eff(True)],
            ["property", Q("Datasheet"), Q(datasheet), ["at", "0", "0", "0"], eff(True)],
            body, pins]


SYMBOLS = [
    # TP4056 NJ REV_2.4 "PIN FUNCTIONS" p6; exposed pad = GND
    ic("TP4056", "U",
       [("4", "VCC", "power_in"), ("8", "CE", "input"), ("1", "TEMP", "input"), ("2", "PROG", "passive")],
       [("5", "BAT", "power_out"), ("7", "~{CHRG}", "open_collector"), ("6", "~{STDBY}", "open_collector"),
        ("3", "GND", "power_in"), ("9", "EP", "passive")],
       "TP4056 NJ REV_2.4 (docs/datasheets/facts.md s7)"),
    # DS3231 Rev 7 "Pin Description" p9 (SO-16): N.C. pins 5-12 "Must be connected to ground"
    ic("DS3231SN", "U",
       [("2", "VCC", "power_in"), ("14", "VBAT", "power_in"), ("15", "SDA", "bidirectional"), ("16", "SCL", "input"),
        ("4", "~{RST}", "bidirectional")],
       [("1", "32kHz", "open_collector"), ("3", "~{INT}/SQW", "open_collector"), ("13", "GND", "power_in")]
       + [(str(n), "NC_GND", "passive") for n in range(5, 13)],
       "DS3231 Rev 7 (docs/datasheets/facts.md s5)"),
    # FS8205A dual N-MOSFET, common drain, TSSOP-8 (docs/datasheets/facts.md addendum 2)
    ic("FS8205A", "Q",
       [("4", "G1", "input"), ("2", "S1", "passive"), ("3", "S1", "passive")],
       [("5", "G2", "input"), ("6", "S2", "passive"), ("7", "S2", "passive"), ("1", "D", "passive"), ("8", "D", "passive")],
       "FS8205A (docs/datasheets/facts.md addendum 2)", half_width=7.62),
]


def write(path):
    lib = ["kicad_symbol_lib", ["version", "20241209"], ["generator", Q("seri-tool-symgen")],
           ["generator_version", Q("9.0")]] + SYMBOLS
    open(path, "w", encoding="utf-8").write(dump(lib) + "\n")


if __name__ == "__main__":
    import os
    write(os.path.join(os.path.dirname(__file__), "..", "lib", "seri.kicad_sym"))
