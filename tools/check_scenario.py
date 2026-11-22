#!/usr/bin/env python3
"""Checks a Wokwi run log against a scenario's .expect file.

Rules, one per line (regular expressions, matched anywhere in a line):
  in-order: <regex>        must appear, after the line matched by the previous in-order rule
  present: <regex>         must appear somewhere
  absent: <regex>          must not appear
  count <op> <n>: <regex>  number of matching lines, op is one of == >= <=
Lines starting with # are comments.

Usage: check_scenario.py <rules.expect> <run.log>   (exit 0 = pass)
"""
import re
import sys

ANSI = re.compile(r"\x1b\[[0-9;]*m")
RULE = re.compile(r"^(in-order|present|absent|count\s*(==|>=|<=)\s*(\d+))\s*:\s?(.*)$")


def parse_rules(text):
    rules = []
    for n, raw in enumerate(text.splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        m = RULE.match(line)
        if not m:
            raise ValueError(f"line {n}: cannot parse rule: {raw}")
        kind = "count" if m.group(1).startswith("count") else m.group(1)
        rules.append({"kind": kind, "op": m.group(2), "n": int(m.group(3) or 0),
                      "re": re.compile(m.group(4)), "src": m.group(4), "line": n})
    return rules


def check(rules, lines):
    lines = [ANSI.sub("", l) for l in lines]
    errors = []
    pos = 0
    for r in rules:
        hits = [i for i, l in enumerate(lines) if r["re"].search(l)]
        if r["kind"] == "in-order":
            nxt = [i for i in hits if i >= pos]
            if not nxt:
                where = "at all" if not hits else f"after line {pos} (seen earlier at line {hits[0] + 1})"
                errors.append(f"rule {r['line']} in-order: '{r['src']}' not found {where}")
            else:
                pos = nxt[0] + 1
        elif r["kind"] == "present" and not hits:
            errors.append(f"rule {r['line']} present: '{r['src']}' not found")
        elif r["kind"] == "absent" and hits:
            errors.append(f"rule {r['line']} absent: '{r['src']}' found at line {hits[0] + 1}: {lines[hits[0]].strip()}")
        elif r["kind"] == "count":
            n, want = len(hits), r["n"]
            ok = {"==": n == want, ">=": n >= want, "<=": n <= want}[r["op"]]
            if not ok:
                errors.append(f"rule {r['line']} count {r['op']} {want}: '{r['src']}' matched {n} lines")
    return errors


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    rules = parse_rules(open(argv[1], encoding="utf-8").read())
    lines = open(argv[2], encoding="utf-8", errors="replace").read().splitlines()
    errors = check(rules, lines)
    name = argv[1].rsplit("/", 1)[-1]
    for e in errors:
        print(f"FAIL {name}: {e}")
    print(f"{'PASS' if not errors else 'FAIL'} {name}: {len(rules)} rules, {len(errors)} failed")
    return 0 if not errors else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
