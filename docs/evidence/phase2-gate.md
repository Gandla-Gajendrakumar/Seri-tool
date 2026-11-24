# Phase 2 gate evidence

**Run:** GitHub Actions `firmware` run #11 (https://github.com/Gandla-Gajendrakumar/Seri-tool/actions/runs/44979854486),
commit `d67f7bf`, 2026-11-22. Both jobs green. Full logs are in the run's `wokwi-logs` artifact.

## Job 1: logic tests, gate, custom chips
- Native unit tests: 125/125 pass (core logic, Monitor on fake hardware, UI, glyph atlas widths).
- Phase 1 gate: core has no hardware headers; ESP32 build passes.
- Custom chip host tests: 59/59 pass. All 5 chips compile to WebAssembly.

## Job 2: Wokwi scenarios (real ESP32 firmware on the virtual board, clock x60)

| Scenario | Result | Simulated run time |
|---|---|---|
| cold_night | PASS (10 rules) | ~25 s |
| heat_spike | PASS (11 rules) | 131 s |
| network_loss | PASS (9 rules) | 137 s |
| normal_day | PASS (9 rules) | 49 s |
| paper_out | PASS (6 rules) | 48 s |
| power_cut | PASS (9 rules) | 28 s |
| sd_removed | PASS (6 rules) | 37 s |
| sensor_unplugged | PASS (7 rules) | 20 s |

`ALL SCENARIOS PASSED`

## What the farmer received and what was printed (copied from the run log)

Heat spike:
```
PRINTER| 21/11 05:57 TEMP HIGH 31.0C
SMS to +919876543210: Shed-1: Temp HIGH 31.0C (safe 26-29C) Chawki 05:57
expect-pin led_r:A == 1                      (red LED on)
SMS to +919876543210: Shed-1: STILL Temp HIGH 31.0C for 31min 06:29
PRINTER| 21/11 06:31 TEMP OK 27.0C
SMS to +919876543210: Shed-1: Temp normal again 27.0C 06:31
expect-pin led_r:A == 0                      (red LED off)
```
Network loss (alert held while there was no network, then sent once):
```
PRINTER| 21/11 05:57 TEMP HIGH 31.0C         (printed at once, no network needed)
SMS to +919876543210: Shed-1: Temp HIGH 31.0C (safe 26-29C) Chawki 05:57   (after network returned)
SMS to +919876543210: Shed-1: STILL Temp HIGH 31.0C for 30min 06:28
```
Power cut:
```
PRINTER| 21/11 05:54 POWER CUT
SMS to +919876543210: Shed-1: Power cut, on battery 65% 05:54
PRINTER| 21/11 05:56 BATTERY LOW 3.29V
SMS to +919876543210: Shed-1: Battery low 2% 05:56
PRINTER| 21/11 05:58 POWER BACK
SMS to +919876543210: Shed-1: Power back 05:58
```
Normal day (SMS setup by the farmer, daily summary, stage change, status request):
```
SMS to +919876543210: Shed-1: OK                       (owner registered with "10 000000")
PRINTER| ======= DAILY SUMMARY =======  ...  Alerts 0
PRINTER| 21/11 06:00 STAGE -> Instar 4
SMS to +919876543210: Shed-1: OK - Instar 4            (reply to "11 4")
SMS to +919876543210: Shed-1: 25.0C 70% Instar 4 06:20 (reply to "20")
```
SD card removed, sensor unplugged, paper out:
```
PRINTER| 21/11 05:53 SD CARD FAULT / SMS: Memory card fault / alerts still sent / PRINTER| SD CARD OK
PRINTER| 21/11 05:54 SENSOR FAULT  / SMS: Sensor fault, check probe cable / SMS: Sensor OK again
SMS: Paper out, refill printer / lines held, then printed once paper loaded:
  PRINTER| 21/11 05:52 PAPER OUT / 05:58 TEMP HIGH 31.0C / 05:58 PAPER OK
```

## Bugs found by the virtual-hardware runs (all fixed before run 11)
- Scenario runner and chip-control naming errors (runs 6-7).
- Expectation files assumed numbers without +91; trace lines split by chip output (run 7).
- Red-LED check ran before the LED update in the same loop pass (run 7).
- Wokwi's ESP32 ADC: no eFuse calibration emulated, and 0-4095 spans 0-5 V. The battery read 24 %, then 34 %, low until the simulation build converted raw counts (runs 7-10, decision D-032).

## Not proven by these runs (bench test plan, phase 7)
Real A7670C AT responses and SMS on Indian networks; Wi-Fi upload and OTA download; an SD card that fails while
inserted (native tests only); real ESP32 battery-ADC accuracy (needs a one-point trim); OLED legibility of
Indian scripts; native-speaker review of all wording.
