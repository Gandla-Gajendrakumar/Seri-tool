#!/usr/bin/env python3
"""Battery-life budget for the Seri-tool carrier board (phase 4).

Every load is a current drawn from the battery rail with a duty cycle, and every number carries
its source (datasheet + page, or a stated design assumption). Loads marked `on_battery: false` only
run on mains (for example the charger's own draw). The report refuses to call itself final while any
source is still PENDING.

Usage: budget.py loads.yaml [report.md]
"""
import sys


def duty(seconds, every_seconds):
    return seconds / every_seconds


def average_ma(loads, on_battery):
    total = 0.0
    for spec in loads.values():
        if on_battery and spec.get("on_battery", True) is False:
            continue
        total += float(spec["ma"]) * float(spec["duty"])
    return total


def runtime_hours(capacity_mah, cfg):
    usable = float(cfg["battery"]["usable_fraction"]["value"])
    return capacity_mah * usable / average_ma(cfg["loads"], on_battery=True)


def pending(cfg):
    out = []
    for section in ("battery", "loads"):
        for name, spec in cfg.get(section, {}).items():
            if "PENDING" in str(spec.get("source", "")):
                out.append(f"{section}.{name}")
    return out


def report(cfg):
    lines = ["# Battery-life budget", ""]
    p = pending(cfg)
    lines.append("**Status: FINAL**" if not p else f"**Status: NOT FINAL - {len(p)} value(s) pending a datasheet: "
                 + ", ".join(p) + "**")
    lines += ["", "| Load | Current (mA) | Duty | Average (mA) | Source |", "|---|---|---|---|---|"]
    for name, s in cfg["loads"].items():
        avg = float(s["ma"]) * float(s["duty"])
        tag = "" if s.get("on_battery", True) else " (mains only)"
        lines.append(f"| {name}{tag} | {s['ma']} | {float(s['duty']):.4f} | {avg:.2f} | {s['source']} |")
    avg = average_ma(cfg["loads"], on_battery=True)
    lines += ["", f"Average battery current on battery: **{avg:.1f} mA**", ""]
    lines += ["| Pack | Capacity | Usable | Runtime |", "|---|---|---|---|"]
    for label, cap in (("2 x 18650 (fitted)", cfg["battery"]["pack_fitted_mah"]["value"]),
                       ("4 x 18650 (footprint)", cfg["battery"]["pack_max_mah"]["value"])):
        h = runtime_hours(float(cap), cfg)
        lines.append(f"| {label} | {cap} mAh | {cfg['battery']['usable_fraction']['value']:.0%} | "
                     f"**{h:.0f} h ({h / 24:.1f} days)** |")
    return "\n".join(lines) + "\n", not p


def main(argv):
    import yaml
    cfg = yaml.safe_load(open(argv[1]))
    text, final = report(cfg)
    if len(argv) > 2:
        open(argv[2], "w").write(text)
    print(text)
    return 0 if final else 3


if __name__ == "__main__":
    sys.exit(main(sys.argv))
