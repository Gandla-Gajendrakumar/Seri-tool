# Seri-tool v1: hand-off pack

A low-cost silkworm-shed climate monitor: temperature and humidity 24x7, SMS alerts in English, Hindi,
Telugu and Kannada (works with no cloud), a 58 mm printed log, Wi-Fi/4G upload (MQTT over TLS) and battery backup.
Everything here was designed and checked in software before any hardware was bought (phases 0-6).
What software could not prove is listed in the bench test plan.

## What is in the pack

| Need | File |
|---|---|
| Order the parts | [ordering.md](ordering.md), `bom/bom.md`, `bom/bom.csv`, `bom/sources.yaml` |
| Make the PCB | `hardware/fab/carrier-v1-gerbers.zip`, `hardware/fab/carrier-v1-schematic.pdf`, `hardware/fab/carrier-v1-assembly-top.pdf`, `hardware/fab/carrier-v1.step` |
| Print the enclosure | `mechanical/cadquery/out/base.stl`, `mechanical/cadquery/out/lid.stl`, `mechanical/cadquery/out/plunger_SW1.stl` (x3), STEP files alongside |
| Assemble and wire | [assembly.md](assembly.md) (solder order, jumpers, harness tables) |
| Flash the firmware | `firmware/release/v0.3.0/flash.sh` (with SHA256SUMS) |
| First power-on | [bringup.md](bringup.md) |
| Bench and field tests | [bench-test-plan.md](bench-test-plan.md) (27 items, with pass levels and a record sheet) |
| Install in a shed | [installer-guide.md](installer-guide.md) |
| Farmer quick-reference card | [farmer-card.pdf](farmer-card.pdf) (one A5 page per language, generated from the firmware's own strings) |
| Requirements → evidence | [traceability.md](traceability.md) |
| Why things are as they are | `docs/decisions.md` (D-001 onwards), `docs/hardware/power-architecture.md`, `docs/datasheets/facts.md` |

## Status of each phase (re-run by `tools/phase7_gate.sh`)

| Phase | Gate | Evidence |
|---|---|---|
| 1 Logic | `tools/phase1_gate.sh` | `docs/evidence/phase1-gate.txt` |
| 2 Firmware on virtual hardware (Wokwi) | manual GitHub Actions workflow (D-036); last full run 8/8 | `docs/evidence/phase2-gate.md`; re-run pending (B-27) |
| 3 Whole-cycle replay | `tools/phase3_gate.sh` | `docs/evidence/phase3-gate.txt` |
| 4 Electronics | `tools/phase4_gate.sh` | `docs/evidence/phase4-gate.txt` |
| 5 BOM | `tools/phase5_gate.sh` | `docs/evidence/phase5-gate.txt` (NOT FINAL until two parts are in stock) |
| 6 Enclosure | `tools/phase6_gate.sh` | `docs/evidence/phase6-gate.txt` |
| 7 Hand-off | `tools/phase7_gate.sh` | `docs/evidence/phase7-gate.txt` |

## Open items (must be closed before or during the pilot)

1. **Two parts out of stock at research time:** a 2 A 5 V boost module and an A7670C breakout that takes VBAT directly (B-26).
2. **MQTT provider not chosen** (Q-5). The device side is done (D-060): per-device settings are loaded from
   `seri.cfg` + `ca.pem` on the microSD card (installer-guide.md). Recommendation: HiveMQ Cloud or EMQX Cloud for the pilot.
3. **Signed firmware updates deferred** beyond v1 (D-034).
4. **Native-speaker review** of all Hindi, Telugu and Kannada text, including the farmer card (B-24).
5. **Assumed values to measure:** modem idle current, printer peak current, holder resistance, module sizes (bench plan).
6. **Wokwi re-run** after the phase 4-7 firmware changes. Tried 2026-11-22: blocked by the Wokwi free plan's monthly CI quota; re-run when it resets or on a paid plan.
