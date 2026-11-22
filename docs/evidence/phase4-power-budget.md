# Battery-life budget

**Status: FINAL**

| Load | Current (mA) | Duty | Average (mA) | Source |
|---|---|---|---|---|
| esp32_modem_sleep | 31 | 1.0000 | 31.00 | ESP32 datasheet v5.3 Table 4-2 p30: modem-sleep 80 MHz dual-core 20-31 mA (max used) |
| esp32_wifi_upload | 0 | 0.0000 | 0.00 | Not used on battery in the worst case (uploads go over 4G). On Wi-Fi: 239 mA avg TX, WROOM p29 |
| devkitc_extras | 6 | 1.0000 | 6.00 | ASSUMED: DevKitC power LED + CP2102 with USB unplugged; no datasheet value in hand |
| ap2112k_quiescent | 0.08 | 1.0000 | 0.08 | AP2112K DS39724 p8: Iq 55 typ / 80 max uA |
| modem_idle_registered | 25 | 1.0000 | 25.00 | ASSUMED: A7670 HD V1.03 Table 46 p59 lists idle/sleep current as TBD |
| modem_upload_4g | 600 | 0.0067 | 4.00 | A7670 HD p60: LTE TX 600 mA typ (B1/B3 23 dBm). 6 s of TX per upload every 900 s (attach+TLS+publish, ASSUMED duration) |
| modem_sms | 630 | 0.0004 | 0.26 | A7670 HD p59: GPRS 630 mA typ. 3 s per SMS x 12 SMS/day = 36 s / 86400 s |
| sht31 | 0.6 | 0.0005 | 0.00 | SHT3x Ver.7 Table 3 p6: 600 uA measuring, 12.5 ms (p7) every 30 s (idle 0.2 uA ignored) |
| ds3231 | 0.11 | 1.0000 | 0.11 | DS3231 Rev 7 p2: standby 110 uA max at 3.63 V |
| oled_on | 20 | 0.0069 | 0.14 | ASSUMED 20 mA for a 1.3 inch SH1106 at mid brightness; on 10 min/day on battery |
| microsd_idle | 1.5 | 1.0000 | 1.50 | ASSUMED: card standby, card-dependent |
| microsd_write | 60 | 0.0017 | 0.10 | ASSUMED: 50 ms write every 30 s |
| printer_low_power | 1100 | 0.0003 | 0.31 | ASSUMED: peak current not in the printer manual (CSN v4.0 p48). 24 heating dots: ~0.75 A at 5 V, / 0.88 boost efficiency, / 3.7 V = 1.15 A from the cell; 2 s per line x 12 lines/day |
| boost_shutdown | 0.003 | 1.0000 | 0.00 | TPS61088 ZHCSDP8A p5: ISD 1 typ / 3 max uA (EN low between prints, D-046) |
| battery_sense_divider | 0.021 | 1.0000 | 0.02 | Design: 100k + 100k divider across 4.2 V |
| pfet_gate_pulldown | 0.042 | 1.0000 | 0.04 | Design: 100k gate pull-down on the load-sharing P-FET at 4.2 V |
| leds_and_buzzer | 2 | 0.1000 | 0.20 | Design: status LED 2 mA (via 680 ohm from 3.3 V) blinking 10 %; buzzer only during alarms (ignored) |
| tp4056_battery_drain | 0.005 | 1.0000 | 0.01 | ASSUMED: charger reverse drain with no input; not in the NJ/SZ datasheet tables I read |
| charger (mains only) | 1000 | 1.0000 | 1000.00 | TP4056 NJ REV_2.4 p9: RPROG 1.1k -> 1000 mA (mains only) |

Average battery current on battery: **68.8 mA**

| Pack | Capacity | Usable | Runtime |
|---|---|---|---|
| 2 x 18650 (fitted) | 5000 mAh | 50% | **36 h (1.5 days)** |
| 4 x 18650 (footprint) | 10000 mAh | 50% | **73 h (3.0 days)** |

Monitoring, logging and the screen keep running after the modem stops at 3.60 V, down to 3.30 V (ESP32 VDD >= 3.0 V with 200 mV LDO dropout, AP2112K p8): 72% usable (ASSUMED 90 % of capacity above 3.3 V at light load, x 0.8 end of life). Extra monitoring-only time: 2 cells +16 h, 4 cells +32 h

## Assumed values (no datasheet number): bench-measure these

Sensitivity: runtime on the fitted 2-cell pack if each assumed current is double.

| Load | Assumed (mA) | If doubled: 2-cell runtime |
|---|---|---|
| devkitc_extras | 6 | 33 h |
| modem_idle_registered | 25 | 27 h |
| oled_on | 20 | 36 h |
| microsd_idle | 1.5 | 36 h |
| microsd_write | 60 | 36 h |
| printer_low_power | 1100 | 36 h |
| tp4056_battery_drain | 0.005 | 36 h |
| **all assumed values doubled at once** | | **25 h** |

Target (P-02 / Q-8: >= 24 h on the fitted 2-cell pack): >= 24 h on 5000 mAh. Result 36 h -> **PASS**
