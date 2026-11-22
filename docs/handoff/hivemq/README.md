# HiveMQ Cloud set-up (product-owner choice, D-061)

The devices upload readings by MQTT over TLS to a HiveMQ Cloud **Serverless (free)** cluster. SMS alerts and
printing never depend on it. Free tier (HiveMQ pricing page and community FAQ, 2026-11-22): **100 connections,
10 GB/month**, MQTT over TLS on port **8883**; clients must send **TLS SNI**; Free/PayG clusters use **Let's Encrypt**
certificates (root: ISRG Root X1).

## One-time: create the cluster
1. Sign up at HiveMQ Cloud and create a **Serverless** cluster.
2. Copy the **cluster URL** from the cluster overview (looks like `xxxxxxxx.s1.eu.hivemq.cloud`).

## Per batch of devices
1. Generate the card files and passwords:
   `python3 tools/make_provision.py --host <cluster URL> --count 10`
   This writes `provision/seri-0001/{seri.cfg, ca.pem}`, ... and `provision/credentials.csv`. Every file is
   checked with the firmware's own parser. The folder is ignored by git.
2. In the HiveMQ console, **Access Management**: add one credential per row of `credentials.csv`
   (username and password). Give it the narrowest permission the console offers that still allows publishing.
3. Copy each device's two files to the root of its microSD card (one folder per device), power it once and check
   the card's event log for `CFG mqtt imported host=...` (installer-guide.md).
4. **Delete the `provision/` folder** (it holds the passwords) once the cards are done; keep `credentials.csv`
   only in a password manager.

## What the device sends
* Topic: `seri/<client_id>/data` (e.g. `seri/seri-0001/data`), one JSON record every 5 min on mains, 15 min on battery:
  `{"d":"Shed-1","s":[[<utc>,<t x10>,<rh x10>],...],"m":1,"b":<battery %>}`.
* Volume: about 0.3 KB per record, about 3 MB per device per month. 100 devices ≈ 0.3 GB, well inside 10 GB.
* `client_id` is unique per device (HiveMQ disconnects the older session if two devices share one).

## Certificate
`docs/handoff/hivemq/isrg-root-x1.pem` is the ISRG Root X1 certificate from the Ubuntu ca-certificates store
(SHA-256 fingerprint `96:BC:EC:06:26:49:76:F3:74:60:77:9A:CF:28:C5:A7:CF:E8:A3:C0:AA:E1:1A:8F:FC:EE:05:C0:BD:DF:08:C6`,
valid to 2035-06-04). If HiveMQ changes its certificate authority, replace this file and re-provision.

## Known risk (bench item B-28)
The A7670C needs `AT+CSSLCFG="enableSNI",0,1` for HiveMQ (added to the driver; syntax from SIMCom A76xx community
usage, the SIMCom SSL application note could not be downloaded here). HiveMQ forum posts report certificate errors on
some SIMCom firmware versions. Test 4G and Wi-Fi uploads to the real cluster before deploying.
