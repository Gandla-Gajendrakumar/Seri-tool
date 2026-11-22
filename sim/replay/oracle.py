#!/usr/bin/env python3
"""Independent oracle for the phase-3 replay: predicts every event and every SMS decision for a cycle CSV,
working ONLY from the written rules (docs/requirements.md, docs/decisions.md), not from the C++ code.

Rules used (requirement / decision):
  sampling      raw read every 6 s from boot+1 s; every 5th read gives a 30 s sample (D-006); a sample is
                invalid if the probe fails (F-MEAS-03)
  climate       start after 300 s of consecutive out-of-band samples; clear after 2 consecutive samples
                inside by 0.5 C / 2 %RH (F-EVT-01/02); band = target range +/- tolerance (Q-3)
  sensor fault  3 invalid samples (60 s) start, 3 valid clear (F-MEAS-04)
  mains         checked every 1 s; 120 s debounce both ways (D-008)
  low battery   on battery and <= 3400 mV for 60 s; clears on mains or >= 3700 mV (D-019)
  paper         checked every 5 s, immediate; SD checked every 30 s, 2 failed checks (30 s)
  stage change  takes effect when the SMS is read (poll every 5 s, needs network); active climate events
                no longer out of band clear silently; pending timers restart (F-EVT-07, D-009)
  SMS policy    F-SMS-05, D-012/013/014 + reboot guard: first alert free unless the same event ended
                < 60 min ago (then it is a repeat); repeats every 30 min (climate, sensor), max 6/day/number;
                then one "limit" text; hard ceiling 20/day/number with room reserved for recoveries;
                recovery texts for climate, sensor and mains events only; counters reset at IST midnight
Within one second the order is: power, sensor sample, paper, SD/clock, SMS repeat tick (every 60 s), SMS poll.

Usage: oracle.py cycle.csv expected.txt
"""
import csv
import datetime as dt
import sys
from decimal import Decimal

IST = dt.timezone(dt.timedelta(hours=5, minutes=30))
TYPES = ["TempHigh", "TempLow", "RhHigh", "RhLow", "SensorFault", "MainsFail", "LowBattery", "PaperOut", "SdFault",
         "ClockInvalid"]
CLIMATE = TYPES[:4]
REPEATS = set(CLIMATE + ["SensorFault"])
RECOVERY = set(CLIMATE + ["SensorFault", "MainsFail"])
TARGETS = {1: (250, 260, 700, 800), 2: (270, 280, 800, 900), 3: (260, 260, 800, 800), 4: (250, 250, 700, 700),
           5: (240, 240, 700, 700), 6: (240, 280, 600, 700)}  # stage_profiles.inc (tenths)
TOL_T, TOL_RH = 10, 50
ONSET = {"TempHigh": 300, "TempLow": 300, "RhHigh": 300, "RhLow": 300, "SensorFault": 60, "MainsFail": 120,
         "LowBattery": 60, "PaperOut": 0, "SdFault": 30, "ClockInvalid": 0}
CLEAR = {"TempHigh": 30, "TempLow": 30, "RhHigh": 30, "RhLow": 30, "SensorFault": 60, "MainsFail": 120,
         "LowBattery": 0, "PaperOut": 0, "SdFault": 0, "ClockInvalid": 0}


def band(stage):
    tmin, tmax, rmin, rmax = TARGETS[stage]
    return tmin - TOL_T, tmax + TOL_T, max(0, rmin - TOL_RH), min(1000, rmax + TOL_RH)


def ist(t):
    return dt.datetime.fromtimestamp(t, IST)


def ist_day(t):
    return ist(t).date()


class Deb:
    def __init__(self, name):
        self.name, self.active, self.bad_since, self.good_since = name, False, None, None

    def step(self, now, bad, good):
        if not self.active:
            self.good_since = None
            if not bad:
                self.bad_since = None
                return 0
            if self.bad_since is None:
                self.bad_since = now
            if now - self.bad_since >= ONSET[self.name]:
                self.active, self.bad_since = True, None
                return 1
            return 0
        self.bad_since = None
        if not good:
            self.good_since = None
            return 0
        if self.good_since is None:
            self.good_since = now
        if now - self.good_since >= CLEAR[self.name]:
            self.active, self.good_since = False, None
            return -1
        return 0


class Policy:
    def __init__(self, out):
        self.out, self.day = out, None
        self.r = [dict(rep=0, tot=0, res=0, lim=False) for _ in range(3)]
        self.ep = {t: dict(active=False, ended=None, start=0, last=0, told=set()) for t in TYPES}

    def roll(self, now):
        d = ist_day(now)
        if d != self.day:
            self.day = d
            for r in self.r:
                r.update(rep=0, tot=0, lim=False)

    def send(self, i, kind, typ, now, reserve):
        r = self.r[i]
        need = 2 if reserve else 1
        if (kind == "Start" or r["rep"] < 6) and r["tot"] + r["res"] + need <= 20:
            self.out.append(("Q", now, kind, typ, i))
            r["rep"] += kind == "Repeat"
            r["tot"] += 1
            r["res"] += reserve
            return True
        if not r["lim"] and r["tot"] + r["res"] + 1 <= 20:
            self.out.append(("Q", now, "LimitReached", typ, i))
            r["lim"] = True
            r["tot"] += 1
        return False

    def event(self, typ, edge, now, recipients):
        self.roll(now)
        e = self.ep[typ]
        if edge == "Start":
            if e["active"]:
                return
            recurrent = e["ended"] is not None and now - e["ended"] < 3600
            e.update(active=True, start=now, last=now, told=set())
            for i in recipients:
                if self.send(i, "Repeat" if recurrent else "Start", typ, now, typ in RECOVERY):
                    e["told"].add(i)
            return
        if not e["active"]:
            return
        for i in sorted(e["told"]):
            if typ not in RECOVERY:
                continue
            if self.r[i]["res"]:
                self.r[i]["res"] -= 1
            if edge == "Clear" and i in recipients:
                self.out.append(("Q", now, "Recovery", typ, i))
                self.r[i]["tot"] += 1
        e.update(active=False, ended=now, told=set())

    def tick(self, now, recipients):
        self.roll(now)
        for typ in TYPES:
            e = self.ep[typ]
            if not e["active"] or typ not in REPEATS or now - e["last"] < 1800:
                continue
            e["last"] = now
            for i in recipients:
                first = i not in e["told"]
                if self.send(i, "Repeat", typ, now, first and typ in RECOVERY):
                    e["told"].add(i)


def main(src, dst):
    rows = list(csv.DictReader(open(src)))
    t0, n = int(rows[0]["t"]), len(rows)
    out = []
    pol = Policy(out)
    deb = {t: Deb(t) for t in TYPES}
    phones = ["", "", ""]
    stage = 2  # factory default: chawki
    b = band(stage)
    last = None  # last valid sample (t, rh)
    inbox = []
    reads = 0

    def recips():
        return [i for i in range(3) if phones[i]]

    def emit(typ, edge, now):
        out.append(("E", now, typ, edge))
        pol.event(typ, edge, now, recips())

    def step(typ, now, bad, good):
        r = deb[typ].step(now, bad, good)
        if r:
            emit(typ, "Start" if r > 0 else "Clear", now)

    end = t0 + n * 30
    for s in range(t0 + 1, end):
        row = rows[(s - t0) // 30]
        if s == int(row["t"]) and row["sms_from"]:
            inbox.append((row["sms_from"], row["sms_body"]))
        mains, mv = row["mains"] == "1", int(row["batt_mv"])
        # 1. power (every second)
        step("MainsFail", s, not mains, mains)
        step("LowBattery", s, (not mains) and mv <= 3400, mains or mv >= 3700)
        # 2. raw read every 6 s from boot+1; 5th read -> sample
        if (s - t0 - 1) % 6 == 0:
            reads += 1
            if reads % 5 == 0:
                valid = row["sensor"] == "1"
                step("SensorFault", s, not valid, valid)
                if valid:
                    t = int(Decimal(row["temp"]) * 10)
                    rh = int(Decimal(row["rh"]) * 10)
                    last = (t, rh)
                    step("TempHigh", s, t > b[1], t <= b[1] - 5)
                    step("TempLow", s, t < b[0], t >= b[0] + 5)
                    step("RhHigh", s, rh > b[3], rh <= b[3] - 20)
                    step("RhLow", s, rh < b[2], rh >= b[2] + 20)
        # 3. paper every 5 s
        if (s - t0 - 1) % 5 == 0:
            ok = row["paper"] == "1"
            step("PaperOut", s, not ok, ok)
        # 4. SD + clock every 30 s
        if (s - t0 - 1) % 30 == 0:
            ok = row["sd"] == "1"
            step("SdFault", s, not ok, ok)
        # 5. SMS repeat tick every 60 s from boot+60
        if (s - t0) % 60 == 0:
            pol.tick(s, recips())
        # 6. SMS poll every 5 s (needs network)
        if (s - t0 - 1) % 5 == 0 and row["net"] == "1":
            while inbox:
                frm, body = inbox.pop(0)
                num = frm[-10:]
                parts = body.split()
                slot = phones.index(num) if num in phones else -1
                if parts[0] == "10" and parts[1] == "000000":
                    phones = [num if i == 0 else ("" if p == num else p) for i, p in enumerate(phones)]
                elif slot == 0 and parts[0] == "21" and parts[1][-10:] not in phones:
                    free = next(i for i in (1, 2) if not phones[i])
                    phones[free] = parts[1][-10:]
                elif slot >= 0 and parts[0] == "11":
                    new = int(parts[1])
                    if new != stage:
                        stage, b = new, band(new)
                        for typ in CLIMATE:
                            d = deb[typ]
                            if d.active and last is not None:
                                t, rh = last
                                still = {"TempHigh": t > b[1], "TempLow": t < b[0], "RhHigh": rh > b[3],
                                         "RhLow": rh < b[2]}[typ]
                                if not still:
                                    d.active = False
                                    emit(typ, "SilentClear", s)
                            d.bad_since = d.good_since = None
    write(out, dst, t0)


def fmt(t):
    return ist(t).strftime("%Y-%m-%d %H:%M:%S")


def write(out, dst, t0):
    lines = ["# Phase-3 replay expected output (generated by sim/replay/oracle.py from the rules)"]
    for o in out:
        if o[0] == "E":
            lines.append(f"E {fmt(o[1])} {o[2]} {o[3]}")
        else:
            lines.append(f"Q {fmt(o[1])} {o[2]} {o[3]} slot{o[4]}")
    days = {}
    for o in out:
        if o[0] == "Q":
            k = (ist_day(o[1]).isoformat(), o[4])
            days.setdefault(k, {"Start": 0, "Repeat": 0, "Recovery": 0, "LimitReached": 0})[o[2]] += 1
    for (d, slot), c in sorted(days.items()):
        lines.append(f"DAY {d} slot{slot} start={c['Start']} repeat={c['Repeat']} recovery={c['Recovery']} "
                     f"limit={c['LimitReached']} total={sum(c.values())}")
    ev = [o for o in out if o[0] == "E"]
    lines.append(f"TOTAL events={len(ev)} starts={sum(1 for o in ev if o[3] == 'Start')} "
                 f"sms_decisions={sum(1 for o in out if o[0] == 'Q')}")
    open(dst, "w").write("\n".join(lines) + "\n")
    print(lines[-1])


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
