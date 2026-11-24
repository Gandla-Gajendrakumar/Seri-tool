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


def average_ma(loads, on_battery, override=None):
    total = 0.0
    for name, spec in loads.items():
        if on_battery and spec.get("on_battery", True) is False:
            continue
        ma = float((override or {}).get(name, spec["ma"]))
        total += ma * float(spec["duty"])
    return total


def runtime_hours(capacity_mah, cfg, override=None, usable=None):
    if usable is None:
        usable = float(cfg["battery"]["usable_fraction"]["value"])
    return capacity_mah * usable / average_ma(cfg["loads"], on_battery=True, override=override)


def assumed(cfg):
    """Values with no datasheet number behind them: they must be measured on the bench."""
    return [f"loads.{n}" for n, s in cfg["loads"].items() if "ASSUMED" in str(s.get("source", ""))]


def meets_target(cfg):
    t = cfg["target"]
    return runtime_hours(float(t["pack_mah"]), cfg) >= float(t["hours"])


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
    if "usable_fraction_monitor" in cfg["battery"]:
        um = cfg["battery"]["usable_fraction_monitor"]
        lines += ["", f"Monitoring, logging and the screen keep running after the modem stops at 3.60 V, down to "
                  f"{um['cutoff']}: {um['value']:.0%} usable ({um['source']}). Extra monitoring-only time: "
                  + ", ".join(f"{label} +{runtime_hours(float(cap), cfg, usable=um['value'] - cfg['battery']['usable_fraction']['value']):.0f} h"
                              for label, cap in (("2 cells", cfg["battery"]["pack_fitted_mah"]["value"]),
                                                 ("4 cells", cfg["battery"]["pack_max_mah"]["value"])))]
    a = assumed(cfg)
    if a:
        fitted = float(cfg["battery"]["pack_fitted_mah"]["value"])
        lines += ["", "## Assumed values (no datasheet number): bench-measure these", "",
                  "Sensitivity: runtime on the fitted 2-cell pack if each assumed current is double.", "",
                  "| Load | Assumed (mA) | If doubled: 2-cell runtime |", "|---|---|---|"]
        for key in a:
            n = key.split(".", 1)[1]
            ma = float(cfg["loads"][n]["ma"])
            lines.append(f"| {n} | {ma:g} | {runtime_hours(fitted, cfg, override={n: 2 * ma}):.0f} h |")
        allx2 = {k.split('.', 1)[1]: 2 * float(cfg['loads'][k.split('.', 1)[1]]['ma']) for k in a}
        lines.append(f"| **all assumed values doubled at once** | | **{runtime_hours(fitted, cfg, override=allx2):.0f} h** |")
    if "target" in cfg:
        t = cfg["target"]
        h = runtime_hours(float(t["pack_mah"]), cfg)
        lines += ["", f"Target ({t['source']}): >= {t['hours']} h on {t['pack_mah']} mAh. Result {h:.0f} h -> "
                  f"**{'PASS' if h >= t['hours'] else 'FAIL'}**"]
    return "\n".join(lines) + "\n", not p


def main(argv):
    import yaml
    cfg = yaml.safe_load(open(argv[1]))
    text, final = report(cfg)
    if len(argv) > 2:
        open(argv[2], "w").write(text)
    print(text)
    if not final:
        return 3
    return 0 if ("target" not in cfg or meets_target(cfg)) else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
