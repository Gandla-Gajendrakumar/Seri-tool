#!/usr/bin/env python3
"""Renders the enclosure to SVG (front with lid, open box with contents) and PNG via headless Chromium."""
import os
import subprocess
import sys

import cadquery as cq

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import enclosure as E  # noqa: E402

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "docs", "evidence", "phase6")


def comp(items):
    return cq.Compound.makeCompound([i.val() for i in items])


def main():
    os.makedirs(OUT, exist_ok=True)
    printed = E.printed_parts()
    ref = E.reference_bodies()
    views = {
        "front": (comp([printed["lid"], ref["printer_bezel"]] + [printed[f"plunger_{r}"] for r in E.button_refs()]),
                  (0, 0, 1)),
        "open_box": (comp([printed["base"], ref["PCB"], ref["DevKitC"]] + [v for k, v in ref.items()
                           if k.startswith(("part:", "module:"))] + [ref["printer"]]), (-1.0, -1.6, 1.3)),
        "lid_inside": (comp([printed["lid"], ref["OLED"], ref["OLED_cable"], ref["printer"]]
                            + [printed[f"plunger_{r}"] for r in E.button_refs()]), (0.6, -1.0, -1.0)),
    }
    pngs = []
    for name, (shape, d) in views.items():
        svg = os.path.join(OUT, f"{name}.svg")
        cq.exporters.export(cq.Workplane().add(shape), svg, opt={
            "width": 1200, "height": 1000, "marginLeft": 40, "marginTop": 40, "projectionDir": d,
            "showAxes": False, "showHidden": False, "strokeWidth": 0.4})
        pngs.append((svg, svg[:-4] + ".png"))
    js = "const { chromium } = require('playwright');(async()=>{const b=await chromium.launch();const p=await b.newPage({viewport:{width:1200,height:1000}});"
    for svg, png in pngs:
        js += f"await p.goto('file://{os.path.abspath(svg)}');await p.waitForTimeout(300);await p.screenshot({{path:'{png}',omitBackground:false}});"
    js += "await b.close();})();"
    subprocess.run(["node", "-e", js], check=True)
    print("rendered:", ", ".join(os.path.basename(p) for _, p in pngs))


if __name__ == "__main__":
    main()
