# Datasheet facts – addendum 2

All PDFs are in `/tmp/claude-0/-home-user-Seri-tool/a0e3ea76-446d-5600-8913-2d583f46d7e6/scratchpad/datasheets/`. "Page" means the **PDF page** (1-based). Quotes come from `pdftotext -layout` output. Spacing is collapsed and table rows are shortened to Parameter … Symbol Min Typ Max Unit. Where a value depended on column position or a symbol that the text extraction lost, the page was rendered to an image and checked.

---

## 1. DW01A one-cell Li-ion protection IC

### 1a. Fortune Semiconductor (genuine): DW01A-G
File `dw01a_fortune.pdf`, LCSC C61503 (manufacturer listed as Fortune Semicon).
Document: **"Datasheet DW01x – One Cell Lithium-ion/Polymer Battery Protection IC", DW01x-DS-17_EN, REV. 1.7, May 2014**, Fortune Semiconductor Corporation. It covers the whole DW01x family. The DW01A-G table is on PDF p.9.

| Fact | Value | Document & rev | Page | Quote |
|---|---|---|---|---|
| Overcharge detect (VOCP) | 4.30 V typ (4.25–4.35) | DW01x-DS-17 Rev 1.7 | 9 | "Overcharge Protection Voltage DW01A VOCP 4.25 4.30 4.35 V" |
| Overcharge release (VOCR) | 4.10 V typ (4.05–4.15) | same | 9 | "Overcharge Release Voltage VOCR 4.05 4.10 4.15 V" |
| Overdischarge detect (VODP) | 2.40 V typ (2.30–2.50) | same | 9 | "Overdischarge Protection Voltage VODP 2.30 2.40 2.50 V" |
| Overdischarge release (VODR) | 3.00 V typ (2.90–3.10) | same | 9 | "Overdischarge Release Voltage VODR 2.90 3.00 3.10 V" |
| Overcurrent detect (VOI1) | 150 mV typ (120–180) | same | 9 | "Overcurrent Protection Voltage VOIP (VOI1) 120 150 180 mV" |
| (Product list value for VOI1) | 150 ± 30 mV, "AUTO recovery" | same | 4 | "DW01A-G SOT-23-6 4.300±0.050 4.100±0.050 2.40±0.100 3.0±0.100 150±30 NO AUTO recovery" |
| Short-circuit detect (VOI2) | 1.35 V typ (1.00–1.70) at VCC = 3.6 V | same | 9 | "Short Current Protection Voltage VCC=3.6V VSIP (VOI2) 1.00 1.35 1.70 V" |
| Overcharge delay | 80 ms typ, 200 ms max | same | 9 | "Overcharge Delay Time TOC 80 200 ms" |
| Overdischarge delay | 40 ms typ, 100 ms max | same | 9 | "Overdischarge Delay Time VCC=3.6V to 2.0V TOD 40 100 ms" |
| Overcurrent delay (1) | 10 ms typ, 20 ms max | same | 9 | "Overcurrent Delay Time (1) VCC=3.6V TOI1 10 20 ms" |
| Short-circuit delay (2) | 5 µs typ, 50 µs max | same | 9 | "Overcurrent Delay Time (2) VCC=3.6V TOI2 5 50 μs" |
| Supply current (operating) | 3.0 µA typ, 6.0 µA max at VCC = 3.9 V | same | 9 | "Supply Current VCC=3.9V ICC 3.0 6.0 μA" |
| Power-down current | **The DW01A-G table has no IPD row.** It gives an overdischarge current instead: 1.5 µA typ, 3.5 µA max at VCC = 2.0 V. (The DW01+/DW01- tables on p.7–8 give IPD 0.1 µA max.) | same | 9 | "Overdischarge Current VCC=2.0V IOD 1.5 3.5 μA" |
| Package | SOT-23-6 | same | 3 / 4 | "PACKAGE TYPE SOT-23-6 (Green-Package)"; "DW01A-G SOT-23-6 …" |

Note: every page carries a diagonal watermark (garbled in the text output, apparently "For Reference Only / Properties of FORTUNE"). It does not hide any of the values above.

### 1b. Clone: PUOLOP (迪浦) DW01A
File `dw01a_puolop.pdf`, LCSC C351410. The C351410 PDF link on LCSC first appeared to be a Fortune file, but the product page and the PDF itself show the maker as PUOLOP.
Document: **"DW01A", Rev B, 2016-4-12**, PUOLOP logo in the page header.

| Fact | Value | Document & rev | Page | Quote |
|---|---|---|---|---|
| Overcharge detect | 4.30 V typ (4.25–4.35) | PUOLOP DW01A Rev B 2016-4-12 | 4 | "Overcharge Protection Voltage VOC 4.25 4.30 4.35 V" |
| Overcharge release | 4.10 V typ (4.05–4.15) | same | 4 | "Overcharge Release Voltage VOCR 4.05 4.10 4.15 V" |
| Overdischarge detect | 2.50 V typ (2.40–2.60) | same | 4 | "Overdischarge Protection Voltage VOD 2.40 2.50 2.60 V" |
| Overdischarge release | 2.90 V typ (2.80–3.00) | same | 4 | "Overdischarge Release Voltage VODR 2.80 2.90 3.00 V" |
| Overcurrent detect | 150 mV typ (130–170) | same | 4 | "Overcurrent Protection Voltage VEDI 130 150 170 mV" |
| Short-circuit detect | 1.36 V typ (0.82–1.75) | same | 4 | "Short Current Protection Voltage Vshort 0.82 1.36 1.75 V" |
| Overcharge delay | 110 ms typ, 200 ms max | same | 4 | "Overcharge Delay Time TOC 110 200 ms" |
| Overdischarge delay | 55 ms typ, 200 ms max | same | 4 | "Overdischarge Delay Time VCC=3.6V to 2.4V TOD 55 200 ms" |
| Overcurrent delay (1) | 7 ms typ, 20 ms max (column position checked on the rendered page) | same | 4 | "Overcurrent Delay Time (1) T EDI 7 20 ms" |
| Short-circuit delay (2) | 400 µs typ, 600 µs max | same | 4 | "Overcurrent Delay Time (2) T short 400 600 μs" |
| Supply current | 3.0 µA typ, 6.0 µA max at VCC = 3.6 V | same | 4 | "Supply Current VCC=3.6V ICC 3.0 6.0 μA" |
| Power-down current | 4 µA max at VCC = 1.8 V | same | 4 | "Power-Down Current VCC=1.8V IPD 4 μA" |
| Package | SOT-23-6 | same | 1 | "PACKAGE TYPE SOT-23-6" |

**Design note:** Fortune and PUOLOP differ in overdischarge detect/release (2.40/3.00 V vs 2.50/2.90 V), short-circuit delay (5 µs vs 400 µs) and power-down current. Check which vendor's part is actually fitted.

---

## 2. FS8205A / 8205A dual N-MOSFET

### 2a. Fortune FS8205A (TSSOP-8)
File `fs8205a_fortune.pdf`, LCSC C16052 (listed as discontinued at LCSC). Document: **"Datasheet FS8205A – Dual N-Channel Enhancement Mode Power MOSFET", FS8205A-DS-12_EN, REV. 1.2, AUG 2009**, Fortune Semiconductor.

| Fact | Value | Document & rev | Page | Quote |
|---|---|---|---|---|
| RDS(on) @ VGS = 4.5 V (per FET, drain-source) | 21 mΩ typ, 25 mΩ max at ID = 4 A | FS8205A-DS-12 Rev 1.2 | 4 | "RDS(ON) Static Drain-Source On-Resistance VGS = 4.5V, ID = 4A - 21 25 mΩ" |
| RDS(on) @ VGS = 2.5 V (per FET) | 27 mΩ typ, 35 mΩ max at ID = 3 A | same | 4 | "VGS = 2.5V, ID = 3A - 27 35 mΩ" |
| RDS(on) @ VGS = 4.0 V | **NOT FOUND**: only 4.5 V and 2.5 V are specified | same | – | – |
| Source-to-source RSS(on) | **NOT FOUND**: only per-FET drain-source RDS(ON) is given. Two FETs in series ≈ 2 × RDS(on); this is an inference, not a datasheet value | same | – | – |
| Continuous drain current | 6 A @ TA = 25 °C, 5 A @ 70 °C (pulsed 25 A) | same | 3 | "ID @TA = 25℃ Continuous Drain Current3 6 A" / "ID @TA = 70℃ Continuous Drain Current3 5 A" |
| Package | TSSOP-8 | same | 3 | "FS8205A TSSOP8 package version TSSOP-8 3,000" |

### 2b. Fortune FS8205 (SOT23-6 version)
File `fs8205_fortune.pdf`, LCSC C32254. Document: **"Datasheet FS8205", FS8205-DS-17_EN, REV. 1.7, NOV 2011**, Fortune Semiconductor.

| Fact | Value | Document & rev | Page | Quote |
|---|---|---|---|---|
| RDS(on) @ 4.5 V (per FET) | 23 mΩ typ, 28 mΩ max at 4 A | FS8205-DS-17 Rev 1.7 | 4 | "RDS(ON) Static Drain-Source On-Resistance VGS = 4.5V, ID = 4A - 23 28 mΩ" |
| RDS(on) @ 2.5 V (per FET) | 30 mΩ typ, 37 mΩ max at 3 A | same | 4 | "VGS = 2.5V, ID = 3A - 30 37 mΩ" |
| RDS(on) @ 4.0 V / source-to-source | **NOT FOUND** | same | – | – |
| Continuous drain current | 6 A @ 25 °C, 5 A @ 70 °C | same | 3 | "ID @TA = 25℃ Continuous Drain Current3 6 A" |
| Package | SOT23-6 | same | 3 | "FS8205 SOT23-6 package version SOT23-6 3,000" |

### 2c. Clone: FUXINSEMI FS8205A (SOT-23-6)
File `fs8205a_fuxin.pdf`, LCSC C908265. Document: **"FS8205A N-Channel Enhancement Mode MOSFET", Ver2.1**, www.fuxinsemi.com.

| Fact | Value | Document & rev | Page | Quote |
|---|---|---|---|---|
| RDS(on) @ 4.5 V | 18 mΩ typ, 25 mΩ max at ID = 6.0 A | FUXINSEMI FS8205A Ver2.1 | 2 | "VGS =4.5V, ID =6.0A 18 25 … Drain-source on-resistance RDS(on) mΩ" |
| RDS(on) @ 2.5 V | 23 mΩ typ, 32 mΩ max at ID = 5.0 A | same | 2 | "VGS =2.5V, ID =5.0A 23 32" |
| RDS(on) @ 4.0 V / source-to-source | **NOT FOUND** | same | – | – |
| Continuous drain current | 6 A | same | 2 | "Continuous Drain Current ID 6 A" |
| Package | SOT-23-6L | same | 1 | "Package SOT-23-6L" |

---

## 3. TP4056 TEMP pin

Document A: **NanJing Top Power ASIC Corp. TP4056, REV_2.4**, file `tp4056_toppower_nanjing.pdf`. Downloaded earlier and reused; not downloaded again.
Document B: **Shenzhen TPOWER Semiconductor TP4056, V1.3** (Chinese), file `tp4056_tpower.pdf`.

| Fact | Value | Document & rev | Page | Quote |
|---|---|---|---|---|
| TEMP upper trip (VTEMP-H) | 80 %VCC typ, 82 % max | NJ REV_2.4 | 5 | "VTEMP-H TEMP upper trip threshold 80 82 %Vcc" |
| TEMP lower trip (VTEMP-L) | 43 % min, 45 %VCC typ | NJ REV_2.4 | 5 | "VTEMP-L TEMP lower trip threshold 43 45 %Vcc" |
| Pin behaviour | Charging is suspended when TEMP is below 45 % or above 80 % of VIN. Grounding TEMP disables the function | NJ REV_2.4 | 7 | "If TEMP pin's voltage is below 45% or above 80% of supply voltage VIN, this means that battery's temperature is too high or too low, charging is suspended." |
| Fixed fractions | K1 = 0.45, K2 = 0.80 | NJ REV_2.4 | 10 | "VTEMP_Lis fixed at (45%×Vcc), while VTEMP_H is fixed at (80%×Vcc)." |
| Divider formula (NTC). R1 goes from VIN to TEMP; R2 sits in parallel with the thermistor (‖ confirmed on the rendered page). RTL = NTC resistance at the low temperature TL, RTH = NTC resistance at the high temperature TH | R1 = RTL·RTH·(K2−K1) / ((RTL−RTH)·K1·K2); R2 = RTL·RTH·(K2−K1) / (RTL·(K1−K1·K2) − RTH·(K2−K1·K2)) | NJ REV_2.4 | 11 | "VTEMPL = R2‖RTL/(R1+R2‖RTL) × VIN" … "(K2=0.8)；VTEMPH＝VLOW＝K1×Vcc (K1=0.45)" … "R1 = RTL RTH (K2 − K1) / (RTL − RTH) K1 K2" |
| Supply independence / single-ended use | The trip temperatures do not depend on VCC. R2 can be left out if only the upper temperature matters | NJ REV_2.4 | 11 | "temperature monitor range is independent of power supply voltage VCC and it only depends on R1, R2, RTL and RTH" / "if considering only one terminal temperature (normally protecting from overheating), there is no need to use R2." |
| Thresholds in the SZ doc | Same values: 80 % typ / 82 % max and 43 % min / 45 % typ | SZ TPOWER V1.3 | 3 | "VTEMP-H TEMP 引脚高翻转电压 80 82 %VCC" / "VTEMP-L TEMP 引脚低翻转电压 43 45 %VCC" |
| SZ doc statement | 45 % / 80 % of VCC | SZ TPOWER V1.3 | 6 | "VTEMP-L=45%×VCC，VTEMP-H=80%×VCC" |
| Divider formula in SZ doc | **NOT FOUND**: no R1/R2 formula in V1.3 | SZ TPOWER V1.3 | – | – |

---

## 4. ESP32-DevKitC V4 header geometry

Document: **Espressif "ESP32-DevKitC V4 Dimensions" drawing, `esp32_devkitc_v4_dimensions.pdf`** from https://dl.espressif.com/dl/schematics/. The drawing has no revision number. PDF metadata: AutoCAD 2016, created 2023-06-28. 1 page, "Unit: mm".
Supporting document: `esp32_devkitc_v4_pcb_layout.pdf` (same folder on dl.espressif.com, 6 pages), which gives the board outline as 48.2 mm × 27.9 mm.

| Fact | Value | Document & rev | Page | Quote |
|---|---|---|---|---|
| Header row spacing (centre to centre) | **25.40 mm** (1000 mil). On the rendered drawing the extension lines start at the centres of the left and right pins | DevKitC V4 Dimensions (no rev, 2023-06-28) | 1 | "Unit: mm" … "25.40" |
| Header pitch | **2.54 mm** (dimensioned between GND and pin 23 on the right-hand row) | same | 1 | "2.54" |
| Board width | 27.94 mm | same | 1 | "27.94" |
| Board length | 48.26 mm | same | 1 | "48.26" |
| Other dimensions | Module width 18.00 mm; antenna overhang 6.04 mm; 1.30 mm and 1.29 mm edge offsets | same | 1 | "18.00" "6.04" "1.30" "1.29" |
| Board outline (layout PDF) | 48.2 mm × 27.9 mm | ESP32-DevKitC V4 PCB layout | 1 | "48.2mm" / "27.9mm" |

---

## 5. 12 V → 5 V buck (≥ 3 A)

There is no manufacturer datasheet for the generic pre-built modules themselves, so the IC datasheets are given here. A typical pre-built 5 A module uses the XL4015E1. LM2596 modules are rated 3 A, which leaves no margin above the requirement.

### 5a. XLSEMI XL4015 (recommended: 5 A rating)
File `xl4015_xlsemi.pdf`, LCSC C51661 (XL4015E1). Document: **"Datasheet XL4015 – 5A 180KHz 36V Buck DC to DC Converter", Rev 2.0**, www.xlsemi.com.

| Fact | Value | Document & rev | Page | Quote |
|---|---|---|---|---|
| Input range | 8–36 V (absolute maximum 40 V) | XL4015 Rev 2.0 | 1, 5 | "Wide 8V to 36V Input Voltage Range" / "Input operation voltage Vin 8 36 V" |
| Max output current | 5 A | same | 1 | "5A Constant Output Current Capability" |
| Switch current limit | 7 A typ | same | 5 | "Switch Current Limit IL VFB =0V 7 A" |
| Efficiency at 12 V → 5 V | **87 % typ at 5 A** | same | 5 | "Vin=12V,Vout=5V Iout=5A η Efficiency - 87 - %" |
| Peak efficiency | up to 96 % | same | 1 | "High efficiency up to 96%" |

### 5b. TI LM2596 (3 A)
File `lm2596_ti.pdf`. Document: **TI "LM2596 SIMPLE SWITCHER Power Converter 150-kHz 3-A Step-Down Voltage Regulator", SNVS124G – November 1999 – revised March 2023**.

| Fact | Value | Document & rev | Page | Quote |
|---|---|---|---|---|
| Input range | 4.5–40 V (operating); 45 V absolute maximum | SNVS124G | 5 | "Supply voltage 4.5 40 V" |
| Input range (5 V fixed version, regulation spec) | 7–40 V | SNVS124G | 6 | "7 V ≤ VIN ≤ 40 V, … 0.2 A ≤ ILOAD ≤ 3 A … 4.8 5 5.2" |
| Max output current | 3 A | SNVS124G | 1 | "3-A output load current" |
| Efficiency at 12 V → 5 V | **80 % typ at 3 A** (LM2596-5.0) | SNVS124G | 6 | "η Efficiency VIN = 12 V, ILOAD = 3 A 80%" |

---

## Failed sources
- `docs.espressif.com`: the proxy now refuses it ("CONNECT tunnel failed, response 403"), so the DevKitC V4 user-guide HTML could not be fetched. The dimension and layout PDFs were downloaded from dl.espressif.com instead.
- `https://dl.espressif.com/dl/schematics/esp32-devkitc-v4_dimensions.pdf` and `…/esp32_devkitc_v4-dimension.pdf`: 404, because the file names were guessed. The correct name is `esp32_devkitc_v4_dimensions.pdf`.
- `https://dl.espressif.com/dl/schematics/esp32_devkitc_v4_sch_20180607.zip`: 404.
- The LCSC "lcsc_datasheet_…pdf" URLs on www.lcsc.com and datasheet.lcsc.com/lcsc/… return an HTML viewer page, not the PDF. The real PDF link (`datasheet.lcsc.com/datasheet/pdf/<hash>.pdf?productCode=Cxxxx`) was taken from each product page.
- LCSC C351410 is a **PUOLOP** DW01A, not Fortune. An earlier listing and search snippets attributed it to Fortune. The genuine Fortune document was taken from C61503 (DW01A-G).
- Values not given in any document read: FS8205/FS8205A RDS(on) at VGS = 4.0 V and source-to-source RSS(on); a DW01A-G power-down current (the Fortune DW01A-G table has none); a divider formula in the SZ TPOWER V1.3 TP4056 document.
