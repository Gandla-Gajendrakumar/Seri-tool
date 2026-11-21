# Seri-tool — Requirements (v0.1, Phase 0)

Status: **DRAFT, for product-owner review.** Derived from the project brief of 2026-11-21.
Items marked **[Q-n]** depend on an open question in `docs/questions.md`.
Items marked **[ASSUMED]** are my proposed default. I will apply it unless you say otherwise.

Each requirement has an ID so tests, simulations and the bench test plan can point to it.
The "Verified by" column shows the phase whose automated check proves it. **BENCH** means
simulation cannot prove it, and it goes into the phase 7 bench test plan.

---

## 1. Purpose and users

| | |
|---|---|
| Product | Low-cost climate monitor for silkworm rearing sheds in India. Crop use comes later. |
| Primary user | Sericulture farmer. May not read English. May not own a smartphone. Uses a basic feature phone for SMS. |
| Secondary user | Installer / dealer / extension officer who sets the device up once. |
| Environment | Rearing shed. 15–40 °C, 40–98 % RH, dust, disinfectant fumes (bleaching powder, formalin / chlorine-dioxide sprays) [ASSUMED]. Unreliable mains, patchy cellular coverage. |

## 2. Fixed hardware decisions (from brief, not reopened)

| ID | Decision |
|---|---|
| HW-01 | V1 is a custom carrier PCB with plug-in modules. A fully integrated PCB comes later. Firmware sits behind a hardware abstraction layer (HAL) so it ports to the later board unchanged. |
| HW-02 | Controller: ESP32-DevKitC with WROOM-32E. Wi-Fi is the first choice for upload; cellular is the fallback. |
| HW-03 | Cellular: SIMCom A7670C breakout (LTE Cat-1 with GSM fallback), for SMS and data. |
| HW-04 | Sensor: SHT31 probe on a 2 m cable. Optional DS18B20 header. |
| HW-05 | Clock: DS3231 with coin cell, corrected from network time. |
| HW-06 | Storage: microSD, holding the log plus the store-and-forward queue. |
| HW-07 | Printer: 58 mm TTL thermal panel printer, ESC/POS. |
| HW-08 | Local UI: 1.3" OLED, 3 buttons, buzzer, 2 LEDs. |
| HW-09 | Power: 12 V 3 A adapter. 2× 18650 in parallel fitted (~5,000 mAh), footprint for 4 (~10,000 mAh). Charger with power-path. Modem on the battery rail. 5 V boost for the printer. Mains-fail and battery-voltage sensing. |
| HW-10 | Relay header for future control. **Not driven in V1.** |
| HW-11 | Enclosure: 3D-printed PETG or ASA, wall-mounted, separate ventilated probe housing, modelled in CadQuery. |

## 3. Functional requirements

### 3.1 Measurement

| ID | Requirement | Verified by |
|---|---|---|
| F-MEAS-01 | Take a raw SHT31 reading every 6 s. Every 30 s, report the **median of the last 5** as one sample, for temperature and RH separately. [ASSUMED: "sample every 30 s, median of 5" means 5 raw reads spread across each 30 s window, not 5 reads in a burst.] | P1 unit, P2 sim |
| F-MEAS-02 | Each sample carries a date and time from the DS3231 in IST (UTC+5:30, no daylight saving). | P2 |
| F-MEAS-03 | A raw read that fails CRC, times out or is out of physical range (T < −40 or > 125 °C, RH < 0 or > 100 %) is discarded. The median uses only valid reads. If fewer than 3 of the 5 are valid, the sample is marked INVALID. | P1 |
| F-MEAS-04 | 3 consecutive INVALID samples (90 s) raise a **sensor fault** event. 3 consecutive valid samples clear it. | P1, P2 (sensor unplugged) |
| F-MEAS-05 | With the DS18B20 fitted: read and log its temperature and show it on the OLED. It is never used for alerts in V1. [ASSUMED] | P2 |
| F-MEAS-06 | Display resolution 0.1 °C and 1 % RH. | P1 |

### 3.2 Stage profiles

| ID | Requirement | Verified by |
|---|---|---|
| F-PROF-01 | All stage profiles live in **one config file** (`firmware/lib/core/profiles` + a human-readable source table). Each entry records its source document. | P1 |
| F-PROF-02 | Initial values from the brief, **to be confirmed against Central Silk Board and Chinese sericulture sources in phase 1. Any change is shown to you before it is applied.** | P1 |

| Stage | Temp target (°C) | RH target (%) |
|---|---|---|
| Incubation | 25–26 | 80 |
| Chawki, instars I–II | 27–28 | 80–90 |
| Instar III | 26 | 80 |
| Instar IV | 25 | 70 |
| Instar V | 24 | 70 |

| ID | Requirement | Verified by |
|---|---|---|
| F-PROF-03 | Default tolerance ±1 °C and ±5 % RH, adjustable per device. **Safe band = [target_min − tol, target_max + tol]**. Example: chawki → 26.0–29.0 °C, 75–95 % RH. [Q-3] | P1 |
| F-PROF-04 | The farmer selects the current stage on the device (buttons) or by SMS. The selection survives power loss. | P1, P2 |
| F-PROF-05 | Stage changes are never automatic in V1. [ASSUMED; see Q-9] | — |

### 3.3 Events (alarm logic)

| ID | Requirement | Verified by |
|---|---|---|
| F-EVT-01 | A **climate event** (separate for temp-high, temp-low, RH-high, RH-low) starts when **every** valid sample over **5 continuous minutes** is outside the safe band in that direction. One in-band sample restarts the 5-minute timer. | P1 |
| F-EVT-02 | A temperature event clears when the reading is back inside the band **by at least 0.5 °C** (hysteresis). RH hysteresis: **2 % RH** [ASSUMED, not in brief]. Clearing needs the reading to stay inside for 2 consecutive samples (60 s) [ASSUMED], so one noisy sample cannot cause a clear-then-re-alarm. | P1 |
| F-EVT-03 | Event start → buzzer pattern, one printed line, SD log record, SMS. | P1 (decisions), P2 (effects) |
| F-EVT-04 | While an event is active: repeat SMS **every 30 minutes**, subject to the daily cap (F-SMS-05). | P1, P3 |
| F-EVT-05 | Event clear → exactly **one recovery SMS**, printed line and log record. Sent even if that day's cap was reached, so the farmer is never left thinking the problem continues. [ASSUMED] | P1, P3 |
| F-EVT-06 | System events, each with its own start/clear logic: **mains fail** (after 2 min continuous outage [ASSUMED, to stop flicker floods]), **mains return** (after 2 min stable), **low battery** (≤ 3.40 V under load [ASSUMED; checked against the cell datasheet in phase 4]; clears at ≥ 3.70 V), **sensor fault** (F-MEAS-04), **paper out** (printer status; see R-04), **SD fault**, **clock invalid**. | P1, P2 |
| F-EVT-07 | Changing stage re-evaluates the band at once. Active events whose condition no longer holds clear **silently** (log + print, no recovery SMS), so a stage change cannot itself trigger texts. [ASSUMED] | P1 |
| F-EVT-08 | The buzzer can be silenced for the current event with one button press. It sounds again only for a new event. | P2 |

### 3.4 SMS

| ID | Requirement | Verified by |
|---|---|---|
| F-SMS-01 | SMS goes directly through the A7670C. **No cloud, no internet, no app** is needed for alerts. | P2 |
| F-SMS-02 | Languages: Telugu, Kannada, Hindi, English. Selectable per registered number [ASSUMED]. Indian scripts need Unicode (UCS-2) SMS, which holds **70 characters per segment** (160 for English). Every alert template must fit in **one segment** in every language, so a message never splits or arrives out of order. A unit test enforces this. | P1 |
| F-SMS-03 | Alert content: what (e.g. "temperature HIGH"), the reading, the safe band, the stage, time since it started, device name. All numbers use Western digits (0–9), which every phone renders. | P1 |
| F-SMS-04 | Up to **3 registered numbers** receive alerts. [Q-1] | P1 |
| F-SMS-05 | **Flood control.** Every **first** alert of an event is always sent. Repeat SMS count against a per-number daily cap (default **6 repeats/day** [Q-2]) that resets at midnight IST. At the cap, the device sends one final "daily limit reached, device keeps monitoring and printing" SMS. Mains flicker is debounced (F-EVT-06). An event that re-occurs within 10 minutes of clearing counts as the same event (no new "first" SMS) [ASSUMED]. | P1, P3 |
| F-SMS-06 | If a send fails, retry with back-off (1, 2, 4 min, …) up to 1 hour. Then log it and print "SMS NOT SENT". Never block measurement. | P2 (network loss) |
| F-SMS-07 | Configuration commands by SMS are accepted **only from registered numbers**, and the device replies to confirm each one. Commands use **digits only** (e.g. `11 3` = set stage to instar III) so no English is needed. [Q-6] | P1, P2 |

### 3.5 Printer

| ID | Requirement | Verified by |
|---|---|---|
| F-PRN-01 | Every event start, every recovery and every system event prints **one line**: time, event, reading. | P1, P2 |
| F-PRN-02 | Daily summary at a fixed time (default 06:00 [ASSUMED]): min / max temperature, min / max RH, total time out of band, event count. | P1, P3 |
| F-PRN-03 | **On battery**, print event lines only. Skip the daily summary and print it after mains returns, marked "late". | P1 |
| F-PRN-04 | Printed language: [Q-4]. Cheap ESC/POS printers have **no fonts for Indian scripts**, so local-language text must be sent as a pre-rendered bitmap image. | P1 |
| F-PRN-05 | Paper out → paper-out event. Lines that could not be printed are queued (up to 200) and printed when paper is loaded. They remain on the SD card regardless. | P2 |

### 3.6 Data upload

| ID | Requirement | Verified by |
|---|---|---|
| F-UP-01 | Every **5 minutes**, publish all samples since the last upload (10 × 30 s samples) plus device status by **MQTT over TLS**. | P2 |
| F-UP-02 | Route: Wi-Fi if associated, otherwise cellular data. | P2 (network loss) |
| F-UP-03 | If both fail, queue on SD and send oldest-first when the link returns. The queue holds at least **30 days** of data. Upload status shows on the OLED. | P2 |
| F-UP-04 | MQTT broker: [Q-5]. The device stores broker host, port, CA certificate and per-device credentials. | — |
| F-UP-05 | Upload never delays sampling or alerts (runs as a separate task). | P2 |

### 3.7 Local UI

| ID | Requirement | Verified by |
|---|---|---|
| F-UI-01 | OLED home screen: temperature, RH, stage, band status (OK / HIGH / LOW), power (mains/battery %), link (Wi-Fi / 4G / none), time. | P2 |
| F-UI-02 | Every setup step can be done with the 3 buttons in the farmer's language, without English, an app or a smartphone. Text is drawn from pre-rendered Indian-script bitmaps. [Q-4] | P2 (screens), BENCH (usability) |
| F-UI-03 | Button-set items: language, stage, tolerance, registered phone numbers (digit entry), date/time (if no network), buzzer on/off, print test. | P2 |
| F-UI-04 | LEDs: green = monitoring OK; red = any active event (blinks if SMS undeliverable). | P2 |
| F-UI-05 | Wi-Fi credentials: [Q-7]. | — |

### 3.8 Clock

| ID | Requirement | Verified by |
|---|---|---|
| F-CLK-01 | The DS3231 is the time source. At boot and every 24 h, correct it from network time: NTP over Wi-Fi, or the cellular network clock. Correct only if the difference is > 2 s. Log every correction. | P2 |
| F-CLK-02 | If the RTC has lost time (oscillator-stop flag) and no network time is available, raise a "clock invalid" event. Monitoring continues with relative time. | P2 |

### 3.9 Firmware update

| ID | Requirement | Verified by |
|---|---|---|
| F-OTA-01 | Over-the-air update over Wi-Fi only, using two app partitions with automatic rollback if the new image does not confirm itself healthy within 5 minutes. | P2 (partition/rollback logic), BENCH |
| F-OTA-02 | Images are signed. Unsigned or tampered images are rejected. [ASSUMED] | P2 |
| F-OTA-03 | No update while running on battery or below 50 % charge. | P1 |

## 4. Reliability ("must not happen")

| ID | Requirement | Verified by |
|---|---|---|
| R-01 | **No alert floods** (F-SMS-05). Hard limit tested across a full rearing cycle: never more than (number of distinct event starts + cap + recoveries) SMS per number per day. | P3 |
| R-02 | **Power cut** does not stop monitoring: automatic switch-over to battery with no reset (power-path). Brown-out reset and hardware watchdog restart the firmware within 10 s if it ever hangs. | P2, P4 (SPICE), BENCH |
| R-03 | **Network loss** does not stop monitoring, printing or local alarms. SMS retries (F-SMS-06), data queues (F-UP-03). | P2 |
| R-04 | **SD failure or removal** does not stop monitoring, printing, SMS or upload. An SD-fault event is raised, and a RAM ring buffer (≥ 2 h) holds data until the card returns. | P2 |
| R-05 | **Sensor failure** is never silent (F-MEAS-04). | P2 |
| R-06 | **Printer failure** (paper out, disconnected) is never silent and never blocks anything else. | P2 |
| R-07 | Every fault is shown on the OLED + red LED **and** sent by SMS. Faults are never only logged. | P2 |
| R-08 | Settings (stage, numbers, language, counters) live in NVS flash, and SMS cap counters survive a reboot, so a reboot loop cannot cause an SMS flood. | P1, P2 |
| R-09 | Watchdog: each task checks in. A missing check-in → controlled restart, logged, and reported after boot. | P2 |

## 5. Power

| ID | Requirement | Verified by |
|---|---|---|
| P-01 | Runs from the 12 V adapter. Battery charges while running. | P4 |
| P-02 | Battery runtime with the 2-cell pack: target **≥ 24 h** of full monitoring + SMS + hourly uploads with no printing except events [Q-8]. The power-budget script reports runtime for 5,000 and 10,000 mAh. | P4 |
| P-03 | The rails stay within each module's datasheet limits during modem transmit bursts (A7670 GSM bursts reach about 2 A) and thermal-printer bursts at the same time. Shown by ngspice. | P4 |
| P-04 | Mains-fail detection independent of the battery voltage. Battery voltage measured by ADC with a divider. | P4 |
| P-05 | On battery: upload interval rises to 15 min [ASSUMED] and printing is limited (F-PRN-03) to extend runtime. Alerts are never reduced. | P1, P4 |

## 6. Firmware architecture

| ID | Requirement |
|---|---|
| A-01 | `lib/core`: pure C++17, no Arduino or ESP-IDF headers (profiles, event state machine, SMS/print formatting, flood control, summaries). Compiles and is unit-tested on the PC. |
| A-02 | `lib/hal`: abstract interfaces (Sensor, Clock, Modem, Printer, Storage, Display, Buttons, Power, Net). An ESP32 implementation now; the integrated board later. Core logic never calls hardware directly. |
| A-03 | Time is injected (a clock interface), so phase 3 can replay a 30-day cycle in minutes. |
| A-04 | Tests are written before the code they test. |

## 7. Enclosure

| ID | Requirement | Verified by |
|---|---|---|
| M-01 | Wall-mounted case + lid with cut-outs for OLED, buttons, LEDs, printer paper exit, DC jack, SD slot access, cable glands. | P6 |
| M-02 | Separate ventilated probe housing (radiation/drip shield) on the 2 m cable. | P6 |
| M-03 | Zero interferences between modelled parts. ≥ 1 mm clearance to every PCB component. Printable without supports where practical. | P6 |
| M-04 | Battery bay serviceable without desoldering. Battery isolated from the printer's heat. | P6 |
| M-05 | Splash-resistant, IP54 target [ASSUMED]. | BENCH |

## 8. What simulation cannot prove (goes to the phase 7 bench test plan)

1. **Real cellular behaviour.** Signal strength in the shed, operator registration, and **SMS delivery on each Indian operator**: Jio is LTE-only, so it has no GSM fallback and SMS depends on IMS support in the modem firmware. Airtel and Vi have 2G. LTE band support of the A7670C for Indian bands B1/B3/B5/B8/B40 gets checked on paper in phase 4 and proven only on the bench.
2. **Sensor accuracy and drift.** SHT31 behaviour at 85–95 % RH for weeks, condensation, and exposure to disinfectant fumes. Simulation trusts the sensor.
3. **Print quality.** Darkness, bitmap legibility of Indian scripts, paper-out detection on the actual printer model (many cheap printers have no status return line).
4. **Charging and battery.** Real cell capacity and ageing, charger heat inside a closed case, behaviour at 40 °C.
5. **Surge, ESD and mains quality.** Lightning-induced surges and adapter quality. Only a lab or field test shows these.
6. **Wi-Fi range** through shed walls.
7. **Enclosure.** Real print shrinkage and fit, PETG creep near warm parts, IP rating.
8. **Farmer usability.** Whether a non-English-reading farmer can actually complete setup. Needs a field trial.
9. **Real EMI.** Whether modem bursts disturb the sensor or printer through the PCB layout. SPICE models only the ideal circuit.

## 9. Out of scope for V1

Control outputs (relay header only), a smartphone app, a cloud dashboard (data is published; no visualisation is built unless you ask), voice calls, crop profiles, BIS/WPC certification work [Q-10].
