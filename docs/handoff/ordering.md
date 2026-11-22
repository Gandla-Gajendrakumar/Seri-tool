# Ordering guide (pilot batch)

The full priced list is `bom/bom.md` (machine-readable: `bom/bom.csv`, sources and reasons: `bom/sources.yaml`).
Prices are from 2026-11-22, mostly from search-engine snapshots of the stores' pages: **confirm price and stock at order time.**

## Before ordering (blocking items)

1. **5 V boost module, >= 2 A from one cell** (TPS61088 class). The chosen source was out of stock (B-26).
2. **A7670C breakout that takes VBAT 3.4-4.2 V directly** (no on-board regulator, D-054). Ask the seller for
   a photo of the pin labels: you need VBAT, GND, TXD, RXD, PWRKEY and ideally VDD_EXT (1.8 V).
3. Fit-check one **Keystone 1043** holder against the footprint drawing (B-20) before ordering boards.
4. Run `tools/phase5_gate.sh` after updating `bom/sources.yaml`: it must report FINAL.

## Order groups

| Group | What | Where | Note |
|---|---|---|---|
| A. Chips and passives | ICs (AO3401A, AP2112K, DS3231SN, DW01A, MMBT3904), MLCCs with stated dielectric, 470 uF | LCSC (one order, ships to India) | Genuine parts (D-055); allow 1-2 weeks and import duty |
| B. Indian stores | DevKitC (Robu), modules, connectors, headers, LEDs, switches, buzzer, cells, printer, OLED, adapter, SD card, enclosure hardware | Robu, Robocraze, Probots, Hubtronics, Evelta, ElectronicsComp, Quartz, Zbotic, DNA Tech, DigiKey India | See the supplier column in bom.md |
| C. PCB | `hardware/fab/carrier-v1-gerbers.zip`: 2 layers, 190 x 152 mm, 1.6 mm FR4, 1 oz, HASL, green mask, white silk | Indian fab (e.g. Lion Circuits, PCB Power) or JLCPCB | Upload the zip to get a real quote; the BOM uses a published per-area rate |
| D. Enclosure | `mechanical/cadquery/out/*.stl` in PETG | Own printer (0.6 kg filament) or a print service (Rs 3-10/g) | |
| E. SIM | Airtel or Vi IoT/M2M SIM (SMS + small data) | Operator / M2M reseller | Needs KYC; Jio only if B-13 passes |

## Cost summary

See the Total section of `bom/bom.md`: parts per unit and the pilot batch of 10 (whole packs).
Shipping, import duty and the SIM plan are not included.
