# Assembly guide: Seri-tool v1 pilot unit

For a technician with a temperature-controlled soldering iron (fine conical and 2 mm chisel tips), flux,
0.5 mm solder, a multimeter and a 12 V bench supply with current limit. Parts are listed in `bom/bom.md`.
Board files are in `hardware/fab/`: the assembly drawing is `carrier-v1-assembly-top.pdf`, the schematic is
`carrier-v1-schematic.pdf`, and Gerbers are in `carrier-v1-gerbers.zip`.

## 1. Solder the carrier board (lowest parts first)

| Step | Parts | Notes |
|---|---|---|
| 1 | U2 TP4056 (SOIC-8 with exposed pad) | Tin the pad, solder the pins, then flow solder into the exposed pad from the pin side with a wide tip. The pad is the charger's heat path. |
| 2 | Q1, Q2, Q11 FS8205A (TSSOP-8, 0.65 mm) | Drag-solder with plenty of flux; check for bridges with a loupe. |
| 3 | U3 DW01A, U4 AP2112K, all SOT-23 (Q3-Q10, Q12) | **Q3, Q12, Q4 are AO3401A (P-FET); Q5-Q10 are MMBT3904 (NPN).** Same package: keep the bags apart. |
| 4 | U5 DS3231SN (SOIC-16W), J13 microSD socket | |
| 5 | All 0805 resistors and capacitors, 1206 capacitors | Values per reference in `bom/bom.csv`. **R22/R23 are 2.2k** (I2C pull-ups for the 2 m probe cable, D-052). |
| 6 | D1, D2, D4 (SMA), D7 (SOD-123), F1 polyfuse | D1/D4 SS34 and D2 SMAJ15A: the stripe is the cathode, matching the silkscreen bar. |
| 7 | BT5 CR2032 holder, solder jumpers | **JP1** leave bridged 1-2 (charger temperature check off, D-048). **JP3** leave bridged 1-2 (modem logic reference = module's 1.8 V) unless the breakout already shifts to 3.3 V: then cut 1-2 and bridge 2-3. |
| 8 | Through-hole: LEDs D3 (red), D5 (green), D6 (red); SW1-3; BZ1 (+ to the + mark); J1 DC jack | Solder LEDs flush to the board (the light pipes do the rest). |
| 9 | Headers: J2/J3 (1x19 female, from 1x40 strip), J4-J7 and JP2 (1x2 male), J10 (1x8 male), J14 (1x4 male), JST-XH J8, J9, J11, J12, J15 | Fit the DevKitC into J2/J3 *before* soldering them, so the sockets line up. |
| 10 | Electrolytics C1, C8, C19, C20, C9, C26 | Watch polarity (stripe = minus). C9/C26 are the low-ESR 1000 uF parts next to the modem header. |
| 11 | BT1, BT2 Keystone 1043 holders | Check the pin fit of the first holder against the footprint before soldering (hole size is from a conflicting drawing; see the bench plan). BT3/BT4 are not fitted. |

**Not fitted (DNP):** R4, TH1 (NTC option), BT3, BT4 (4-cell option).

## 2. Modules on the enclosure floor

Stick each module to its marked floor area with VHB tape (`mechanical/cadquery`, modules on the base):

| Module | Set before wiring | Wire to |
|---|---|---|
| XL4015 buck | **Adjust the output to 5.10 V on the bench first** (12 V in, no load) | IN+ → J4-1, IN- → J4-2, OUT+ → J5-1, OUT- → J5-2; 20-22 AWG |
| 5 V boost module | Adjust to 5.0 V if it is adjustable | IN+ → J6-1, IN- → J6-2, OUT+ → J7-1, OUT- → J7-2; 20-22 AWG |
| A7670C breakout | Must take VBAT 3.4-4.2 V directly (no regulator, D-054); SIM in; antenna on | 8-wire harness to J10, table below |

## 3. Harness tables

**J10 → A7670C breakout** (≤ 6 cm, 24 AWG or thicker; two wires each for VBAT and GND, D-057)

| J10 pin | Net | Breakout pin |
|---|---|---|
| 1, 2 | BAT_P (battery, 3.4-4.2 V) | VBAT (both wires) |
| 3, 4 | GND | GND (both wires) |
| 5 | MDM_TXD | module **TXD** (output) |
| 6 | MDM_RXD | module **RXD** (input) |
| 7 | MDM_PWRKEY | PWRKEY (the carrier pulls it low through Q8) |
| 8 | MDM_VDD_EXT | VDD_EXT / 1.8 V output (leave unconnected if JP3 is set to 3V3) |

**J14 → OLED on the lid** (4-wire Dupont, 15-20 cm): 1 GND → GND, 2 +3V3 → VCC, 3 SCL → SCL, 4 SDA → SDA.
Check the module's own pin labels: some modules swap GND and VCC.

**J8 / J9 → printer** (CSN-A2, TTL version). J8: 1 +5V_PRN → printer VH, 2 GND → printer GND (power cable, ≥ 2 A).
J9: 1 PRN_RXD → printer **RXD** (data into the printer), 2 PRN_TXD_EXT → printer **TXD** (status out of the printer), 3 GND → GND.

**J11 → SHT31 probe, extended to 2 m** (D-052). Shielded 2-pair 24 AWG: pair A = SDA + GND, pair B = SCL + 3V3.
Shield connected to GND **at the board end only**. Solder and heat-shrink the joint inside the box, behind the PG7 gland.
J11: 1 +3V3, 2 SDA, 3 SCL, 4 GND.

**J12** (optional DS18B20): 1 +3V3, 2 DATA, 3 GND. **J15** (relay, future): 1 VSYS, 2 RELAY_DRV (open collector, sinks ≤ 100 mA), 3 GND.

## 4. Jumper JP2: one 3.3 V source only (D-043)

* **JP2 fitted:** normal running. The DevKitC is powered from the carrier.
* **JP2 removed:** required whenever USB is plugged into the DevKitC (flashing, debugging). In this mode the
  DevKitC runs from USB and the carrier's peripherals from the carrier; both are 3.3 V, so signals still work.

## 5. Firmware

`firmware/release/v0.3.0/flash.sh /dev/ttyUSB0` (checks SHA256SUMS, then writes bootloader 0x1000,
partitions 0x8000, boot_app0 0xe000, application 0x10000). Build from source: `cd firmware && pio run -e esp32`.

## 6. Enclosure

Print `mechanical/cadquery/out/base.stl`, `lid.stl` and three `plunger_SW*.stl` in PETG (0.2 mm layers,
4 perimeters, 30 % infill; base open side up, lid outside face down). Press M3 heat-set inserts into the 4 PCB
posts and the 4 corner bosses. Fit the PG7 gland (probe) and M12 vent in the bottom wall, the printer in the lid
cut-out, the OLED on its 4 posts (M2 x 5), the plungers from the inside, and 3 mm light pipes into the LED holes.
Mount on the wall with the two keyholes and the bottom screw, sensor probe downwards.
