# Bench and field test plan (what software could not prove)

Everything below was **assumed or out of reach** in phases 0-6 (and the HiveMQ connection, D-061). Each item gives the reason, the method and the pass level.
Items marked ⚠ gate the pilot order or the field trial. Record results per board in the table at the end.

## Power and electrical

| ID | Item | Why it matters | Method | Pass |
|---|---|---|---|---|
| B-01 ⚠ | Battery path resistance per cell (holder + wiring) | Worst-case modem margin is only 3 mV (SPICE, D-050) | 2 A DC through holder → C9 node, 4-wire voltage drop | ≤ 20 mΩ |
| B-02 ⚠ | Printer peak current, low-power mode (ESC 7 n1=3) | SPICE envelope and protection trip assume ≤ 1.0 A at 5 V on battery | 0.1 Ω shunt in +5V_PRN, scope, print a dense line | ≤ 1.0 A peak |
| B-03 | Modem VBAT dip during GSM bursts at 3.60 V cells | Confirms the SPICE result (3.403 V) | Scope at J10-1, cells at 3.60 V, force 2G, send SMS | ≥ 3.40 V |
| B-04 | No reset on mains fail / return | R-02 | 100 cycles of 12 V on/off, boot counter in the log | 0 resets |
| B-05 | A7670C idle current, registered (datasheet: "TBD") | Budget assumes 25 mA | DC current into VBAT, idle 10 min | Re-run `hardware/power_budget`; runtime still ≥ 24 h |
| B-06 | DevKitC extras on 3V3 (CP2102, LED) | Budget assumes 6 mA | Current into ESP_3V3 with the ESP32 in reset | As B-05 |
| B-07 ⚠ | Runtime on 2 cells | P-02 target 24 h; budget says 36 h | Full charge, mains off, normal operation with 4G uploads | ≥ 24 h with SMS still working |
| B-08 | TP4056 temperature in the closed box | No thermal vias under the pad (hand solder, D-048) | Thermocouple on U2, 500 mA charge, box closed, 35 °C ambient | < 85 °C case |
| B-09 | Printer start-up time after the boost turns on | Firmware waits 500 ms (assumed) | Battery mode, print after idle | First line printed correctly |
| B-10 | Printer baud (9600 / 19200 auto-detect) and paper-out via ESC v | D-041, D-047 | Boot log shows detected baud; remove paper | Paper-out alert on OLED, red LED, SMS |
| B-11 | Surge and ESD | Rural mains, long probe cable | ±4 kV contact ESD on probe and J1 (IEC 61000-4-2 style); adapter plugged/unplugged 50x | No reset, no damage |
| B-12 | EMI: modem bursts vs sensor and printer | Layout effects not in SPICE | 24 h log with frequent SMS | 0 sensor read errors, no print corruption |

## Radio and network

| ID | Item | Method | Pass |
|---|---|---|---|
| B-13 ⚠ | SMS delivery on Airtel, Vi and Jio (Jio is LTE-only: needs IMS SMS) | 20 SMS each in English + one Indian script | All delivered; note Jio result |
| B-14 | LTE bands in the pilot districts (A7670C band list not verified from a datasheet) | Register in each pilot village, log band/RSSI | Registered, RSSI recorded |
| B-15 | Unicode SMS rendering on 3 common feature phones | Send Telugu/Kannada/Hindi alerts | Readable (no boxes), or note models that fail |
| B-16 | Wi-Fi range through shed walls | RSSI at the mounting spot | Upload works, or 4G only |
| B-28 ⚠ | Upload to the real HiveMQ Cloud cluster over 4G (A7670C TLS 1.2 + SNI + ISRG Root X1) and over Wi-Fi | Provision one unit with tools/make_provision.py, watch the topic in the HiveMQ web client for 24 h | Records arrive on `seri/<client_id>/data`; no reconnect storm; if the modem rejects SNI or the certificate, note the firmware version (AT+SIMCOMATI) |

## Sensor and clock

| ID | Item | Method | Pass |
|---|---|---|---|
| B-17 ⚠ | 2 m extended I2C probe at 50 kHz with 2.2k pull-ups (D-052) | 24 h logging next to the modem | 0 bus errors |
| B-18 | SHT31 accuracy at shed humidity | Beside a calibrated reference at ~75 % and ~90 % RH for 2 weeks | Within ±0.5 °C and ±3 % RH |
| B-19 | DS3231 drift | 30 days without network time correction | ≤ 1 min/month |

## Mechanical

| ID | Item | Method | Pass |
|---|---|---|---|
| B-20 ⚠ | Keystone 1043 pin fit (drawing gives 1.57 mm pin, 1.19 mm hole; footprint uses 1.7 mm; pin row on the centre line) | Fit a real holder on a bare board | Pins and posts fit; adjust `hardware/kicad/lib/seri.pretty` if not |
| B-21 ⚠ | Real module sizes vs enclosure assumptions (modem breakout 45x45x16, buck 51.5x26.5x14, boost 40x30x12, OLED holes 31.4x29.5, jack axis 6.5 mm) | Measure, update `mechanical/cadquery/enclosure.py`, re-run `tools/phase6_gate.sh` | Gate passes with measured values |
| B-22 | Print fit: printer cut-out, plunger travel, light pipes, lid tongue, shrinkage | Print one set, assemble | Everything fits without force |
| B-23 | Splash resistance (no IP rating claimed) | Spray from 4 directions for 5 min | No water inside |

## People and words

| ID | Item | Method | Pass |
|---|---|---|---|
| B-24 ⚠ | Native-speaker review of every Hindi, Telugu and Kannada text (SMS, OLED, printer, farmer card) | One reviewer per language, using `firmware/lib/core/src/sms_text.cpp`, `ui.cpp`, `docs/handoff/farmer-card.pdf` | Corrections applied, tests re-run |
| B-25 | Farmer usability without English | Field trial: farmer sets language, stage, phone number using buttons and the card | Done without help |

## Before ordering

| ID | Item | Pass |
|---|---|---|
| B-26 ⚠ | Prices and stock at order time (`bom/bom.md`); a 2 A 5 V boost module and a VBAT-input A7670C breakout in stock | Phase 5 gate passes |
| B-27 | Wokwi scenarios re-run after the phase 4-6 firmware changes (workflow "Wokwi scenarios", manual) | 8/8 pass |

## Record sheet

| Board # | B-01 (mΩ) | B-02 (A) | B-03 (V) | B-05 (mA) | B-07 (h) | B-08 (°C) | B-13 Airtel/Vi/Jio | B-17 errors | Notes |
|---|---|---|---|---|---|---|---|---|---|
| 1 | | | | | | | | | |
| 2 | | | | | | | | | |
