#!/usr/bin/env python3
"""Creates microSD provisioning files for a batch of devices on HiveMQ Cloud (D-060, D-061).

  make_provision.py --host <cluster>.s1.eu.hivemq.cloud --count 10 [--prefix seri] [--start 1] [--out provision]

For each device it writes <out>/<client_id>/seri.cfg and ca.pem (Let's Encrypt ISRG Root X1, which signs
HiveMQ Cloud Free/PayG certificates), plus <out>/credentials.csv: enter each username/password in the HiveMQ
Cloud console (Access Management) before the device goes online. Every pair of files is checked with the
firmware's own parser (tools/check_cfg.cpp). The output holds passwords: keep it out of git (it is ignored)
and delete it after the cards are written.
"""
import argparse
import csv
import os
import re
import secrets
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
CA = os.path.join(ROOT, "docs", "handoff", "hivemq", "isrg-root-x1.pem")


def build_checker():
    exe = os.path.join(tempfile.mkdtemp(), "check_cfg")
    core = os.path.join(ROOT, "firmware", "lib", "core")
    subprocess.run(["g++", "-std=gnu++17", "-I" + os.path.join(core, "include"), os.path.join(ROOT, "tools", "check_cfg.cpp"),
                    os.path.join(core, "src", "provision.cpp"), "-o", exe], check=True)
    return exe


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--host", required=True, help="HiveMQ Cloud cluster URL (from the cluster overview), no scheme")
    ap.add_argument("--port", type=int, default=8883)
    ap.add_argument("--count", type=int, required=True)
    ap.add_argument("--prefix", default="seri")
    ap.add_argument("--start", type=int, default=1)
    ap.add_argument("--out", default=os.path.join(ROOT, "provision"))
    a = ap.parse_args(argv)
    if not re.fullmatch(r"[A-Za-z0-9.-]+", a.host) or "://" in a.host:
        sys.exit("host must be a bare host name, e.g. abc123.s1.eu.hivemq.cloud")
    checker = build_checker()
    os.makedirs(a.out, exist_ok=True)
    rows = []
    for n in range(a.start, a.start + a.count):
        cid = f"{a.prefix}-{n:04d}"
        user, pw = cid, secrets.token_urlsafe(18)   # 24 URL-safe characters, no '=' or spaces
        d = os.path.join(a.out, cid)
        os.makedirs(d, exist_ok=True)
        with open(os.path.join(d, "seri.cfg"), "w", newline="\n") as f:
            f.write(f"# Seri-tool device {cid}: copy seri.cfg and ca.pem to the root of the microSD card\n"
                    f"mqtt_host={a.host}\nmqtt_port={a.port}\nmqtt_user={user}\nmqtt_pass={pw}\nclient_id={cid}\n")
        shutil.copyfile(CA, os.path.join(d, "ca.pem"))
        r = subprocess.run([checker, os.path.join(d, "seri.cfg"), os.path.join(d, "ca.pem")], capture_output=True, text=True)
        if r.returncode != 0:
            sys.exit(f"{cid}: {r.stdout.strip()}")
        rows.append((cid, user, pw, f"seri/{cid}/data"))
    with open(os.path.join(a.out, "credentials.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["client_id", "username", "password", "publish_topic"])
        w.writerows(rows)
    print(f"{len(rows)} devices -> {os.path.relpath(a.out, ROOT)}/ (checked with the firmware parser). "
          f"Add the users in credentials.csv to HiveMQ Cloud, write the cards, then delete this folder.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
