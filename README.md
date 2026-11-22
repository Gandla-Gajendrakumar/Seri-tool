# Seri-tool

Low-cost climate monitor for silkworm rearing sheds in India: temperature and humidity
24×7, SMS alerts in Telugu / Kannada / Hindi / English, thermal-printer log, Wi-Fi / 4G upload,
battery backup. Designed and verified in software before any hardware is bought.

## Layout

| Path | Contents |
|---|---|
| `firmware/` | PlatformIO project. `lib/core` = pure, PC-testable logic. `lib/hal` = hardware interfaces. `test/` = unit tests. |
| `sim/wokwi/` | Wokwi virtual hardware: diagram, custom chips (A7670C, printer, SHT31), scenarios. |
| `sim/replay/` | Whole-rearing-cycle replay data and expected outputs. |
| `hardware/kicad/` | Carrier PCB schematic and layout. |
| `hardware/spice/` | ngspice power-stage simulations. |
| `hardware/power_budget/` | Battery-life calculator. |
| `mechanical/cadquery/` | Enclosure and component models. |
| `bom/` | Bill of materials with Indian suppliers. |
| `docs/` | Requirements, decisions, questions, datasheet index, hand-off pack. |
| `tools/` | Helper scripts (`check_toolchain.sh`). |

## Quick check

```
tools/check_toolchain.sh        # exercises every tool; exit code = failures
cd firmware && pio test -e native
```

## Status

| Phase | State | Evidence |
|---|---|---|
| 0 Setup | Approved 2026-11-21 | `tools/check_toolchain.sh`, `docs/requirements.md` |
| 1 Logic | Approved 2026-11-22 | `tools/phase1_gate.sh` → `docs/evidence/phase1-gate.txt` |
| 2 Firmware on virtual hardware | Approved 2026-11-22 (all 8 Wokwi scenarios) | `docs/evidence/phase2-gate.md` |
| 3 Whole-cycle replay | Gate passed, awaiting review | `tools/phase3_gate.sh` → `docs/evidence/phase3-gate.md` |
