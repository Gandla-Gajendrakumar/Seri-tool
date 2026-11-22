#!/usr/bin/env python3
"""Phase-3 checks on the replay output beyond the exact match:
  - SMS caps: per number per IST day, at most 20 texts in total and at most 6 repeats (F-SMS-05, D-012)
  - every alert the farmer was told about got exactly one recovery text (recovery-type events)
  - monitoring never stopped: no gap between valid-or-invalid samples above 30 s
  - one daily summary per day of the cycle (38), summaries held during power cuts printed late
  - every SMS job either sent or reported as not sent (nothing silently lost)
Usage: check_cycle.py actual.txt stats.txt   (exit 0 = all checks pass)
"""
import re
import sys

RECOVERY = {"TempHigh", "TempLow", "RhHigh", "RhLow", "SensorFault", "MainsFail"}


def main(actual, stats):
    fails, notes = [], []
    lines = open(actual).read().splitlines()
    st = open(stats).read()
    worst_total = worst_repeat = 0
    for l in lines:
        m = re.match(r"DAY (\S+) slot(\d) start=(\d+) repeat=(\d+) recovery=(\d+) limit=(\d+) total=(\d+)", l)
        if m:
            rep, tot = int(m.group(4)), int(m.group(7))
            worst_total, worst_repeat = max(worst_total, tot), max(worst_repeat, rep)
            if tot > 20:
                fails.append(f"{m.group(1)} slot{m.group(2)}: {tot} SMS > 20")
            if rep > 6:
                fails.append(f"{m.group(1)} slot{m.group(2)}: {rep} repeats > 6")
    notes.append(f"worst day per number: {worst_total} SMS (cap 20), {worst_repeat} repeats (cap 6)")
    # told vs recovered, per (type, slot): count episodes with any Start/Repeat after the episode began
    open_ep, told, recovered = {}, 0, 0
    for l in lines:
        p = l.split()
        if p[0] == "E" and p[3] in RECOVERY:
            if p[4] == "Start":
                open_ep[p[3]] = set()
            elif p[3] in open_ep:
                open_ep.pop(p[3])
        if p[0] == "Q" and p[4] in RECOVERY:
            if p[3] in ("Start", "Repeat") and p[4] in open_ep and p[5] not in open_ep[p[4]]:
                open_ep[p[4]].add(p[5])
                told += 1
            elif p[3] == "Recovery":
                recovered += 1
    notes.append(f"told episodes per number: {told}, recovery texts: {recovered}")
    if told != recovered:
        fails.append(f"told {told} != recovered {recovered}")
    gap = int(re.search(r"max_gap_between_samples_s=(\d+)", st).group(1))
    if gap > 30:
        fails.append(f"monitoring gap of {gap} s")
    summaries = int(re.search(r"daily_summaries=(\d+)", st).group(1))
    late = int(re.search(r"printed_late=(\d+)", st).group(1))
    if summaries != 38:
        fails.append(f"{summaries} daily summaries, expected 38")
    if late < 1:
        fails.append("no summary was printed late although power was off at 06:00")
    decisions = int(re.search(r"sms_decisions=(\d+)", open(actual).read()).group(1))
    sent = int(re.search(r"sms_sent=(\d+)", st).group(1))
    not_sent = int(re.search(r"sms_not_sent_lines=(\d+)", st).group(1))
    replies = 9  # owner claim, add helper, helper language, 6 stage changes (gen_cycle.py)
    offline = int(re.search(r"offline_summaries=(\d+)", st).group(1))
    if not_sent and not offline:
        fails.append("alerts were given up during an outage but no offline summary was sent (D-037)")
    if decisions + replies + offline != sent + not_sent:
        fails.append(f"SMS lost: {decisions} alerts + {replies} replies + {offline} offline summaries != "
                     f"{sent} sent + {not_sent} reported not sent")
    notes.append(f"SMS jobs {decisions + replies + offline} (incl. {offline} offline summaries) = sent {sent} + "
                 f"reported not sent {not_sent}")
    notes.append(f"monitoring gap max {gap} s; summaries {summaries} (late {late})")
    for n in notes:
        print("  " + n)
    for f in fails:
        print("FAIL " + f)
    print("CHECKS: PASSED" if not fails else "CHECKS: FAILED")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1], sys.argv[2]))
