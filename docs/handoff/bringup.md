# First power-on (bring-up) procedure

Do these steps in order on every pilot board. Stop at the first step that fails and record the reading.
Expected values come from the design (docs/hardware/power-architecture.md) and the datasheets cited there.
Use a 12 V bench supply with its current limit set as given.

| # | Set-up | Action / measurement | Pass |
|---|---|---|---|
| 1 | Bare board, nothing plugged in, no cells, JP2 removed | Resistance to GND from BAT_P, +5V_BUS, VSYS, +3V3, VIN_12V (use C9, C4, C8, C17, C1 pads) | All > 1 kΩ (no shorts) |
| 2 | XL4015 alone on the bench, 12 V in | Adjust output | 5.10 ± 0.05 V |
| 3 | Buck wired to J4/J5; 12 V at J1, limit 0.3 A; no cells, no DevKitC | +5V_BUS (J5-1) / VSYS (C8+) / +3V3 (C17+) / supply current | 5.1 V / 4.6-4.8 V / 3.30 ± 0.05 V / < 40 mA |
| 4 | As 3, reversed 12 V polarity for 2 s | Nothing warms; current ≈ 0 | D1 blocks (no damage) |
| 5 | Two 18650 cells, **each within 0.05 V of the other before inserting**, limit raised to 1 A | BAT_P; 12 V supply current; D3 | Cell voltage; ≈ 0.25-0.3 A at 12 V while charging (500 mA into the cells); D3 lit |
| 6 | As 5, disconnect 12 V | VSYS (follows battery − ~0.05 V); +3V3 | VSYS ≥ 3.5 V with charged cells; +3V3 stays 3.30 V |
| 7 | 12 V back; boost wired to J6/J7; GPIO26 not driven (no DevKitC) | +5V_PRN (J7-1) | ~0 V (boost off by default: R11/R13 keep Q4 off) |
| 8 | Flash firmware with JP2 **removed** and USB in (`firmware/release/v0.3.1/flash.sh`) | Serial monitor 115200 baud | `seri-tool 0.3.1 booting`, then `READY` |
| 9 | USB out, JP2 fitted, 12 V on, OLED/probe/SD/printer connected | OLED home screen; serial log via a USB-UART on the DevKitC is not available in this mode, so use JP2-removed + USB for logs | Home screen shows temperature, RH, stage, "Mains" |
| 10 | Breathe on the probe | OLED RH rises within ~10 s | Responds |
| 11 | Menu → print test | Printer prints a test line in the chosen language | Legible line; with paper removed the OLED/red LED show paper-out |
| 12 | SIM inserted, antenna on, modem harness to J10 | Wait ≤ 60 s; OLED link icon; send `20` from the owner phone | 4G registered; status SMS received |
| 13 | Unplug 12 V for > 2 min | OLED shows battery %; owner gets the mains-fail SMS; device keeps running (no reboot in the log) | As described |
| 14 | Plug 12 V back for > 2 min | Mains-return SMS; charging LED on | As described |

Record the readings of steps 3, 5, 6 and 12 for each board in the bench test record (bench-test-plan.md).
