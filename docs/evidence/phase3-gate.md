# Phase 3 gate evidence: whole rearing cycle replay

**Gate: PASSED.** Run `tools/phase3_gate.sh` (about 15 s); raw output in `docs/evidence/phase3-gate.txt`.

## What was tested
- **Data:** a SYNTHETIC 38-day rearing cycle (2026-12-01 to 2027-01-08), 30-second resolution, 109,440 rows,
  generated deterministically by `sim/replay/gen_cycle.py`. No real shed logs exist yet (Q-12), so this is
  modelled data. The incidents scripted into it (I1-I13) are listed at the top of that file.
- **Device logic:** the real `lib/core` Monitor (the same code the ESP32 runs), on fake hardware, one tick per
  device second, with the farmer's SMS commands (owner registration, a Telugu helper number, every stage change).
- **Expected file:** `sim/replay/expected.txt`, written by a **separate Python oracle** (`sim/replay/oracle.py`)
  from the written rules only, not from the C++ code.
- **Why not Wokwi:** the free Wokwi plan stops a simulation after 5 minutes (about 5 hours of shed time at x60), so a
  38-day cycle cannot run there. The firmware drivers were proven in Wokwi in phase 2; this replay proves the
  logic over a full cycle.

## Results
```
Replay: real Monitor on fake hardware
  replay: 58 events, 168 SMS decisions, 167 SMS sent, 469 printed lines, 38 summaries
  actual == expected: 58 events, 168 SMS decisions, line for line
== 4. Caps and invariants
  worst day per number: 15 SMS (cap 20), 6 repeats (cap 6)
  told episodes per number: 38, recovery texts: 38
  SMS jobs 177 = sent 167 + reported not sent 10
  monitoring gap max 30 s; summaries 38 (late 2)
CHECKS: PASSED

GATE: PASSED
```

## SMS per day for the owner (the helper number received the same)

| Date | Alarms started | First alerts | Repeats | Recoveries | Limit texts | **Total** (cap 20) | Lines printed |
|---|---|---|---|---|---|---|---|
| 2026-12-03 | TempHigh | 1 | 3 | 1 | 0 | **5** | 12 |
| 2026-12-12 | RhHigh, TempLow | 2 | 6 | 2 | 1 | **11** | 14 |
| 2026-12-14 | TempHigh | 1 | 6 | 7 | 1 | **15** | 38 |
| 2026-12-18 | MainsFail | 1 | 0 | 1 | 0 | **2** | 13 |
| 2026-12-20 | MainsFail | 1 | 0 | 0 | 0 | **1** | 12 |
| 2026-12-21 | LowBattery | 1 | 0 | 1 | 0 | **2** | 15 |
| 2026-12-22 | TempHigh | 1 | 3 | 1 | 0 | **5** | 22 |
| 2026-12-24 | SensorFault | 1 | 0 | 1 | 0 | **2** | 13 |
| 2026-12-25 | SdFault | 1 | 0 | 0 | 0 | **1** | 13 |
| 2026-12-27 | PaperOut | 1 | 0 | 0 | 0 | **1** | 10 |
| 2026-12-28 | TempLow | 1 | 3 | 1 | 0 | **5** | 14 |
| 2026-12-29 | TempHigh | 1 | 6 | 1 | 1 | **9** | 12 |
| 2026-12-30 | TempHigh | 1 | 6 | 1 | 1 | **9** | 13 |
| 2026-12-31 | TempHigh | 1 | 6 | 1 | 1 | **9** | 13 |
| 2027-01-04 | RhLow | 1 | 5 | 1 | 0 | **7** | 12 |

The busiest day was the edge-hovering day (14 Dec): **15 alarm episodes, but only 15 texts per number.** The 60-minute
recurrence rule turned the repeats into capped "STILL" texts: 6 repeats, then one "daily limit" text, with
recoveries only for the episodes the farmer had been told about. During the heat wave (29-31 Dec), each day reached
the 6-repeat cap and sent one limit text. The cap reset at midnight.

## Findings for the product owner
1. **12-hour network outage (22 Dec):** the alerts raised during the outage were retried for 1 hour, then printed as
   "SMS NOT SENT" (10 texts across 2 numbers, requirement F-SMS-06). They were never delivered, even after the network
   came back. Printing and logging continued throughout. **Resolved (D-037, confirmed):** when the network returns,
   each number gets one offline summary ("No signal 07:00-19:00. N alerts not sent. Now T RH"). The replay sends 2.
2. **Humidity above 100 %:** my first generated data contained physically impossible readings (100.4 %RH). The
   firmware correctly treated them as a faulty sensor (F-MEAS-03), and the cross-check exposed the difference.
   The generator now caps humidity at 100 %, like the real SHT31. A probe sitting in condensation at a steady
   100 % stays valid.
3. **Power cuts:** both summaries due during power cuts were held and printed "late" when mains returned. Low battery
   was raised 1 h 34 min before mains came back in the 12.5 h cut (4.1 V to 3.3 V), with 2 % charge left.

## Checks that the gate can actually fail
- Planting a bug in the alert code (60-minute recurrence window cut to 10 minutes) made the gate fail on the 14 Dec
  hovering incident, exit code 1.
- The gate scripts themselves had a bug: a `| tee` subshell hid the failure exit code (phase 1 gate too). Fixed;
  both gates now exit non-zero on failure.

## Not proven by this replay
Real shed data, real sensor drift, and real modem or network timing: the fake modem answers instantly, while real
SMS submission can take seconds.
