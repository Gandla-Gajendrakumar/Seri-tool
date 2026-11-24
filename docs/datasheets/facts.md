# Datasheet facts: IoT climate monitor carrier PCB (phase 4)

Every value below was read from the downloaded document named in the row. "Page" is the PDF page number. For every document except the 18650 spec (where the footer says "Page N Of 19"), the PDF page number matches the page number printed on the page. Where pdftotext dropped a "±" glyph, the sign was checked against the rendered page. **NOT FOUND** means the value is not in any document I could download from the allowed hosts. A value read off a graph rather than a table is marked "(graph read)".

---

## 1. Espressif ESP32-WROOM-32E module

Docs: *ESP32-WROOM-32E & ESP32-WROOM-32UE Datasheet v2.1* (WROOM) and *ESP32 Series Datasheet v5.3* (SERIES).

| Fact | Value | Document & rev | Page | Quote |
|---|---|---|---|---|
| VDD operating range | 3.0 – 3.3 – 3.6 V | WROOM v2.1, Table 14 | 28 | "VDD33 Power supply voltage 3.0 3.3 3.6 V" |
| Min. supply current capability | ≥ 0.5 A | WROOM v2.1, Table 14 | 28 | "IVDD Current delivered by external power supply 0.5 — — A" |
| Wi-Fi TX current | 239 mA avg / **379 mA peak** (802.11b 1 Mbps @19.5 dBm); 190/276 mA (11g 54M); 183/258 mA (11n HT20 MCS7) | WROOM v2.1, Table 16 | 29 | "802.11b, 20 MHz, 1 Mbps, @19.5 dBm 239 379" |
| RX current | 112 mA (b/g/n HT20), 118 mA (HT40) | WROOM v2.1, Table 16 | 29 | "802.11b/g/n, 20 MHz 112 112" |
| Absolute max VDD | −0.3 to 3.6 V | WROOM v2.1, Table 13 | 28 | "VDD33 Power supply voltage –0.3 3.6 V" |
| Input-only GPIOs | GPIO34, 35, 36 (SENSOR_VP), 39 (SENSOR_VN) on the module. They have no output driver and no internal pull-up/pull-down. | SERIES v5.3, Table 6-1; WROOM v2.1 pin table (type "I") | SERIES 61; WROOM 11 | "GPIO pins 34-39 are input-only. These pins do not feature an output driver or internal pull-up/pull-down circuitry." |
| Strapping defaults | GPIO0 pull-up (1); GPIO2 pull-down (0); MTDI/GPIO12 pull-down (0); MTDO/GPIO15 pull-up (1); GPIO5 pull-up (1) | WROOM v2.1, Table 4 | 13 | "GPIO0 Pull-up 1 / GPIO2 Pull-down 0 / MTDI Pull-down 0 / MTDO Pull-up 1 / GPIO5 Pull-up 1" |
| Boot mode (GPIO0/GPIO2) | SPI (normal) boot: GPIO0 = 1, GPIO2 = any. Download boot: GPIO0 = 0 **and** GPIO2 = 0. | WROOM v2.1, Table 6 | 14 | "SPI Boot Mode 1 Any value / Joint Download Boot Mode 0 0" |
| GPIO12 (MTDI) | Must be 0 at reset (the default) so VDD_SDIO = 3.3 V. With MTDI = 1, VDD_SDIO comes from the internal 1.8 V LDO. | WROOM v2.1 §4.2 | 15 | "MTDI = 0 (by default), VDD_SDIO pin is powered directly from VDD3P3_RTC… MTDI = 1, VDD_SDIO pin is powered from internal 1.8 V LDO." |
| GPIO15 (MTDO) | 1 (default): U0TXD boot log enabled; 0: log disabled. Together with GPIO5 it sets SDIO-slave timing (default 1/1). | WROOM v2.1, Tables 7–8 | 16 | "U0TXD Printing Control… Enabled 1 … Disabled 0" |
| GPIO5 | Default 1. Only affects SDIO-slave timing. | WROOM v2.1, Table 8 | 16 | "Rising edge sampling, rising edge output 1 1" |
| Strapping hold time | tH ≥ 1 ms after CHIP_PU goes high | WROOM v2.1, Table 5 | 14 | "tH … before these pins start operating as regular IO pins. 1" |
| ADC range at 11 dB (atten = 3) | Effective range 150 – 2450 mV, total error ±60 mV after eFuse-Vref calibration. Accuracy degrades above about 2450 mV. | WROOM v2.1, Tables 10–11 | 25–26 | "Atten = 3, effective measurement range of 150 ∼ 2450 mV –60 60 mV" |
| CHIP_PU / EN | Must stay low ≥ 50 µs to reset; 3.3 V must be stable ≥ 50 µs before EN rises. | WROOM v2.1, Table 9 | 17 | "tSTBL … 50", "tRST … 50" |

## 2. ESP32-DevKitC V4

| Fact | Value | Document & rev | Page | Quote |
|---|---|---|---|---|
| On-board 3.3 V regulator | AMS1117-3.3 | *ESP32_DevKitc_V4* schematic, Rev V4, dated 6 Dec 2017 | 1 (single sheet) | Schematic part label "AMS1117-3.3" (VIN to VOUT) |
| Powering through the 3V3 pin | Allowed. It is one of three mutually exclusive options. | *ESP32-DevKitC V4 Getting Started Guide* (esp-dev-kits "latest", HTML page, no page numbers), section "Power Supply Options" | n/a (HTML) | "There are three mutually exclusive ways to provide power to the board: Micro USB port, default power supply; 5V and GND header pins; 3V3 and GND header pins" |
| One-source warning | Use exactly one source | same, Warning box | n/a | "The power supply must be provided using one and only one of the options above, otherwise the board and/or the power supply source can be damaged." |
| C15 note | On early boards C15 can force download mode; remove it if that happens | same, "Note on C15" | n/a | "The board may boot into Download mode … please remove the component." |

## 3. SIMCom A7670 series

Doc: *A7672X/A7670X Series Hardware Design V1.03, 2022-11-02, Released* (HD). It covers the A7672S, A7672E/**A7670E** and A7672SA/A7670SA. Also used: the A7670E product brief "V: 2021.03" (BRIEF).

| Fact | Value | Document & rev | Page | Quote |
|---|---|---|---|---|
| VBAT range | 3.4 min / 3.8 typ / 4.2 max V | HD V1.03, Table 8 and Table 42 | 26, 56 | "VBAT Module supply voltage 3.4 3.8 4.2 V" |
| VBAT absolute max | 4.8 V | HD V1.03, Table 41 | 56 | "Voltage on VBAT -0.5 - 4.8 V" |
| Peak current | 2 A (GSM TX burst at max power). Supply must deliver ≥ 2 A so the droop stays < 300 mV. | HD V1.03 §3.1, Table 8 | 26 | "the peak current can reach 2A … the power supply capacity of external power supply must be no less than 2A." |
| LTE TX current | Typ. 600 mA (B1/B3 @23 dBm), 580 mA (B5), 540 mA (B7/B20), 520 mA (B8) | HD V1.03, Table 46 | 60 | "LTE-FDD B1 @10MHz 23dBm Typical :600 mA" |
| GPRS current | EGSM900 2Rx/4Tx, power level #5: typ. 630 mA | HD V1.03, Table 46 | 59 | "EGSM900( 2 Rx,4 Tx ) @power level #5 Typical: 630mA" |
| Bulk capacitance near VBAT | ≥ 300 µF total if the supply can deliver 2 A peak; ≥ 600 µF if it cannot. Also 33 pF + 10 pF + 0.1 µF + 1 µF ceramics, VBAT trace ≥ 3 mm wide, and a TVS. The EVB used a 330 µF tantalum. | HD V1.03 §3.1.1 | 26–27 | "the total capacity of the external power supply capacitance is recommended to be no less than 300uf. If the peak current of 2A cannot be provided … no less than 600uf" |
| UART logic level | **1.8 V.** All UART pins are in the 1.8 V domain. Absolute max on digital pins (GPIO, I2C, UART, PCM) is **2.0 V**. | HD V1.03, Table 7, Table 41 | 19, 56 | "RXD 10 1.8V DI Data input"; "Voltage at digital pins (GPIO, I2C, UART,PCM) -0.3 - 2.0 V" |
| 1.8 V reference output | The pin is named VDD_1V8 (pin 15) in this revision: 1.8 V, up to 50 mA, intended for level-conversion circuits. "VDD_EXT" appears only in a test-point note. | HD V1.03, Table 7 | 18 (VDD_EXT note p22) | "1.8V power output, output current up to 50 mA … can provide power for level conversion circuit" |
| 1.8 V IO thresholds | VIH ≥ 0.7×VCC, VIL ≤ 0.3×VCC, VOH ≥ VCC−0.2, VOL ≤ 0.2 V | HD V1.03, Table 43 | 56 | "VIH High-level input voltage VCC*0.7 1.8 VCC+0.2 V" |
| Recommended level shifter | Discrete transistor circuit with MMBT3904. Not recommended above 115200 bps. Default main UART baud rate is 115200. | HD V1.03 §3.3.1 | 33–34 | "The recommended triode model is MMBT3904." / "not recommended to use this circuit when the signal speed is higher than 115200bps." |
| PWRKEY power-on pulse | Ton typ. 50 ms low. STATUS goes high after typ. 7 s; UART ready typ. 8 s. PWRKEY is pulled up internally to VBAT. Do not put more than 100 nF on PWRKEY. | HD V1.03, Table 10 and §3.2.1 | 29–30 | "Ton The time of active low-level impulse of PWRKEY pin to power on module - 50 - ms" |
| A7670E bands | LTE-FDD B1/B3/B5/B7/B8/B20; GSM 900/1800 | HD V1.03, Table 1 (rendered page checked); BRIEF | HD 10; BRIEF 2 | "LTE-FDD B1/B3/B5/B7/B8/B20 … GSM/GPRS/EDGE 900/1800 MHz" |
| **A7670C bands** | **NOT FOUND.** The A7670C is not in Table 1 of HD V1.03. The LCSC A7670C page (C2840705) has no datasheet, and the SIMCom downloads need a login. | — | — | — |
| Sleep / idle current | **NOT FOUND.** The document lists them as "TBD". | HD V1.03, Table 2 and Table 46 | 12, 59 | "Current in sleep mode: TBD" |
| Power-off leakage | 20 µA typ. | HD V1.03, Table 8 | 26 | "Module average consumption current (off leakage current) - 20 - uA" |
| Brown-out behaviour | RF performance is affected below 3.4 V. Use a regulator with an enable pin controlled by the MCU. | HD V1.03 §3.1.1 | 27 | "If the voltage drops below 3.4V, the RF performance of the module will be affected" |

## 4. Sensirion SHT3x-DIS (SHT31)

Doc: *Datasheet SHT3x-DIS, December 2022, Version 7*.

| Fact | Value | Document & rev | Page | Quote |
|---|---|---|---|---|
| VDD range | 2.15 – 3.3 – 5.5 V (absolute max −0.3 to 6 V) | Ver. 7, Table 3 / Table 6 | 6 / 7 | "Supply voltage VDD 2.15 3.3 5.5 V" |
| I2C addresses | 0x44 (ADDR low, default); 0x45 (ADDR high). ADDR must not float. | Ver. 7, Table 8 | 9 | "I2C address A 0x44 (default) … I2C address B 0x45" |
| Measurement time, high repeatability | 12.5 ms typ / 15 ms max (2.4–5.5 V); 15.5 ms max (2.15–2.4 V) | Ver. 7, Tables 4–5 | 7 | "tMEAS,h High repeatability - 12.5 15 ms" |
| SHT31 RH accuracy | ±2 %RH typ. (maximum: Figure 3) | Ver. 7, Table 1 | 2 | "SHT31 Accuracy tolerance Typ. 2 %RH" |
| SHT31 T accuracy | ±0.2 °C typ., 0–90 °C (maximum: Figure 9) | Ver. 7, Table 2 | 2 | "SHT31 Accuracy tolerance typ., 0°C to 90°C 0.2 °C" |
| Supply current | Idle (single-shot) 0.2 µA typ / 2.0 µA max at 25 °C; idle (periodic mode) 45 µA; measuring 600 µA typ / 1500 µA max; average 1.7 µA at 1 Hz low repeatability | Ver. 7, Table 3 | 6 | "Measuring - 600 1500 µA" |
| Bus capacitance / cable | CB ≤ 400 pF per bus line. No cable-length figure is given: the datasheet says to size the pull-ups for bus capacitance and frequency, e.g. Rp = 10 kΩ. Decouple with 100 nF placed close to the sensor. | Ver. 7, Table 21 and §3.2 | 14, 8 | "Capacitive load on bus line CB - - 400 pF" |
| CRC | CRC-8, polynomial 0x31 (x⁸+x⁵+x⁴+1), init 0xFF, check CRC(0xBEEF) = 0x92 | Ver. 7, Table 20 | 14 | "Polynomial 0x31 (x8 + x5 + x4 + 1) … Initialization 0xFF" |

## 5. Maxim/ADI DS3231

Doc: *DS3231 Extremely Accurate I2C-Integrated RTC/TCXO/Crystal, 19-5170 Rev 7, 3/10*. The file is LCSC's mirror for part C9866; analog.com dropped the connection.

| Fact | Value | Document & rev | Page | Quote |
|---|---|---|---|---|
| VCC range | 2.3 – 3.3 – 5.5 V | Rev 7, Recommended DC Operating Conditions | 2 | "Supply Voltage VCC 2.3 3.3 5.5 V" |
| VBAT range | 2.3 – 3.0 – 5.5 V | same | 2 | "VBAT 2.3 3.0 5.5 V" |
| I2C address | 7-bit 1101000b = 0x68 | Rev 7, I2C section | 16–17 | "the 7-bit DS3231 address, which is 1101000" |
| Accuracy | ±2 ppm 0 to +40 °C; ±3.5 ppm −40 to +85 °C (industrial part) | Rev 7, front page and EC table | 1, 3 | "Accuracy ±2ppm from 0°C to +40°C" |
| Active current (VCC) | 200 µA max at 3.63 V; 300 µA max at 5.5 V | Rev 7, EC table | 2 | "Active Supply Current ICCA … VCC = 3.63V 200" |
| Standby current (VCC) | 110 µA max at 3.63 V (I2C idle, 32 kHz on) | Rev 7, EC table | 2 | "Standby Supply Current ICCS … 110" |
| Battery timekeeping current | 0.84 µA typ / 3.0 µA max at VBAT = 3.63 V | Rev 7, EC table (VCC = 0) | 3 | "Timekeeping Battery Current IBATT … 0.84 3.0" |
| Power-fail switchover | VPF 2.45 / 2.575 / 2.70 V | Rev 7, EC table | 2 | "Power-Fail Voltage VPF 2.45 2.575 2.70 V" |
| OSF meaning | Bit 7 of the status register (0Fh). 1 means the oscillator is stopped or has stopped at some point, so the time is not trustworthy. It is set at first power-up and stays 1 until written 0. | Rev 7, Status Register | 14 | "A logic 1 in this bit indicates that the oscillator either is stopped or was stopped for some period and may be used to judge the validity of the timekeeping data." |

## 6. 58 mm TTL thermal panel printer (CSN-A2 / "A2")

Docs: *CSN-A2 Micro panel printer's User Manual, Version 4.0* (CSN) and *A2 Micro panel thermal printer* user manual, undated, 24 pp. (A2M). Both are from cdn-shop.adafruit.com.

| Fact | Value | Document & rev | Page | Quote |
|---|---|---|---|---|
| Supply voltage | DC 5–9 V (CSN also lists a 12 V option). The status byte flags VH > 9.5 V. | CSN v4.0 §1 and §5.1; A2M spec | CSN 4, 8, 40; A2M 4 | "Input power DC5-9V or 12V"; "VH(input voltage, 5V-9V)" |
| Peak current while printing | **NOT FOUND** as a number. The manual only says peak current scales with the "max heating dots" setting (default 64 dots). | CSN v4.0, ESC 7 | 48 | "The more max heating dots, the more peak current will cost when printing" |
| TTL logic level | **NOT FOUND** (neither 5 V nor 3.3 V tolerance is stated). The CN4 "TTL connector" on the USB-board variant has a pin labelled "+3.3V". | CSN v4.0 §5.3.2 | 9 | "CN4 TTL connector … 1 +3.3V / 2 Transmit data (TXD, printer output) / 3 Receive data (RXD, printer input) / 4 GND" |
| Default baud rate | **NOT FOUND** in either manual | — | — | — |
| Print width | 384 dots/line (8 dots/mm); 48 mm print width on 58 mm paper | CSN v4.0 §1; A2M | CSN 4; A2M 4 | "Resolution ration 8dots/mm(384dots/line)" |
| Real-time status (DLE EOT) | **NOT FOUND.** DLE EOT is not listed in either command table. | — | — | — |
| Paper-out status | Supported through `ESC v n` (1B 76 n). CSN: returns one byte, bit 2 (0x04) = paper out, bit 3 = VH > 9.5 V, bit 6 = temperature > 60 °C. A2M: returns an ASCII string such as "P1V72T30". `GS a n` enables Automatic Status Back. | CSN v4.0 §8.2.6; A2M §5.2.7 | CSN 40–41; A2M 20 | "For example: return “0x04” is means paper out." / "Example: P1V72T30 Mean:Paper Ready,Current voltage 7.2V" |
| Sleep | `ESC 8 n1 n2` sets the idle time before sleep. To wake: send 0xFF, then wait 50 ms. | CSN v4.0 | 48 | "host must send one byte (0xff) to wake up control board. And waiting 50ms" |
| Print speed vs voltage | 60 mm/s; up to 80 mm/s at 8.5 V | CSN v4.0 §1 | 4 | "Printing speed 60mm/sec.; Max.:80MM/sec.(voltage 8.5V)" |

## 7. TP4056 Li-ion charger

Docs: **NanJing Top Power ASIC Corp. TP4056 REV_2.4** (NJ; LCSC C16581, TP4056-42-ESOP8) and Shenzhen TPOWER TP4056 V1.3 (SZ; LCSC C382139, in Chinese) for comparison.

| Fact | Value | Document & rev | Page | Quote |
|---|---|---|---|---|
| Input voltage, operating | NJ: 4.0 – 5 – 8.0 V. SZ: 4 – 5 – 6 V. | NJ REV_2.4 EC table; SZ V1.3 | NJ 5; SZ 3 | "VCC Input Supply Voltage ● 4.0 5 8.0 V" |
| Input absolute max | NJ: −0.3 to 8 V. SZ: −0.3 to 7 V. | NJ REV_2.4; SZ V1.3 | NJ 2; SZ 2 | "Input Supply Voltage(VCC)：-0.3V～8V" |
| Charge current formula | RPROG = 1100 / IBAT (V/A), i.e. IBAT = (VPROG/RPROG)×1100 with VPROG = 1.0 V. Max 1000 mA. | NJ REV_2.4 | 7, 9 | "RPROG = 1100 / IBAT" |
| RPROG table (NJ) | 30k→50 mA, 20k→70, 10k→130, 5k→250, 4k→300, 3k→400, 2k→580, 1.6k→690, 1.4k→780, 1.2k→900, **1.1k→1000 mA** | NJ REV_2.4 | 9 | "1.2 900 / 1.1 1000" |
| Charge current tolerance | RPROG = 1.1k: 850 / 1000 / 1150 mA. RPROG = 2.2k: 425 / 500 / 575 mA. | NJ REV_2.4 EC table | 5 | "RPROG=1.1K, Current Mode ● 850 1000 1150 mA" |
| Termination (float) voltage | 4.158 / 4.2 / 4.242 V (±1 %), 0–85 °C | NJ REV_2.4 EC table | 5 | "VFLOAT … 4.158 4.2 4.242 V" |
| Trickle charge | Threshold 2.8/2.9/3.0 V. Current 120/130/140 mA at RPROG = 1.1k. | NJ REV_2.4 EC table | 5 | "ITRIKL Trickle Charge Current VBAT<VTRIKL, RPROG=1.1K ● 120 130 140 mA" |
| Termination current | C/10: 120/130/140 mA at 1.1k; 60/70/80 mA at 2.2k | NJ REV_2.4 EC table | 5 | "C/10 termination current threshold RPROG=1.1K ● 120 130 140 mA" |
| Recharge threshold | 4.05 V | NJ REV_2.4 | 9 | "drops below the 4.05V recharge threshold" |
| Load on BAT pin | After termination, every BAT-pin load must be supplied by the battery. Transient BAT loads can disturb C/10 detection. There is no power path. | NJ REV_2.4 | 9 | "In this state all loads on the BAT pin must be supplied by the battery." |

## 8. 3.3 V LDO

Docs: Diodes *AP2112, DS39724 Rev. 2-2 (June 2017)* and Microne *ME6211, V14*.

| Fact | Value | Document & rev | Page | Quote |
|---|---|---|---|---|
| AP2112K-3.3 max output current | 600 mA (min) | DS39724 Rev 2-2, AP2112-3.3 EC | 8 | "IOUT(MAX) Maximum Output Current VIN = 4.3V … 600" |
| AP2112K-3.3 dropout | 5/8 mV @10 mA; 125/200 mV @300 mA; **250 typ / 400 max mV @600 mA** | same | 8 | "IOUT = 600mA — 250 400" |
| AP2112K-3.3 quiescent current | 55 µA typ / 80 µA max; standby 0.01 µA | same | 8 | "IQ Quiescent Current VIN = 4.3V, IOUT = 0mA — 55 80 µA" |
| AP2112 input range | 2.5 – 6.0 V recommended; absolute max 6.5 V | DS39724, Recommended Operating / Abs Max | 3 | "VIN Supply Voltage 2.5 6.0 V" |
| AP2112 current limit | Short-circuit current 50 mA typ. (foldback) | same | 8 | "ISHORT Short Current Limit VOUT = 0V — 50 — mA" |
| ME6211C33 max output current | 500 mA (VIN = VOUT + 1 V) | ME6211 V14 | 8 | "Maximum Output Current IOUTMAX VIN= VOUT+1V 500 mA" |
| ME6211C33 dropout | 120 mV @100 mA; 260 mV @200 mA (typ.) | ME6211 V14 | 8 | "VDIF2 IOUT =200mA 260 mV" |
| ME6211C33 quiescent current | 60 µA typ. | ME6211 V14 | 8 | "Supply Current ISS VIN= VOUT+1V 60 μA" |
| ME6211 input range | 2 V – 6.0 V operating; 6.5 V absolute max | ME6211 V14 | 1, 5 | "Operating Voltage Range: 2V～6.0V" |

## 9. MPS MP1584EN buck

| Fact | Value | Document & rev | Page | Quote |
|---|---|---|---|---|
| Input range / max Iout / efficiency at 12→5 V | **NOT FOUND.** No copy on the allowed hosts: the LCSC product pages for C15051, C21999497 and C7304223 return "Page Not Found", and monolithicpower.com, mouser.com and jlcpcb.com are not allowed. Search snippets say "4.5V to 28V, 3A", but I did not read a datasheet, so treat that as unverified. | — | — | — |

## 10. Boost converters

Doc: TI *TPS61088 10A 全集成同步升压转换器, ZHCSDP8A, May 2015, revised May 2015 (Rev. A)*. This is the Chinese edition of SLVSCM8, mirrored by LCSC for C92483. ti.com returned 404 for every datasheet URL. Also *MT3608 Rev1.1* (Aerosemi, LCSC C84817).

| Fact | Value | Document & rev | Page | Quote |
|---|---|---|---|---|
| TPS61088 input range | 2.7 – 12 V. UVLO rising ≤ 2.7 V; falling 2.4/2.5 V. | ZHCSDP8A, EC table | 1, 5 | "VIN Input voltage range 2.7 12 V" |
| TPS61088 output range | 4.5 – 12.6 V | ZHCSDP8A, features | 1 | "输出电压范围：4.5 至 12.6V" |
| TPS61088 switch current limit | 10 A rated, adjustable with RILIM. At RILIM = 100 kΩ: PFM 10.6/11.9/13 A; FPWM 9.0/10.3/11.4 A. | ZHCSDP8A, EC table | 1, 5 | "Peak switch current limit in PFM mode RILIM = 100 kΩ … 10.6 11.9 13 A" |
| TPS61088 efficiency, 3.6 V in → 5 V out at 2 A | About 95–96 % (graph read, Fig. 2 FPWM / Fig. 4 PFM, "5-V Output" curve). Not a tabulated value. | ZHCSDP8A, §7.6 Fig. 2/4 | 6 | Figure title: "Efficiency vs Output Current, VIN = 3.6 V" |
| MT3608 switch current limit | 4 A typ. (VIN = 5 V, 50 % duty) | MT3608 Rev1.1, EC table | 3 | "SW Current Limit VIN= 5V, Duty cycle=50% 4 A" |
| MT3608 input range | 2 – 24 V | MT3608 Rev1.1 | 1, 3 | "Operating Input Voltage 2 24 V" |

## 11. 18650 Li-ion cell

Doc: Great Power Battery Co. *Product Specification, Model 18650 (2600 mAh 3.7 V), Ver A/1, 2012-7-18, doc YXY-ICR18650-A-0001*. Hosted by SparkFun at cdn.sparkfun.com. I found no Samsung or LG cell datasheet on the allowed hosts.

| Fact | Value | Document & rev | Page | Quote |
|---|---|---|---|---|
| Nominal capacity | 2600 mAh (minimum 2500 mAh), 0.5C discharge | Ver A/1, §3 | 8 | "Nominal Capacity 2600mAh … Minimum Capacity 2500mAh" |
| Nominal voltage | 3.7 V | Ver A/1, §3 | 8 | "Nominal Voltage 3.7V" |
| Charge voltage | 4.2 V ± 0.03 V. Standard charge 0.5C (1300 mA); max charge 1C (2600 mA). | Ver A/1, §3 | 8, 10 | "Charge Voltage 4.2V±0.03V" |
| Discharge cut-off | 3.0 V | Ver A/1, §3 item 8 | 8 | "0.2C constant current discharge to3.0V" |
| Max continuous discharge | 1.5C = 3900 mA (the doc writes "3900mAh") | Ver A/1, §3 item 12 | 10 | "Maximum discharge current 1.5C 3900mAh" |
| Internal resistance | ≤ 60 mΩ (AC 1 kHz, 50 % SoC) | Ver A/1, §3 item 9 | 8 | "Cell Internal Impedance ≤60mΩ … measured at AC 1KHZ after 50% charge" |
| Charge temperature | 0 – 45 °C (discharge −20 to 60 °C) | Ver A/1, §3 item 13 | 10 | "Charge/充电 0~45℃" |

## 12. AO3401A P-MOSFET and SS34 Schottky

Docs: AOS *AO3401A, Rev 3.1, December 2023* (LCSC C15127) and MDD *SS32 thru SS3200, Rev:2024A5* (LCSC C8678).

| Fact | Value | Document & rev | Page | Quote |
|---|---|---|---|---|
| AO3401A VDS | −30 V | Rev 3.1 | 1 | "Drain-Source Voltage VDS -30 V" |
| AO3401A VGS max | ±12 V | Rev 3.1 | 1 | "Gate-Source Voltage VGS ±12 V" |
| AO3401A continuous ID | −4.0 A (TA = 25 °C); −3.2 A (70 °C) | Rev 3.1 | 1 | "Continuous Drain Current TA=25°C -4 / TA=70°C -3.2" |
| AO3401A RDS(on) at −4.5 V | 47 typ / 60 max mΩ (ID = −3.5 A) | Rev 3.1 | 2 | "VGS=-4.5V, ID=-3.5A 47 60 mΩ" |
| AO3401A RDS(on) at −2.5 V | 60 typ / 85 max mΩ (ID = −2.5 A) | Rev 3.1 | 2 | "VGS=-2.5V, ID=-2.5A 60 85 mΩ" |
| AO3401A VGS(th) | −0.5 / −0.9 / −1.3 V | Rev 3.1 | 2 | "VGS(th) Gate Threshold Voltage … -0.5 -0.9 -1.3 V" |
| SS34 VF at 3 A | 0.55 V max (shared cell for SS32–SS34) | Rev:2024A5 | 1 | "Maximum instantaneous forward voltage at 3.0A VF 0.55" |
| SS34 VF at 1 A | **NOT FOUND** as a tabulated value. Only the typical curve (Fig. 3) is given. | Rev:2024A5 | 2 | "Fig.3 Typical Forward Characteristic" |
| SS34 max current | 3.0 A average (derated above 100 °C lead temperature to 0 A at 125 °C); IFSM 80 A; VRRM 40 V | Rev:2024A5 | 1–2 | "Maximum average forward rectified current … 3.0 A" |

---

## Design implications

1. **The modem UART is 1.8 V, and its pins are rated only to 2.0 V absolute max** (HD V1.03 p56). Connecting the ESP32's 3.3 V TX directly would exceed that rating, so a level shifter is required on TXD/RXD and on any DTR/RI/PWRKEY logic.
   - SIMCom's MMBT3904 transistor circuit is rated only up to 115200 bps.
   - For a faster link, use a MOSFET or translator-IC shifter. Reference the 1.8 V side to the module's VDD_1V8 (pin 15, 50 mA, p18).
   - The default baud rate is 115200 (p34).
2. **The modem's VBAT window is 3.4–4.2 V (absolute max 4.8 V).** It needs ≥ 2 A peak with < 300 mV droop, plus ≥ 300 µF bulk (≥ 600 µF if the source cannot deliver 2 A) and the 33 pF/10 pF/0.1 µF/1 µF ceramics close to the pins.
   - Running VBAT straight off the 18650: the cell's cut-off is 3.0 V, so the modem is out of spec (RF degraded below 3.4 V) for the last part of the discharge. Either cut the modem off with low-battery firmware at about 3.5 V, or feed it from a dedicated 3.8 V regulator with an enable pin, as SIMCom recommends (p27).
   - It must **never** connect to the 5 V boost rail directly.
3. **ESP32 supply.** The external supply must deliver ≥ 0.5 A (Table 14); TX peaks reach 379 mA.
   - AP2112K-3.3 (600 mA) meets this. ME6211C33 (500 mA at VIN = VOUT + 1 V) only just reaches 0.5 A, and only with 4.3 V in, so prefer the AP2112K.
4. **LDO dropout at low battery.**
   - AP2112K-3.3 dropout is 250 mV typ / 400 mV max at 600 mA, and 125/200 mV at 300 mA. Fed from the cell, 3.3 V regulation is lost below about 3.5–3.7 V cell voltage.
   - The ESP32 minimum is 3.0 V. With 200 mV max dropout at 300 mA, VDD stays ≥ 3.0 V only while the cell is above about 3.2 V. Set the firmware low-battery shutdown above that, or feed the LDO from a regulated rail.
5. **The DevKitC is "one source only"** (3V3 pin powering is allowed). If the carrier feeds the DevKitC's 3V3 or 5V pin, plugging in USB at the same time back-feeds the carrier and its AMS1117-3.3.
   - Add a Schottky/ideal-diode OR on the carrier's feed, or document "unplug carrier power before USB".
   - An SS34 in a 5 V path drops up to 0.55 V at 3 A.
6. **Strapping pins.**
   - GPIO12/MTDI must be low at reset. If high, VDD_SDIO switches to 1.8 V and the 3.3 V flash will not boot.
   - GPIO0 must be high for normal boot. GPIO0 = 0 plus GPIO2 = 0 enters download mode.
   - GPIO15/MTDO high enables the boot log.
   - Do not route the modem TX, printer TX, or any externally driven or pulled line onto GPIO0, 2, 5, 12 or 15. The modem and printer can be powered while the ESP32 resets.
   - GPIO34–39 are input-only with no pull-ups. They suit ADC/battery sense and the printer RX, and need external pull-ups if used as inputs.
7. **Battery ADC.** At atten = 3 (11 dB) the usable range is 150–2450 mV (±60 mV calibrated). A 4.2 V cell needs a divider of at least 1:2 (4.2 V → 2.1 V). Use an ADC1 pin (GPIO32–39).
8. **Printer supply.** The printer needs 5–9 V on VH. The datasheets give no peak-current figure; it scales with the "max heating dots" setting (ESC 7, default 64 dots). Treat the printer as the largest load and limit heating dots in firmware.
   - **SS34 OR-ing in the printer path** drops up to 0.55 V, which takes a 5.0 V rail below the 5 V minimum. Use an ideal-diode/P-FET switch instead, or set the boost slightly higher (still ≤ 9 V).
9. **Boost converter choice.** TPS61088 (10 A switch limit, adjustable via RILIM; about 95 % at 3.6 V→5 V/2 A by graph read) has ample margin. The MT3608's 4 A switch limit is marginal for 5 V/2 A from a cell. Estimate (my derivation, not from the datasheet): Iout ≈ ILIM×(VIN/VOUT)×η ≈ 4×(3.0/5)×0.9 ≈ 2.2 A before inductor ripple. So the MT3608 cannot reliably supply 2 A at low battery.
10. **Cell current budget.** A 5 V/2 A load at 3.0 V cell voltage and about 90 % efficiency draws about 3.7 A from the cell (derived). That is near the cell's 3.9 A (1.5C) continuous maximum, before adding modem 2 A bursts. Avoid printing while the modem transmits, and set TPS61088 RILIM to cap cell current.
11. **TP4056.**
   - RPROG = 1.1 kΩ gives 1.0 A (850–1150 mA), below the cell's 1C max charge and close to its 0.5C (1300 mA) standard charge, so it is acceptable. 1.2 kΩ gives about 900 mA if thermal limits apply.
   - Input range: NJ allows 4.0–8.0 V; the Shenzhen TPOWER clone is 4–6 V (absolute max 7 V). Keep VIN at USB 5 V and pin the vendor in the BOM.
   - There is no power path: system loads on BAT can corrupt C/10 termination (NJ p9). Add load-sharing (P-FET plus Schottky) between USB 5 V and the battery.
   - The cell's charge temperature range is 0–45 °C. Wire the TEMP pin to an NTC, or tie it to GND to disable the check.
12. **AO3401A as a high-side switch.** VGS(th) is −0.5 to −1.3 V and the gate is rated ±12 V.
   - An ESP32 GPIO at 3.3 V on the gate, with the source at 4.2 V (battery), gives VGS = −0.9 V when "off". The FET may partly conduct. With the source at 5 V, VGS = −1.7 V and the FET is on.
   - So drive the gate through an NPN/N-MOSFET with a pull-up to the source rail; do not drive it directly from a GPIO.
   - When fully on, RDS(on) is ≤ 60 mΩ at −4.5 V and ≤ 85 mΩ at −2.5 V. At about 2.5 V of gate drive (a low cell) expect up to 85 mΩ, which is about 0.17 V of drop at 2 A.
13. **I2C bus.** SHT31 (0x44/0x45) and DS3231 (0x68) do not conflict. Keep the total bus capacitance ≤ 400 pF (SHT3x) and decouple the SHT31 with 100 nF. CRC-8 uses polynomial 0x31, init 0xFF.
14. **DS3231.**
   - Check OSF (reg 0Fh bit 7) at boot and treat the time as invalid if it is set; it is set at first power-up. Clear it after an NTP/modem time sync.
   - The VCC→VBAT switchover is at 2.45–2.70 V, so a 3.3 V rail is fine.
   - The backup coin cell draws about 0.84 µA typical in timekeeping.
15. **Printer status.** `ESC v` paper-out status exists, but the two manuals return different formats: a bit-coded byte (CSN v4.0) or the string "P1V72T30" (A2). Firmware must detect which format the unit sends. DLE EOT is not documented. Confirm on the real unit.
16. **Unresolved printer facts.** Baud rate, TTL voltage tolerance and peak current are not in any document I could download. Measure them on the actual unit before freezing the design.
   - Until then, add a level shifter or divider on the ESP32-to-printer RX line.
   - Note that CN4 on the USB-board variant is labelled +3.3 V.

---

## Downloaded files (all in `/tmp/claude-0/-home-user-Seri-tool/a0e3ea76-446d-5600-8913-2d583f46d7e6/scratchpad/datasheets/`)

| File | Document | Source URL |
|---|---|---|
| esp32-wroom-32e.pdf | ESP32-WROOM-32E/32UE Datasheet v2.1 | https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32e_esp32-wroom-32ue_datasheet_en.pdf |
| esp32.pdf | ESP32 Series Datasheet v5.3 | https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf |
| esp32_devkitc_v4-sch.pdf | ESP32-DevKitC V4 schematic (Rev V4, 2017-12-06) | https://dl.espressif.com/dl/schematics/esp32_devkitc_v4-sch.pdf (dl.espressif.com is not on the allowed list, but the proxy allowed it) |
| (HTML, not PDF) | ESP32-DevKitC V4 Getting Started Guide | https://documentation.espressif.com/esp-dev-kits/en/latest/esp32/esp32-devkitc/user_guide.html |
| a7670e_lase.pdf | A7672X/A7670X Series Hardware Design V1.03 (2022-11-02) | https://datasheet.lcsc.com/datasheet/pdf/d54143a94b7b1c567909f195edbb42b2.pdf?productCode=C5362211 |
| a7670e_fase.pdf | A7670E product brief V 2021.03 | https://datasheet.lcsc.com/datasheet/pdf/97761416f7d1b9f155f7db25d6b58de1.pdf?productCode=C7435296 |
| a7670e_masc.pdf | A7670E(-MASC) product brief (not used) | https://datasheet.lcsc.com/datasheet/pdf/9ef59010dead2bec7b1790c4ccc94de7.pdf?productCode=C22357882 |
| sht3x.pdf | Datasheet SHT3x-DIS, Version 7, Dec 2022 | https://sensirion.com/media/documents/213E6A3B/63A5A569/Datasheet_SHT3x_DIS.pdf |
| ds3231.pdf | DS3231 19-5170 Rev 7 (3/10) | https://datasheet.lcsc.com/datasheet/pdf/5101f986439e4595b0e0d04ae2a48583.pdf?productCode=C9866 |
| adafruit_printer.pdf | CSN-A2 User Manual Version 4.0 | https://cdn-shop.adafruit.com/datasheets/CSN-A2%20User%20Manual.pdf |
| adafruit_printer2.pdf | A2 Micro panel thermal printer manual | https://cdn-shop.adafruit.com/datasheets/A2-user%20manual.pdf |
| tp4056_toppower_nanjing.pdf | NanJing Top Power TP4056 REV_2.4 | https://datasheet.lcsc.com/datasheet/pdf/d328ee101cbb1f3719611cd61db479c2.pdf?productCode=C16581 |
| tp4056_tpower.pdf | Shenzhen TPOWER TP4056 V1.3 (Chinese) | https://datasheet.lcsc.com/datasheet/pdf/59634a9576f5ef3c592c92550f50e945.pdf?productCode=C382139 |
| ap2112k.pdf | Diodes AP2112 DS39724 Rev 2-2 | https://datasheet.lcsc.com/datasheet/pdf/5b29baebdc4332b382a530ff21092301.pdf?productCode=C51118 |
| me6211.pdf | Microne ME6211 V14 | https://datasheet.lcsc.com/datasheet/pdf/d15331d096a848ac81a26b001a32d4e3.pdf?productCode=C82942 |
| tps61088.pdf | TI TPS61088 ZHCSDP8A (Chinese, Rev A) | https://datasheet.lcsc.com/datasheet/pdf/65ccb75806824687872eb7e1eedc92a8.pdf?productCode=C92483 |
| mt3608.pdf | Aerosemi MT3608 Rev1.1 | https://datasheet.lcsc.com/datasheet/pdf/87b648f6991783104e708a12c210cc63.pdf?productCode=C84817 |
| icr18650_2600_sparkfun.pdf | Great Power 18650 2600 mAh spec Ver A/1 | https://cdn.sparkfun.com/datasheets/Prototyping/ICR18650%202600mAH%20datasheet.pdf |
| ao3401a.pdf | AOS AO3401A Rev 3.1 (Dec 2023) | https://datasheet.lcsc.com/datasheet/pdf/fee353dd1e9e0bc90b295f14f381aa4c.pdf?productCode=C15127 |
| ss34_mdd.pdf | MDD SS32–SS3200 Rev:2024A5 | https://datasheet.lcsc.com/datasheet/pdf/dfa1ff67dea875d0135103ba9ada713a.pdf?productCode=C8678 |

Failed sources:
- **analog.com:** the HTTP/2 stream was reset mid-transfer.
- **ti.com:** every datasheet URL returned 404.
- **simcom.com:** downloads need a login.
- **docs.espressif.com:** the connection failed.
- **learn.adafruit.com and monolithicpower.com:** not allowed.
- **LCSC MP1584 product pages:** not found.

---

## Addendum

The coordinator allowed two more hosts: www.monolithicpower.com and learn.adafruit.com. Same rules as above: values were read only from the fetched source and quoted verbatim. "Section" is the page heading inside the Adafruit guide, which is HTML and has no page numbers.

### A1. MPS MP1584EN, still NOT FOUND

| Fact | Value | Source | Section/Page | Quote |
|---|---|---|---|---|
| Input range, absolute max input, max Iout, fsw range, efficiency at 12 V→5 V | **NOT FOUND** | www.monolithicpower.com | — | Every URL returned HTTP 200 with an HTML page titled "Client Challenge" (a JavaScript bot check) instead of the PDF. URLs tried: the documentview URL the coordinator suggested, `/pub/media/document/MP1584_r1.1.pdf`, and both product pages. WebFetch reports `getaddrinfo ENOTFOUND www.monolithicpower.com` because the WebFetch tool's network list does not include the host. I did not try to get around the challenge. |

### A2. Adafruit "Mini Thermal Receipt Printers" guide

Source: https://learn.adafruit.com/mini-thermal-receipt-printer?view=all, "published September 02, 2012, last edited January 18, 2025". Saved as HTML at `scratchpad/la.html`. The guide covers products #597 Mini, #2751 Tiny, #2752 Nano and #2753 Guts.

| Fact | Value | Source | Section | Quote |
|---|---|---|---|---|
| Supply voltage | 5–9 V DC. Never exceed 9 V; 12 V or more permanently damages the printer. | Adafruit guide | "Power" | "they require a lot of power: 5 to 9 Volts DC , at least 2 Amps of current." / "One should never exceed 9 Volts to the printer; 12V or more will permanently damage it!" |
| Peak current while printing | Supply requirement of **at least 2 A** "during high-current printing". The guide gives no measured peak figure. | Adafruit guide | "Overview", "Power" | "a 5 Volt to 9 Volt regulated DC power supply that can provide 2 Amps or more during high-current printing" |
| Brown-out advice | When the MCU shares a 5 V/2 A supply, power the MCU separately to avoid brown-out. A larger supply reduces droop. | Adafruit guide | "Power" | "you might want to power the microcontroller separately through its USB port, to prevent a brownout condition when the printer is drawing a lot of current." |
| How current depends on density/heating | No current-vs-heating figure. The printer "can only heat so many dots at a time". If dense images jam, lower the density passed to `printer.begin()`; the library default is 120. Higher current or voltage gives darker printing. Related: the CSN-A2 manual p48 says peak current rises with "max heating dots". | Adafruit guide | "Other Things to Know" (under Hacking!), "Power" | "If you try to print a “dense” image and the paper jams … pass a lower density value to printer.begin(). Default value is 120." / "The higher current can make for darker printing" |
| TTL logic level | Printer logic is 3.3 V and "5V tolerant", so no level shifting is needed in either direction. It is not RS-232 (10 V); never connect it to a PC serial port. | Adafruit guide | "Making Connections" → "TTL Serial" | "The printer logic is 3.3V, but “5V tolerant,” meaning no extra level-shifting circuitry is needed in either direction." |
| Default baud rate | 19200 or 9600, depending on the unit. The self-test page prints the actual value. Adafruit's Arduino library defaults to 19200. | Adafruit guide | "Print Test Page"; "Firmware Version and Baud Rate"; "Install Library" | "Note the baud rate on the test page. This may be 19200 or 9600." / "If no baud rate is shown … first assume 9600 as the default. In that doesn’t work in tests that follow, then try 19200." / "Some printers arrive from the factory set for 19200 baud, but a few may be set to 9600." |
| Paper status reply | `has_paper()` returns True/False. It only works on some printers and firmware versions, and needs the printer's TX wired to the MCU's RX. | Adafruit guide | "Check Paper" | "With some printers (not all, and it varies among firmware releases), if you've connected the RX pin of your board to the printer TX, you can query paper status with the has_paper function that returns True or False if there is paper present" |
| Flow control | Hardware handshaking (DTR) avoids buffer overflow but "is not a panacea". | Adafruit guide | "Hacking!" | "Hardware handshaking is a means by which a printer or other device can report to the microcontroller that it’s ready to receive more data" |
| Firmware dependency | The CircuitPython class must match the firmware version printed on the test page (example: 2.69). | Adafruit guide | "Create ThermalPrinter Class" | "In this example we're specifying a printer with version 2.69 firmware, but be sure to specify your printer's version!" |
| Cashino A2 sheet (linked from the guide's Downloads section) | Power supply DC 5–9 V; 384 dots/line; Serial (RS-232, TTL). No current, baud or logic-level data. | `datasheets/cashino_a2.pdf` (single page, title "printer.xls") from https://cdn-shop.adafruit.com/datasheets/cashino%20thermal%20printer%20a2.pdf | p1 | "Power Supply (Adapter) DC5V-9V" |

### A3. Still NOT FOUND after the addendum
- **MP1584EN:** all requested values, because of the bot challenge on monolithicpower.com.
- **Printer measured peak current:** only a "≥ 2 A supply" requirement is given.
- **DLE EOT real-time status:** not mentioned in the guide or in the manuals.
- **A7670C bands and A7670 sleep current:** unchanged; not in scope for this addendum.

### A4. Design implications revised by the addendum
- **Printer logic level is resolved.** The printer is 3.3 V logic and 5 V tolerant, so it can connect directly to the ESP32's 3.3 V UART. The earlier recommendation of a level shifter on the printer line is withdrawn.
- **Baud rate must be detected.** Firmware should try 9600 and 19200, or read the rate from the self-test page.
- **Size the 5 V printer rail for at least 2 A, and keep the ESP32/modem rails separate** so printing does not brown them out. The TPS61088 qualifies; the MT3608 (about 2.2 A estimated at a 3.0 V cell) has no margin. Limit density and heating dots in firmware, and do not print during modem TX bursts.
- **Paper status is unreliable across units.** `has_paper` support varies by firmware, so wire the printer's TX to an ESP32 RX pin (GPIO34–39 are fine; they are input-only) and treat paper-status support as optional.
