#!/usr/bin/env python3
"""Generates a SYNTHETIC full rearing cycle for the phase-3 replay (no real shed logs exist yet, Q-12).

38 days at 30-second resolution, starting 2026-12-01 07:00 IST, deterministic (fixed seed).
Columns: t (UTC s), temp (C, 0.1), rh (%, 0.1), mains, batt_mv, net, sd, paper, sensor, sms_from, sms_body

Normal days: a well-managed shed tracking the stage target with a small daily swing and noise, kept
inside the alarm band so that ONLY the scripted incidents below can raise alerts.
Incidents (all deliberate; the replay must handle each exactly as the requirements say):
  I1  day  2 13:00-15:00  heat spike +3.5 C (incubation)                    -> TempHigh, repeats, recovery
  I2  day 11 00:30-04:30  cold damp night -3.5 C, RH +12 (chawki)           -> TempLow + RhHigh
  I3  day 13 12:00-15:00  temperature hovering at the upper band edge:
                          7 min at 29.6 C / 6 min at 28.3 C, repeatedly     -> recurrence rule, repeat cap
  I4  day 17 03:00-09:00  power cut across the 06:00 summary (battery 4.1->3.6 V) -> summary held, printed late
  I5  day 19 18:00 to day 20 06:30  long power cut, battery 4.1->3.3 V     -> MainsFail, LowBattery
  I6  day 21 07:00-19:00  no network, with a heat event 11:00-13:00         -> SMS held/given up, uploads queued
  I7  day 23 10:00-10:20  sensor unplugged                                  -> SensorFault, recovery
  I8  day 24 14:00-16:00  SD card removed                                   -> SdFault, data kept in RAM
  I9  day 26 20:00 to day 27 05:00 paper out, cold dip 01:00-02:00 -2.5 C  -> PaperOut, lines printed later
  I10 days 28-30 12:00-17:00 heat wave +3 C (instar V)                      -> daily repeat cap + limit SMS
  I11 day 34 13:00-16:00  dry air, RH -14 (spinning)                        -> RhLow
  I12 25 random mains flickers of 30-90 s                                   -> must NOT alert (2 min debounce)
  I13 15 random single invalid sensor samples                               -> must NOT raise a sensor fault
SMS commands from the farmer: owner registration, a helper number in Telugu, and every stage change.
"""
import csv
import datetime as dt
import math
import os
import random
import sys

SEED = 20261201
STEP = 30
IST = dt.timezone(dt.timedelta(hours=5, minutes=30))
START = dt.datetime(2026, 12, 1, 7, 0, tzinfo=IST)
DAYS = 38
OWNER, HELPER = "+919876543210", "+919123456780"
# stage index (command 11 n), start day, target mid temp, target mid RH, band (lo, hi) temp, (lo, hi) RH
STAGES = [
    (1, 0, 25.5, 75.0, (24.0, 27.0), (65.0, 85.0)),   # incubation 25-26 C, 70-80 %
    (2, 10, 27.5, 85.0, (26.0, 29.0), (75.0, 95.0)),  # chawki 27-28 C, 80-90 %
    (3, 16, 26.0, 80.0, (25.0, 27.0), (75.0, 85.0)),  # instar III
    (4, 20, 25.0, 70.0, (24.0, 26.0), (65.0, 75.0)),  # instar IV
    (5, 25, 24.0, 70.0, (23.0, 25.0), (65.0, 75.0)),  # instar V
    (6, 32, 26.0, 65.0, (23.0, 29.0), (55.0, 75.0)),  # spinning 24-28 C, 60-70 %
]


def at(day, hh, mm=0):
    return int((START.replace(hour=0, minute=0) + dt.timedelta(days=day, hours=hh, minutes=mm)).timestamp())


def stage_at(t):
    cur = STAGES[0]
    for s in STAGES:
        if t >= at(s[1], 8):
            cur = s
    return cur


def ramp(t, a, b, minutes=10):
    """0 outside [a, b), 1 inside, linear 10-minute ramps at both ends."""
    if t < a or t >= b:
        return 0.0
    r = minutes * 60
    return min(1.0, (t - a) / r, (b - t) / r)


def main(out):
    rng = random.Random(SEED)
    t0 = int(START.timestamp())
    n = DAYS * 86400 // STEP
    flickers = set()
    while len(flickers) < 25:
        d = rng.randint(1, DAYS - 2)
        if d in (17, 19, 20, 21):
            continue
        start = at(d, rng.randint(0, 23), rng.randint(0, 59))
        for k in range(rng.randint(1, 3)):
            flickers.add(start + k * STEP - (start - t0) % STEP)
    glitches = set()
    while len(glitches) < 15:
        glitches.add(t0 + STEP * rng.randint(100, n - 100))
    sms = {t0 + 2 * STEP: (OWNER, "10 000000"), t0 + 4 * STEP: (OWNER, "21 9123456780"),
           t0 + 6 * STEP: (HELPER, "12 3"), t0 + 8 * STEP: (OWNER, "11 1")}
    for s in STAGES[1:]:
        sms[at(s[1], 8)] = (OWNER, f"11 {s[0]}")
    tn = rh_n = 0.0
    rows = []
    for i in range(n):
        t = t0 + i * STEP
        st = stage_at(t)
        (tlo, thi), (rlo, rhi) = st[4], st[5]
        hour = (dt.datetime.fromtimestamp(t, IST).hour + dt.datetime.fromtimestamp(t, IST).minute / 60)
        swing = math.sin((hour - 9) / 24 * 2 * math.pi)  # peak around 15:00
        tn = 0.97 * tn + rng.gauss(0, 0.03)
        rh_n = 0.97 * rh_n + rng.gauss(0, 0.15)
        temp = st[2] + 0.5 * swing + tn
        rh = st[3] - 2.0 * swing + rh_n
        temp = min(max(temp, tlo + 0.4), thi - 0.4)
        rh = min(max(rh, rlo + 2.0), rhi - 2.0)
        mains, batt, net, sd, paper, sensor = 1, 4100, 1, 1, 1, 1
        # ---- incidents ----
        temp += 3.5 * ramp(t, at(2, 13), at(2, 15))                                     # I1
        temp -= 3.5 * ramp(t, at(11, 0, 30), at(11, 4, 30))                             # I2
        rh += 12.0 * ramp(t, at(11, 0, 30), at(11, 4, 30))
        if at(13, 12) <= t < at(13, 15):                                                # I3
            temp = 29.6 if ((t - at(13, 12)) // 60) % 13 < 7 else 28.3
        if at(17, 3) <= t < at(17, 9):                                                  # I4
            mains, batt = 0, int(4100 - 500 * (t - at(17, 3)) / (6 * 3600))
        if at(19, 18) <= t < at(20, 6, 30):                                             # I5
            mains, batt = 0, int(4100 - 800 * (t - at(19, 18)) / (12.5 * 3600))
        if at(21, 7) <= t < at(21, 19):                                                 # I6
            net = 0
        temp += 3.0 * ramp(t, at(21, 11), at(21, 13))
        if at(23, 10) <= t < at(23, 10, 20):                                            # I7
            sensor = 0
        if at(24, 14) <= t < at(24, 16):                                                # I8
            sd = 0
        if at(26, 20) <= t < at(27, 5):                                                 # I9
            paper = 0
        temp -= 2.5 * ramp(t, at(27, 1), at(27, 2))
        for d in (28, 29, 30):                                                          # I10
            temp += 3.0 * ramp(t, at(d, 12), at(d, 17))
        rh -= 14.0 * ramp(t, at(34, 13), at(34, 16))                                    # I11
        rh = min(max(rh, 0.0), 100.0)  # a real SHT31 reports 0-100 %RH (raw 0-65535)
        if t in flickers:                                                               # I12
            mains = 0
        if t in glitches:                                                               # I13
            sensor = 0
        frm, body = sms.get(t, ("", ""))
        rows.append([t, f"{temp:.1f}", f"{rh:.1f}", mains, batt, net, sd, paper, sensor, frm, body])
    os.makedirs(os.path.dirname(out) or ".", exist_ok=True)
    with open(out, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["t", "temp", "rh", "mains", "batt_mv", "net", "sd", "paper", "sensor", "sms_from", "sms_body"])
        w.writerows(rows)
    print(f"wrote {len(rows)} rows ({DAYS} days) to {out}")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "sim/replay/out/cycle.csv")
