# Bill of materials: Seri-tool v1 pilot

**Status: FINAL (every line priced and in stock at research time)**

Prices in INR from Indian suppliers where found (LCSC otherwise, at 84 INR/USD). Most come from a web-search index on 2026-11-22, not a live checkout: confirm before ordering. Shipping not included. Sources and links: `bom/sources.yaml`.

## Carrier board parts

| Item | Value | Qty | Refs | MPN | Supplier | Unit | Per unit | Batch of 10 |
|---|---|---|---|---|---|---|---|---|
| DC barrel jack 5.5x2.1 mm, PCB horizontal with switch pin | 12V 3A DC jack | 1 | J1 | DC-005 | [Robu.in](https://aws.robu.in/?p=121507) | ₹9.00 | ₹9.00 | ₹90.00 |
| Polyfuse 1.5 A hold [1812] | 1.5A hold | 1 | F1 | SMD1812B150TF/24 (Yageo) / SMD1812P150TF/24 (PTTC) | [Evelta](https://evelta.com/resettable-fuse-ptc-24v-1-5a-1812-smd1812b150tf-24/) | ₹13.00 | ₹13.00 | ₹130.00 |
| SS34 [SMA / DO-214AC] | SS34 | 2 | D1 D4 | SS34 | [Evelta](https://evelta.com/ss34-sma-40v-3a-schottky-rectifier-diode-2pin-do-214ac/) | ₹5.00 | ₹10.00 | ₹100.00 |
| SMAJ15A [SMA] | SMAJ15A | 1 | D2 | SMAJ15A | [Evelta](https://evelta.com/smaj15a-15v-400w-esd-suppressor-tvs-diodes-2pin-sma-do-214ac/) | ₹5.64 | ₹5.64 | ₹56.40 |
| Electrolytic 100uF 25V radial 8 mm, 3.5 mm pitch | 100uF 25V | 1 | C1 | generic; LCSC alt Rubycon 25YXJ100M5X11 (C108360, D5 mm) | [Quartz Components](https://quartzcomponents.com/products/100uf-25v-radial-electrolytic-capacitor) | ₹3.00 | ₹3.00 | ₹30.00 |
| Capacitor 0805 X7R 100nF 50V | 100nF | 9 | C2 C3 C7 C13 C18 C21 C22 C23 C25 | 0805B104K500CT (Walsin) / CC0805KRX7R9BB104 (Yageo, LCSC C49678) | [Hubtronics](https://hubtronics.in/walsin-0805b104k500ct) | ₹0.49 | ₹4.41 | ₹49.00 |
| Resistor 0805 1% (per value) | 100k | 8 | R1 R8 R9 R10 R11 R13 R21 R35 | 0805W8F1002T5E (10k; LCSC C17414) - series 0805W8F____T5E for the other values | [Quartz Components (assortment kit)](https://quartzcomponents.com/products/30-sets-resistor-kit-805-smd-with-1-tolerance-from-1-ohm-to-3-3m-ohm-600pcs-copy) | ₹0.37 | ₹2.93 | ₹366.00 |
| Resistor 0805 1% (per value) | 22k | 1 | R2 | 0805W8F1002T5E (10k; LCSC C17414) - series 0805W8F____T5E for the other values | [Quartz Components (assortment kit)](https://quartzcomponents.com/products/30-sets-resistor-kit-805-smd-with-1-tolerance-from-1-ohm-to-3-3m-ohm-600pcs-copy) | ₹0.37 | ₹0.37 | ₹366.00 |
| Male header strip 1x40 2.54 mm + jumper shunt | BUCK IN | 1 | J4 | generic | [Hubtronics](https://hubtronics.in/misc-connectors/1752) | ₹2.95 | ₹2.95 | ₹29.50 |
| Male header strip 1x40 2.54 mm + jumper shunt | BUCK OUT 5V | 1 | J5 | generic | [Hubtronics](https://hubtronics.in/misc-connectors/1752) | ₹2.95 | ₹2.95 | ₹29.50 |
| Capacitor 1206 X5R 22uF 10V | 22uF 10V | 1 | C4 | GRM31CR71A226KE15L (Murata 10V X7R) / CL31A226KAHNNNE (Samsung 25V X5R, LCSC C12891) | [Lion Circuits](https://lioncircuits.com/parts/GRM31CR71A226KE15L) | ₹35.57 | ₹35.57 | ₹426.80 |
| TP4056 [SOP-8 with exposed pad, ESOP-8] | TP4056 | 1 | U2 | TP4056 (UMW) / TP4056-42-ESOP8 (Top Power) | [Evelta](https://evelta.com/tp4056-1a-linear-lithium-ion-battery-charger-ic-esop-8) | ₹13.05 | ₹13.05 | ₹130.50 |
| Resistor 0805 1% (per value) | 2.2k | 3 | R3 R22 R23 | 0805W8F1002T5E (10k; LCSC C17414) - series 0805W8F____T5E for the other values | [Quartz Components (assortment kit)](https://quartzcomponents.com/products/30-sets-resistor-kit-805-smd-with-1-tolerance-from-1-ohm-to-3-3m-ohm-600pcs-copy) | ₹0.37 | ₹1.10 | ₹366.00 |
| Capacitor 1206 X5R 10uF 16V | 10uF | 4 | C5 C6 C17 C24 | CL31A106KBHNNNE (50V X5R, LCSC C13585) | [LCSC](https://www.lcsc.com/product-detail/C13585.html) | ₹10.15 | ₹40.62 | ₹406.20 |
| Resistor 0805 1% (per value) | 10k | 10 | R16 R18 R19 R27 R28 R29 R30 R31 R37 R38 (DNP: R4) | 0805W8F1002T5E (10k; LCSC C17414) - series 0805W8F____T5E for the other values | [Quartz Components (assortment kit)](https://quartzcomponents.com/products/30-sets-resistor-kit-805-smd-with-1-tolerance-from-1-ohm-to-3-3m-ohm-600pcs-copy) | ₹0.37 | ₹3.66 | ₹366.00 |
| JST XH 2.54/2.50 mm headers + housings + crimp (2, 3, 4 pin) | 10k B3950 | 0 |  (DNP: TH1) | B2B-XH-A(LF)(SN), B3B-XH-A(LF)(SN), B4B-XH-A(LF)(SN) | [Lion Circuits](https://lioncircuits.com/parts/B2B-XH-A) | — | — | — |
| LED 3 mm red / green, THT | red: charging | 1 | D3 | 3 mm LED red; 3 mm LED green | [Electroncomponents](https://www.electroncomponents.com/LED-3mm-Red-Diffused) | ₹0.80 | ₹0.80 | ₹8.00 |
| Resistor 0805 1% (per value) | 1k | 3 | R5 R7 R14 | 0805W8F1002T5E (10k; LCSC C17414) - series 0805W8F____T5E for the other values | [Quartz Components (assortment kit)](https://quartzcomponents.com/products/30-sets-resistor-kit-805-smd-with-1-tolerance-from-1-ohm-to-3-3m-ohm-600pcs-copy) | ₹0.37 | ₹1.10 | ₹366.00 |
| DW01A [SOT-23-6] | DW01A | 1 | U3 | DW01A | [LCSC](https://www.lcsc.com/product-detail/C351410.html) | ₹2.10 | ₹2.10 | ₹21.00 |
| Resistor 0805 1% (per value) | 100 | 3 | R6 R24 R25 | 0805W8F1002T5E (10k; LCSC C17414) - series 0805W8F____T5E for the other values | [Quartz Components (assortment kit)](https://quartzcomponents.com/products/30-sets-resistor-kit-805-smd-with-1-tolerance-from-1-ohm-to-3-3m-ohm-600pcs-copy) | ₹0.37 | ₹1.10 | ₹366.00 |
| FS8205A [TSSOP-8] | FS8205A | 3 | Q1 Q2 Q11 | FS8205A (TSSOP-8) | [Hubtronics](https://hubtronics.in/8205a-tssop8?tag=FS8205A) | ₹5.90 | ₹17.70 | ₹177.00 |
| 18650 PCB holder, single cell, THT [Keystone 1043] | 18650 | 2 | BT1 BT2 (DNP: BT3 BT4) | 1043 | [DigiKey India](https://www.digikey.in/en/products/detail/keystone-electronics/1043/2745669) | ₹278.00 | ₹556.00 | ₹5,560.00 |
| AO3401A [SOT-23] | AO3401A | 3 | Q3 Q12 Q4 | AO3401A | [LCSC](https://www.lcsc.com/product-detail/C15127.html) | ₹6.95 | ₹20.85 | ₹208.50 |
| Electrolytic 100uF 10V radial 6.3 mm, 2.5 mm pitch | 100uF 10V | 2 | C8 C19 | ECE-A1AKA101 (Panasonic KA, 6.3 x 7 mm, 2.5 mm) | [Robu.in](https://robu.in/product/100-uf-10v-through-hole-electrolytic-capacitor-pack-of-20/) | ₹1.95 | ₹3.90 | ₹39.00 |
| Electrolytic 1000uF 6.3V LOW-ESR radial 10 mm, 5 mm pitch | 1000uF 6.3V low-ESR | 2 | C9 C26 | 10ZLH1000MEFC10X12.5 (Rubycon ZLH, 10 V) or EEU-FM0J102 (Panasonic FM, 6.3 V) | [Tanotis](https://www.tanotis.com/products/rubycon-10zlh1000mefc10x12-5-electrolytic-capacitor-miniature-1000-micro-f-10-v-zlh-series-plusmn-20-radial-leaded-10-mm) | ₹71.31 | ₹142.62 | ₹1,426.20 |
| Capacitor 1206 X5R 22uF 10V | 22uF | 2 | C10 C11 | GRM31CR71A226KE15L (Murata 10V X7R) / CL31A226KAHNNNE (Samsung 25V X5R, LCSC C12891) | [Lion Circuits](https://lioncircuits.com/parts/GRM31CR71A226KE15L) | ₹35.57 | ₹71.13 | ₹746.90 |
| Capacitor 0805 X5R/X7R 1uF 25V | 1uF | 1 | C12 | CL21B105KAFNFNE (Samsung, LCSC C1712) / CC0805KKX7R8BB105 (Yageo) | [LCSC](https://www.lcsc.com/product-detail/C1712.html) | ₹2.64 | ₹2.64 | ₹52.80 |
| Capacitor 0805 C0G 33pF and 10pF | 33pF | 1 | C14 | 33pF: VJ0805A330JXACW1BC (Vishay) or CL21C330JBANNNC (Samsung, LCSC C1814); 10pF: CL21C100JBANNNC (Samsung, LCSC C1785) | [Lion Circuits](https://lioncircuits.com/parts/VJ0805A330JXACW1BC) | ₹11.56 | ₹11.56 | ₹115.60 |
| Capacitor 0805 C0G 33pF and 10pF | 10pF | 1 | C15 | 33pF: VJ0805A330JXACW1BC (Vishay) or CL21C330JBANNNC (Samsung, LCSC C1814); 10pF: CL21C100JBANNNC (Samsung, LCSC C1785) | [Lion Circuits](https://lioncircuits.com/parts/VJ0805A330JXACW1BC) | ₹11.56 | ₹11.56 | ₹115.60 |
| AP2112K-3.3 [SOT-23-5] | AP2112K-3.3 | 1 | U4 | AP2112K-3.3TRG1 | [LCSC](https://www.lcsc.com/product-detail/C51118.html) | ₹14.14 | ₹14.14 | ₹141.40 |
| Capacitor 0805 X5R 4.7uF 10V | 4.7uF | 1 | C16 | CL21A475KAQNNNE (25V X5R, LCSC C1779) | [LCSC](https://www.lcsc.com/product-detail/C1779.html) | ₹2.91 | ₹2.91 | ₹58.10 |
| Male header strip 1x40 2.54 mm + jumper shunt | 3V3 to DevKitC: REMOVE before USB | 1 | JP2 | generic | [Hubtronics](https://hubtronics.in/misc-connectors/1752) | ₹2.95 | ₹2.95 | ₹29.50 |
| MMBT3904 [SOT-23] | MMBT3904 | 6 | Q5 Q6 Q7 Q8 Q9 Q10 | MMBT3904 | [LCSC](https://www.lcsc.com/product-detail/C20526.html) | ₹0.87 | ₹5.24 | ₹87.36 |
| Resistor 0805 1% (per value) | 4.7k | 7 | R12 R15 R17 R20 R26 R34 R36 | 0805W8F1002T5E (10k; LCSC C17414) - series 0805W8F____T5E for the other values | [Quartz Components (assortment kit)](https://quartzcomponents.com/products/30-sets-resistor-kit-805-smd-with-1-tolerance-from-1-ohm-to-3-3m-ohm-600pcs-copy) | ₹0.37 | ₹2.56 | ₹366.00 |
| Male header strip 1x40 2.54 mm + jumper shunt | BOOST IN | 1 | J6 | generic | [Hubtronics](https://hubtronics.in/misc-connectors/1752) | ₹2.95 | ₹2.95 | ₹29.50 |
| Male header strip 1x40 2.54 mm + jumper shunt | BOOST OUT 5V | 1 | J7 | generic | [Hubtronics](https://hubtronics.in/misc-connectors/1752) | ₹2.95 | ₹2.95 | ₹29.50 |
| Electrolytic 470uF 10V radial 8 mm, 3.5 mm pitch | 470uF 10V | 1 | C20 | 10YXF470MFFC8X11.5 (Robu) / 10YXG470MEFCT78X11.5 (LCSC C2940709) | [LCSC](https://www.lcsc.com/product-detail/C2940709.html) | ₹7.63 | ₹7.63 | ₹76.30 |
| JST XH 2.54/2.50 mm headers + housings + crimp (2, 3, 4 pin) | PRINTER POWER | 1 | J8 | B2B-XH-A(LF)(SN), B3B-XH-A(LF)(SN), B4B-XH-A(LF)(SN) | [Lion Circuits](https://lioncircuits.com/parts/B2B-XH-A) | ₹10.67 | ₹10.67 | ₹106.70 |
| JST XH 2.54/2.50 mm headers + housings + crimp (2, 3, 4 pin) | PRINTER TTL | 1 | J9 | B2B-XH-A(LF)(SN), B3B-XH-A(LF)(SN), B4B-XH-A(LF)(SN) | [Lion Circuits](https://lioncircuits.com/parts/B2B-XH-A) | ₹10.67 | ₹10.67 | ₹106.70 |
| Male header strip 1x40 2.54 mm + jumper shunt | A7670C HARNESS | 1 | J10 | generic | [Hubtronics](https://hubtronics.in/misc-connectors/1752) | ₹2.95 | ₹2.95 | ₹29.50 |
| Female header 1x19 2.54 mm | DevKitC J2 (left) | 1 | J2 | generic 1x40 / 1x20 female berg strip | [Robocraze](https://robocraze.com/products/40x1-pin-2-54mm-straight-female-pin-berg-strip-pack-of-50) | ₹6.38 | ₹6.38 | ₹319.00 |
| Female header 1x19 2.54 mm | DevKitC J3 (right) | 1 | J3 | generic 1x40 / 1x20 female berg strip | [Robocraze](https://robocraze.com/products/40x1-pin-2-54mm-straight-female-pin-berg-strip-pack-of-50) | ₹6.38 | ₹6.38 | ₹319.00 |
| JST XH 2.54/2.50 mm headers + housings + crimp (2, 3, 4 pin) | SHT31 PROBE (2 m) | 1 | J11 | B2B-XH-A(LF)(SN), B3B-XH-A(LF)(SN), B4B-XH-A(LF)(SN) | [Lion Circuits](https://lioncircuits.com/parts/B2B-XH-A) | ₹10.67 | ₹10.67 | ₹106.70 |
| JST XH 2.54/2.50 mm headers + housings + crimp (2, 3, 4 pin) | DS18B20 (option) | 1 | J12 | B2B-XH-A(LF)(SN), B3B-XH-A(LF)(SN), B4B-XH-A(LF)(SN) | [Lion Circuits](https://lioncircuits.com/parts/B2B-XH-A) | ₹10.67 | ₹10.67 | ₹106.70 |
| DS3231SN [SOIC-16 wide] | DS3231SN | 1 | U5 | DS3231SN# / DS3231SN#T&R | [LCSC](https://www.lcsc.com/product-detail/C9866.html) | ₹614.86 | ₹614.86 | ₹6,148.60 |
| CR2032 PCB holder [Keystone 3002 or equivalent] | CR2032 | 1 | BT5 | Keystone 3002 (SMT) / generic 20 mm THT holder | [Hubtronics](https://hubtronics.in/1528?tag=pcb+mount) | ₹6.00 | ₹6.00 | ₹60.00 |
| microSD socket [Hirose DM3AT-SF-PEJM5 or equivalent push-push with card detect] | microSD | 1 | J13 | DM3AT-SF-PEJM5 / generic 9-pin push-push (TF-01A style) | [Hubtronics](https://hubtronics.in/connectors/micro-sd-card-connector-push-type) | ₹17.70 | ₹17.70 | ₹177.00 |
| Male header strip 1x40 2.54 mm + jumper shunt | OLED cable (GND VCC SCL SDA) | 1 | J14 | generic | [Hubtronics](https://hubtronics.in/misc-connectors/1752) | ₹2.95 | ₹2.95 | ₹29.50 |
| Tactile switch 6x6 mm, 13 mm height, THT | UP | 1 | SW1 | 6x6x13 tact switch (e.g. KFC-A06-13) | [Ezyspare](https://ezyspare.com/?p=2555) | ₹4.00 | ₹4.00 | ₹40.00 |
| Tactile switch 6x6 mm, 13 mm height, THT | OK | 1 | SW2 | 6x6x13 tact switch (e.g. KFC-A06-13) | [Ezyspare](https://ezyspare.com/?p=2555) | ₹4.00 | ₹4.00 | ₹40.00 |
| Tactile switch 6x6 mm, 13 mm height, THT | DOWN | 1 | SW3 | 6x6x13 tact switch (e.g. KFC-A06-13) | [Ezyspare](https://ezyspare.com/?p=2555) | ₹4.00 | ₹4.00 | ₹40.00 |
| LED 3 mm red / green, THT | green | 1 | D5 | 3 mm LED red; 3 mm LED green | [Electroncomponents](https://www.electroncomponents.com/LED-3mm-Red-Diffused) | ₹0.80 | ₹0.80 | ₹8.00 |
| Resistor 0805 1% (per value) | 680 | 2 | R32 R33 | 0805W8F1002T5E (10k; LCSC C17414) - series 0805W8F____T5E for the other values | [Quartz Components (assortment kit)](https://quartzcomponents.com/products/30-sets-resistor-kit-805-smd-with-1-tolerance-from-1-ohm-to-3-3m-ohm-600pcs-copy) | ₹0.37 | ₹0.73 | ₹366.00 |
| LED 3 mm red / green, THT | red | 1 | D6 | 3 mm LED red; 3 mm LED green | [Electroncomponents](https://www.electroncomponents.com/LED-3mm-Red-Diffused) | ₹0.80 | ₹0.80 | ₹8.00 |
| Active buzzer 5 V, 12 mm, 7.6 mm pin pitch | active 5V | 1 | BZ1 | TMB12A05 | [Thingbits](https://www.thingbits.in/products/tmb12a05-5v-dc-active-buzzer-12-x-9-5mm) | ₹22.42 | ₹22.42 | ₹224.20 |
| 1N4148W [SOD-123] | 1N4148W | 1 | D7 | 1N4148W | [Evelta](https://evelta.com/switching-diode-100v-150ma-sod-123/) | ₹1.84 | ₹1.84 | ₹18.40 |
| JST XH 2.54/2.50 mm headers + housings + crimp (2, 3, 4 pin) | RELAY (VSYS, IN, GND) | 1 | J15 | B2B-XH-A(LF)(SN), B3B-XH-A(LF)(SN), B4B-XH-A(LF)(SN) | [Lion Circuits](https://lioncircuits.com/parts/B2B-XH-A) | ₹10.67 | ₹10.67 | ₹106.70 |

Subtotal: **₹1,781.79 per unit**, ₹21,322.86 for 10 units (whole packs)

## Off-board parts

| Item | Value | Qty | Refs | MPN | Supplier | Unit | Per unit | Batch of 10 |
|---|---|---|---|---|---|---|---|---|
| ESP32-DevKitC V4 (WROOM-32E) |  | 1 | plugs into J2/J3 | ESP32-DevKitC-32E | [Robu.in](https://st.robu.in/product/espressif-esp32-devkitc-esp32-wroom-3-2e-development-board) | ₹799.00 | ₹799.00 | ₹7,990.00 |
| A7670C 4G LTE Cat-1 breakout |  | 1 | plugs into J10 (match pin order before ordering PCBs) | SIMA7670C V1.1 (LM39302) | [Sharvi Technologies (IndiaMART, Bengaluru)](https://www.indiamart.com/proddetail/a7670c-lans-simcom-4g-lte-cat-1-module-breakout-board-2856405650597.html) | ₹975.00 | ₹975.00 | ₹9,750.00 |
| SHT31 probe, 2 m cable |  | 1 | to J11; ~1 m probe extended to 2 m (D-052) | SHT31 cable probe (seller model) | [DigiKey (US)](https://www.digikey.com/en/products/detail/dfrobot/SEN0385/13590873) | ₹1,671.60 | ₹1,671.60 | ₹16,716.00 |
| Shielded twisted-pair cable, 2-pair 24 AWG (per metre) |  | 2 | probe extension, metres | 2 pair 24 AWG STP (RS-485 type) | [IndiaMART (seller listing)](https://m.indiamart.com/impcat/twisted-pair-cables.html) | ₹110.00 | ₹220.00 | ₹2,200.00 |
| 58 mm TTL thermal printer with status TX |  | 1 | to J8/J9 | CSN-A2L TTL | [DNA Technology (older page)](https://www.dnatechindia.com/industrial-electronics-products/thermal-printer/CSN-A2-2-inch-micro-panel-thermal-printer-ttl-interface.html) | ₹2,076.80 | ₹2,076.80 | ₹20,768.00 |
| 1.3 inch OLED SH1106 I2C 128x64 |  | 1 | plugs into J14 | 1.3in OLED SH1106 I2C 4-pin | [Probots](https://probots.co.in/1-3-inch-128x64-oled-blue-display-sh1106-i2c.html) | ₹329.00 | ₹329.00 | ₹3,290.00 |
| 18650 Li-ion cell 2500-3000 mAh (genuine brand) |  | 2 | BT1, BT2 (footprints for 4) | INR18650-25R | [Robu.in](https://robu.in/?p=63398) | ₹418.00 | ₹836.00 | ₹8,360.00 |
| 12 V 3 A DC adapter, BIS registered |  | 1 | to J1 | FEDUS 12V 3A adapter | [Aajjo (FEDUS, New Delhi)](https://www.aajjo.com/product/fedus-12v-3a-dc-power-supply-adapter-for-lcd-tv-cctv-dvr-led-light-router-60-hz-in-new-delhi-fedus-india) | ₹299.00 | ₹299.00 | ₹2,990.00 |
| XL4015 buck module 5 A |  | 1 | wired to J4/J5 | XL4015 5A buck module with heatsink | [Hubtronics](https://hubtronics.in/power-converter-modules/dc-dc-power-converter-modules/xl4015-module) | ₹87.00 | ₹87.00 | ₹870.00 |
| 5 V boost module >= 2 A from 3.0-4.2 V |  | 1 | wired to J6/J7 | TPS61088A single-cell boost module (Probots) / TPS61023 breakout (7Semi) | [Robocraze](https://robocraze.com/products/tps61023-3-7a-5v-output-mini-boost-converter-breakout-board-7semi) | ₹79.00 | ₹79.00 | ₹790.00 |
| microSD card 8-16 GB (industrial or reputable) |  | 1 | J13 | SDSDQAF3-016G-I | [Tanotis](https://www.tanotis.com/products/samsung-32gb-pro-endurance-uhs-i-microsdhc-memory-card-with-sd-adapter) | ₹2,205.00 | ₹2,205.00 | ₹22,050.00 |
| CR2032 coin cell |  | 1 | BT5 | CR2032 | [Flipkart](https://www.flipkart.com/panasonic-cr2032-pack-10-battery/p/itm82db323a41035) | ₹42.30 | ₹42.30 | ₹423.00 |
| 57 mm thermal paper roll |  | 2 | consumable: first two rolls | 57mm x 7m thermal roll | [IndiaMART (various)](https://m.indiamart.com/impcat/thermal-paper-rolls.html) | ₹12.00 | ₹24.00 | ₹240.00 |
| PCB fabrication: 2-layer, 190 x 152 mm, 1.6 mm FR4, 1 oz, HASL, qty 5 and qty 10 |  | 1 | one board | n/a | [Secure Circuits (IndiaMART)](https://m.indiamart.com/secure-circuits/double-layer-pcb.html) | ₹187.70 | ₹187.70 | ₹1,877.00 |
| PETG or ASA 3D printer filament 1 kg (1.75 mm) |  | 0.6 | kg: base 304 + lid 139 cm3 = 565 g solid PETG (own printer) | Anycubic PETG Black 1kg 1.75mm | [Robocraze](https://robocraze.com/products/anycubic-petg-black-1kg-1-75mm-petg-3d-printer-filament) | ₹949.00 | ₹569.40 | ₹5,694.00 |
| M3 brass heat-set inserts (M3 x 5-6 mm), pack |  | 8 | 4 PCB posts + 4 lid bosses | M3 x 6 mm brass heat set threaded round insert nut | [iBots](https://ibots.in/?p=34353) | ₹5.12 | ₹40.96 | ₹512.00 |
| M3 x 8 mm screws (pan/countersunk), pack |  | 8 | lid x 4 (M3 x 8), PCB x 4 (M3 x 6 from the same kit) | Rpi Shop M3x8mm Full Thread Phillips Drive CSK Head Screw (Pack of 100) | [Zbotic](https://zbotic.in/product/100pcs-m3-129-grade-alloy-steel-hex-bolt-socket-head-cap-screws-kits/) | ₹3.44 | ₹27.55 | ₹344.40 |
| M2 x 5 mm self-tapping screws, pack |  | 4 | OLED on the lid posts | Pan Head Phillips Self-tapping Screws 2x5mm (LCSC C2911562) | [Probots](https://probots.co.in/m2-x-5mm-black-nylon-pan-head-phillips-screw.html) | ₹15.00 | ₹60.00 | ₹600.00 |
| Acrylic light pipe rod 3 mm diameter |  | 2 | LEDs D5/D6 to the lid | SLP3-200-250-F | [DigiKey India](https://www.digikey.in/en/products/detail/bivar-inc/SLP3-200-250-F/7393552) | ₹75.43 | ₹150.86 | ₹1,508.60 |
| PG7 nylon cable gland (IP68, 3-6.5 mm cable) |  | 1 | probe cable | 50.007 PA BS SW | [DigiKey India](https://www.digikey.in/en/products/detail/jacob-gmbh/50-007-PABSSW/11674493) | ₹130.46 | ₹130.46 | ₹1,304.60 |
| M12 breathable membrane vent (IP67/IP68, protective vent) |  | 1 | bottom wall | M12 breather vent plug / JDAE12PA-SW | [Eleczar Pvt Ltd (aajjo.com)](https://www.aajjo.com/product/ip68-breather-vent-plugs-waterproof-m12-for-automotive-in-faridabad-eleczar-private-limited) | ₹39.00 | ₹39.00 | ₹390.00 |
| Dupont jumper cable female-female, 4-wire ribbon, 15-20 cm |  | 4 | wires: OLED to J14 | 20CM DuPont Wire Color Jumper Cable 2.54mm 1P-1P Female to Female 40PCS | [Robu.in](https://aws.robu.in/?p=7449) | ₹0.85 | ₹3.40 | ₹34.00 |
| Dupont jumper cable female-female, 8-wire, 10 cm |  | 8 | wires: modem breakout to J10 (D-057) | 10CM Female to Female Breadboard Jumper DuPont 2.54MM 1P-1P Cable 40 Pcs | [Zbotic](https://zbotic.in/product/10cm-female-to-female-breadboard-jumper-wires-2-54mm/) | ₹0.95 | ₹7.60 | ₹76.00 |
| 3M VHB double-sided tape 12-25 mm |  | 0.012 | rolls (0.3 m of a 25 m roll): modem, buck, boost on the floor | VHB 5915 (12 mm x 25 m x 0.4 mm) | [Moglix](https://www.moglix.com/3m-vhb-5915-double-sided-tape-12mmx25mx04mm/mp/msn2r27xtmxyll) | ₹1,061.00 | ₹12.73 | ₹1,061.00 |

Subtotal: **₹10,873.36 per unit**, ₹109,838.60 for 10 units (whole packs)

## Total

* Per unit (parts only): **₹12,655.15**
* Pilot batch of 10 (whole packs, so leftover stock): **₹131,161.46** (₹13,116.15 per unit)

Not costed here: IoT/M2M SIM (Airtel or Vi) (recurring operator charge, not hardware); 3D printing labour / service (own printer assumed; a print service costs about Rs 3-10 per gram, i.e. Rs 1,700-5,700 per enclosure)

