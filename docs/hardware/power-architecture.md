# Power architecture (carrier board v1)

Plain-language summary first; the engineering detail and datasheet pages follow.

## In one paragraph

The 12 V adapter feeds a small ready-made **buck module** that makes 5 V. That 5 V charges the two
18650 cells (through a **TP4056** charger) and runs the electronics. When the mains fails, a
transistor switch (**P-FET**) connects the cells to the electronics with no break, so nothing
restarts. The **4G modem sits directly on the cells**, because it needs short 2 A bursts that only a
battery can deliver cleanly. The **ESP32** gets a steady 3.3 V from a small regulator. The **printer**
gets 5 V from a ready-made **boost module** that the ESP32 can switch off to save battery. A
**protection chip** cuts the cells off on a short circuit or if they are run completely flat.

```
 12 V jack ─ fuse ─ diode ─┬─ buck module ── +5V_BUS ─┬─ TP4056 charger ──────────┐
 (mains sense → GPIO35) ───┘                          │                          │
                                                      └─ diode ─┐                │
                                                                ├── VSYS ──┬── AP2112K ── +3V3 ── ESP32, OLED, RTC, SD, sensor
                        cells (BAT_P) ── 2 x P-FET ─────────────┘          ├── switch ── boost module ── +5V_PRN ── printer
                           │   (on only when +5V_BUS is gone)              └── buzzer, relay header
                           ├── 4G modem (2 x 1000 µF next to it)
                           └── battery sense → GPIO36
   cells (CELL_N) ── DW01A + 3 x FS8205A protection ── board GND
```

## Why each part

| Part | Why | Datasheet fact it rests on |
|---|---|---|
| Buck module, >= 3 A (XL4015 class) | Mains-side 12 V → 5 V; covers printer + ESP32 + charger at once | XL4015 Rev 2.0 p5: 5 A, 87 % at 12 V → 5 V |
| TP4056 at 500 mA (RPROG 2.2 kΩ) | Simple, cheap charger. 500 mA keeps the buck within rating and the enclosure cool | TP4056 NJ REV_2.4 p5: 425–575 mA at 2.2 kΩ |
| 2 x AO3401A + SS34 "power path" | The TP4056 has none of its own; this switches to battery with no reset | TP4056 p9: "all loads on the BAT pin must be supplied by the battery"; AO3401A Rev 3.1 p2 |
| Modem on the cells, 2 x 1000 µF | Modem needs 3.4–4.2 V and 2 A peaks with ≥ 300 µF | A7670 HD V1.03 p26–27 |
| AP2112K-3.3 | ESP32 needs ≥ 0.5 A supply; AP2112K gives 600 mA | WROOM-32E v2.1 p28; AP2112 DS39724 p8 |
| Boost module + P-FET switch | Printer needs 5 V ≥ 2 A; switched off between prints on battery | CSN-A2 manual; D-046 |
| DW01A + 3 x FS8205A | Short-circuit and over-discharge protection for bare 18650s | DW01x-DS-17 p9; FS8205A-DS-12 p3 |
| Level shifter (MMBT3904, common base) | Modem UART is 1.8 V, 2.0 V absolute max | A7670 HD p19, p33–34, p56 |
| 3V3 jumper JP2 | DevKitC must have only one power source | DevKitC V4 guide, "Power Supply Options" |

## Rules the firmware enforces (tested in phases 1–3)

* Low-battery alert at **3.70 V**, clears at 3.85 V or mains (D-038).
* Modem silent below **3.60 V** on battery; held alerts go out when mains returns (D-038).
* On battery: **no modem activity within 2 s of printing**, printer in low-power mode, boost off
  between prints, screen sleeps after 60 s (D-039, D-046).

## Evidence

* SPICE sweep: `hardware/spice/sweep.py` → `docs/evidence/phase4-spice-sweep.md` (envelope 12/12 pass;
  worst case the modem dips to 3.403 V at a 3.60 V battery, against a 3.40 V minimum).
* Battery budget: `hardware/power_budget/budget.py` → `docs/evidence/phase4-power-budget.md`
  (36 h on 2 cells, 73 h on 4 cells, against a 24 h target).
* Schematic ERC and PCB DRC: `tools/phase4_gate.sh`.

## Bench items (cannot be settled in software)

| Item | Why it matters | Pass level |
|---|---|---|
| Holder + wiring resistance per cell | The modem margin is only 3 mV in the worst case | ≤ 20 mΩ (D-050) |
| Printer peak current in low-power mode | Decides battery printing and the protection trip margin | ≤ 1.0 A at 5 V |
| Printer start-up time after the boost turns on | Firmware waits 500 ms (assumed) | prints the first line correctly |
| A7670C idle current, registered | Datasheet says "TBD"; the budget assumes 25 mA | runtime stays ≥ 24 h |
| DevKitC extras (CP2102, LED) on 3V3 | Budget assumes 6 mA | as above |
| TP4056 temperature in the closed box at 500 mA | No thermal vias under the exposed pad (hand solder) | < 85 °C case |
| Modem SMS on the chosen SIM (Airtel/Vi, and Jio over LTE) | Bands/IMS not provable in software | SMS received |
