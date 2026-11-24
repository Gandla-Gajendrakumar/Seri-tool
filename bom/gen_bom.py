#!/usr/bin/env python3
"""Bill of materials for the Seri-tool pilot build (phase 5).

Board parts come straight from the schematic netlist (hardware/kicad/gen/carrier.py), so the BOM cannot
drift from the design. Off-board parts (modules, cells, printer...) are listed in OFFBOARD below.
Prices and links live in bom/sources.yaml (researched; each price says where it came from).

Usage: gen_bom.py [units]      writes bom/bom.md and bom/bom.csv; exit 0 = every line priced, 3 = not final.
"""
import csv
import math
import os
import sys
from collections import OrderedDict

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "hardware", "kicad", "gen"))

# schematic (symbol, value) -> research item name in sources.yaml
ITEM_BY_PART = {
    ("R", None): "Resistor 0805 1% (per value)",
    ("C", "100nF"): "Capacitor 0805 X7R 100nF 50V",
    ("C", "1uF"): "Capacitor 0805 X5R/X7R 1uF 25V",
    ("C", "4.7uF"): "Capacitor 0805 X5R 4.7uF 10V",
    ("C", "33pF"): "Capacitor 0805 C0G 33pF and 10pF",
    ("C", "10pF"): "Capacitor 0805 C0G 33pF and 10pF",
    ("C", "10uF"): "Capacitor 1206 X5R 10uF 16V",
    ("C", "22uF"): "Capacitor 1206 X5R 22uF 10V",
    ("C", "22uF 10V"): "Capacitor 1206 X5R 22uF 10V",
    ("C_Polarized", "100uF 25V"): "Electrolytic 100uF 25V radial 8 mm, 3.5 mm pitch",
    ("C_Polarized", "100uF 10V"): "Electrolytic 100uF 10V radial 6.3 mm, 2.5 mm pitch",
    ("C_Polarized", "470uF 10V"): "Electrolytic 470uF 10V radial 8 mm, 3.5 mm pitch",
    ("C_Polarized", "1000uF 6.3V low-ESR"): "Electrolytic 1000uF 6.3V LOW-ESR radial 10 mm, 5 mm pitch",
    ("TP4056", None): "TP4056 [SOP-8 with exposed pad, ESOP-8]",
    ("DW01A", None): "DW01A [SOT-23-6]",
    ("FS8205A", None): "FS8205A [TSSOP-8]",
    ("AO3401A", None): "AO3401A [SOT-23]",
    ("MMBT3904", None): "MMBT3904 [SOT-23]",
    ("SS34", None): "SS34 [SMA / DO-214AC]",
    ("SMAJ15A", None): "SMAJ15A [SMA]",
    ("1N4148W", None): "1N4148W [SOD-123]",
    ("AP2112K-3.3", None): "AP2112K-3.3 [SOT-23-5]",
    ("DS3231SN", None): "DS3231SN [SOIC-16 wide]",
    ("Polyfuse", None): "Polyfuse 1.5 A hold [1812]",
    ("Battery_Cell", "18650"): "18650 PCB holder, single cell, THT [Keystone 1043]",
    ("Battery_Cell", "CR2032"): "CR2032 PCB holder [Keystone 3002 or equivalent]",
    ("Micro_SD_Card_Det_Hirose_DM3AT", None): "microSD socket [Hirose DM3AT-SF-PEJM5 or equivalent push-push with card detect]",
    ("Barrel_Jack_Switch", None): "DC barrel jack 5.5x2.1 mm, PCB horizontal with switch pin",
    ("Buzzer", None): "Active buzzer 5 V, 12 mm, 7.6 mm pin pitch",
    ("SW_Push", None): "Tactile switch 6x6 mm, 13 mm height, THT",
    ("LED", None): "LED 3 mm red / green, THT",
    ("Thermistor_NTC", None): "NTC 10k B3950 (optional)",
    ("Conn_01x19", None): "Female header 1x19 2.54 mm",
    ("Conn_01x08", None): "Female header 1x8 and 1x4 2.54 mm",
    ("Jumper_2_Open", None): "Male header strip 1x40 2.54 mm + jumper shunt",
}
# connectors whose part depends on the footprint
ITEM_BY_FOOTPRINT = {
    "JST_XH": "JST XH 2.54/2.50 mm headers + housings + crimp (2, 3, 4 pin)",
    "PinHeader_1x": "Male header strip 1x40 2.54 mm + jumper shunt",
    "PinSocket_1x04": "Female header 1x8 and 1x4 2.54 mm",
}

# off-board parts per unit: (item, qty, note)
OFFBOARD = [
    ("ESP32-DevKitC V4 (WROOM-32E)", 1, "plugs into J2/J3"),
    ("A7670C 4G LTE Cat-1 breakout", 1, "plugs into J10 (match pin order before ordering PCBs)"),
    ("SHT31 probe, 2 m cable", 1, "to J11; ~1 m probe extended to 2 m (D-052)"),
    ("Shielded twisted-pair cable, 2-pair 24 AWG (per metre)", 2, "probe extension, metres"),
    ("58 mm TTL thermal printer with status TX", 1, "to J8/J9"),
    ("1.3 inch OLED SH1106 I2C 128x64", 1, "plugs into J14"),
    ("18650 Li-ion cell 2500-3000 mAh (genuine brand)", 2, "BT1, BT2 (footprints for 4)"),
    ("12 V 3 A DC adapter, BIS registered", 1, "to J1"),
    ("XL4015 buck module 5 A", 1, "wired to J4/J5"),
    ("5 V boost module >= 2 A from 3.0-4.2 V", 1, "wired to J6/J7"),
    ("microSD card 8-16 GB (industrial or reputable)", 1, "J13"),
    ("CR2032 coin cell", 1, "BT5"),
    ("57 mm thermal paper roll", 2, "consumable: first two rolls"),
    ("PCB fabrication: 2-layer, 190 x 152 mm, 1.6 mm FR4, 1 oz, HASL, qty 5 and qty 10", 1, "one board"),
    # enclosure (phase 6): quantities per unit; pack sizes in sources.yaml
    ("PETG or ASA 3D printer filament 1 kg (1.75 mm)", 0.6, "kg: base 304 + lid 139 cm3 = 565 g solid PETG (own printer)"),
    ("M3 brass heat-set inserts (M3 x 5-6 mm), pack", 8, "4 PCB posts + 4 lid bosses"),
    ("M3 x 8 mm screws (pan/countersunk), pack", 8, "lid x 4 (M3 x 8), PCB x 4 (M3 x 6 from the same kit)"),
    ("M2 x 5 mm self-tapping screws, pack", 4, "OLED on the lid posts"),
    ("Acrylic light pipe rod 3 mm diameter", 2, "LEDs D5/D6 to the lid"),
    ("PG7 nylon cable gland (IP68, 3-6.5 mm cable)", 1, "probe cable"),
    ("M12 breathable membrane vent (IP67/IP68, protective vent)", 1, "bottom wall"),
    ("Dupont jumper cable female-female, 4-wire ribbon, 15-20 cm", 4, "wires: OLED to J14"),
    ("Dupont jumper cable female-female, 8-wire, 10 cm", 8, "wires: modem breakout to J10 (D-057)"),
    ("3M VHB double-sided tape 12-25 mm", 0.012, "rolls (0.3 m of a 25 m roll): modem, buck, boost on the floor"),
]
NOT_COSTED = [("IoT/M2M SIM (Airtel or Vi)", "recurring operator charge, not hardware"),
              ("3D printing labour / service", "own printer assumed; a print service costs about Rs 3-10 per gram, i.e. Rs 1,700-5,700 per enclosure")]


def load_sources(path=os.path.join(HERE, "sources.yaml")):
    import yaml
    out = OrderedDict()
    for e in yaml.safe_load(open(path)) or []:
        out[e["item"]] = e
    return out


def item_for(sym, value, fp):
    for key, item in ITEM_BY_FOOTPRINT.items():
        if key in fp:
            return item
    return ITEM_BY_PART.get((sym, value)) or ITEM_BY_PART.get((sym, None))


def carrier_lines():
    import carrier
    lines = OrderedDict()
    for lib, sym, ref, value, fp, nets, f in carrier.PARTS:
        if ref.startswith("#") or f.get("in_bom") == "no":
            continue
        item = item_for(sym, value, fp)
        if item is None:
            raise KeyError(f"no BOM item for {ref} {sym} {value} {fp}")
        key = (item, value)
        ln = lines.setdefault(key, {"item": item, "value": value, "qty": 0, "refs": [], "dnp_refs": [],
                                    "footprint": fp.split(":")[1]})
        if f.get("dnp") == "yes":
            ln["dnp_refs"].append(ref)
        else:
            ln["qty"] += 1
            ln["refs"].append(ref)
    out = []
    for ln in lines.values():
        ln["refs"] = " ".join(ln["refs"])
        ln["dnp_refs"] = " ".join(ln["dnp_refs"])
        out.append(ln)
    return out


def pick_source(entry):
    """The source marked `selected: true` (chosen on spec, recorded with a reason), else the cheapest
    in-stock priced source, Indian suppliers first."""
    priced = [s for s in entry.get("sources", []) if s.get("pack_price_inr") not in (None, "")]
    chosen = [s for s in priced if s.get("selected")]
    if chosen:
        return chosen[0]
    priced = [s for s in priced if s.get("in_stock") is not False]
    if not priced:
        return None
    india = [s for s in priced if s.get("country") == "IN"]
    pool = india or priced
    return min(pool, key=lambda s: float(s["pack_price_inr"]) / float(s.get("pack_qty") or 1))


def unit_inr(src):
    return float(src["pack_price_inr"]) / float(src.get("pack_qty") or 1)


def batch_inr(src, qty, units):
    packs = math.ceil(qty * units / float(src.get("pack_qty") or 1))
    return packs * float(src["pack_price_inr"])


def price_lines(lines, sources, units=10):
    rows, missing = [], []
    for ln in lines:
        entry = sources[ln["item"]]          # KeyError = item never researched
        src = pick_source(entry)
        row = dict(ln, mpn=entry.get("mpn", ""), manufacturer=entry.get("manufacturer", ""), source=src,
                   unit_inr=None, line_inr=None, batch_inr=None)
        if src is None or ln["qty"] == 0:
            if ln["qty"] and ln["item"] not in missing:
                missing.append(ln["item"])
        else:
            row["unit_inr"] = unit_inr(src)
            row["line_inr"] = row["unit_inr"] * ln["qty"]
            row["batch_inr"] = batch_inr(src, ln["qty"], units)
        rows.append(row)
    return rows, missing


def availability_problems(rows):
    out = []
    for r in rows:
        if r["qty"] and r["source"] and r["source"].get("in_stock") is False and r["item"] not in out:
            out.append(r["item"])
    return out


def offboard_lines():
    return [{"item": i, "value": "", "qty": q, "refs": n, "dnp_refs": "", "footprint": "off-board"} for i, q, n in OFFBOARD]


def fmt(v):
    return "—" if v is None else f"₹{v:,.2f}"


def report(units=10):
    sources = load_sources()
    board, mb = price_lines(carrier_lines(), sources, units)
    off, mo = price_lines(offboard_lines(), sources, units)
    missing = mb + [m for m in mo if m not in mb]
    avail = availability_problems(board + off)
    if not missing and not avail:
        status = "**Status: FINAL (every line priced and in stock at research time)**"
    else:
        status = "**Status: NOT FINAL**"
        if missing:
            status += f"\n\n* {len(missing)} line(s) without a price: " + "; ".join(missing)
        if avail:
            status += f"\n* {len(avail)} chosen source(s) out of stock at research time: " + "; ".join(avail)
    lines = [f"# Bill of materials: Seri-tool v1 pilot", "", status, "",
             "Prices in INR from Indian suppliers where found (LCSC otherwise, at 84 INR/USD). Most come from a "
             "web-search index on 2026-11-22, not a live checkout: confirm before ordering. Shipping not included. "
             "Sources and links: `bom/sources.yaml`.", ""]
    tot = {}
    for title, rows in (("Carrier board parts", board), ("Off-board parts", off)):
        lines += [f"## {title}", "",
                  "| Item | Value | Qty | Refs | MPN | Supplier | Unit | Per unit | Batch of " + str(units) + " |",
                  "|---|---|---|---|---|---|---|---|---|"]
        for r in rows:
            s = r["source"]
            sup = f"[{s['supplier']}]({s['url']})" if s else "—"
            refs = r["refs"] + (f" (DNP: {r['dnp_refs']})" if r["dnp_refs"] else "")
            lines.append(f"| {r['item']} | {r['value']} | {r['qty']} | {refs} | {r['mpn']} | {sup} | "
                         f"{fmt(r['unit_inr'])} | {fmt(r['line_inr'])} | {fmt(r['batch_inr'])} |")
        u = sum(r["line_inr"] or 0 for r in rows)
        b = sum(r["batch_inr"] or 0 for r in rows)
        tot[title] = (u, b)
        lines += ["", f"Subtotal: **{fmt(u)} per unit**, {fmt(b)} for {units} units (whole packs)", ""]
    u = sum(v[0] for v in tot.values())
    b = sum(v[1] for v in tot.values())
    lines += ["## Total", "", f"* Per unit (parts only): **{fmt(u)}**",
              f"* Pilot batch of {units} (whole packs, so leftover stock): **{fmt(b)}** ({fmt(b / units)} per unit)", "",
              "Not costed here: " + "; ".join(f"{i} ({why})" for i, why in NOT_COSTED), ""]
    return "\n".join(lines) + "\n", board + off, missing + avail


def main(argv):
    units = int(argv[1]) if len(argv) > 1 else 10
    text, rows, missing = report(units)
    open(os.path.join(HERE, "bom.md"), "w").write(text)
    with open(os.path.join(HERE, "bom.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["item", "value", "qty", "refs", "dnp_refs", "footprint", "manufacturer", "mpn", "supplier", "url",
                    "unit_inr", "line_inr", f"batch{units}_inr"])
        for r in rows:
            s = r["source"] or {}
            w.writerow([r["item"], r["value"], r["qty"], r["refs"], r["dnp_refs"], r["footprint"], r["manufacturer"],
                        r["mpn"], s.get("supplier", ""), s.get("url", ""),
                        "" if r["unit_inr"] is None else f"{r['unit_inr']:.2f}",
                        "" if r["line_inr"] is None else f"{r['line_inr']:.2f}",
                        "" if r["batch_inr"] is None else f"{r['batch_inr']:.2f}"])
    print(text)
    return 0 if not missing else 3


if __name__ == "__main__":
    sys.exit(main(sys.argv))
