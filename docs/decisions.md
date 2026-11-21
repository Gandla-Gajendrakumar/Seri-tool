# Decision log

Decisions I made without asking, because they don't change cost, farmer experience or scope.
Newest last. Fixed decisions from the brief are in `requirements.md` §2 and are not repeated here.

| # | Date | Decision | Reason |
|---|---|---|---|
| D-001 | 2026-11-21 | Build system: **PlatformIO** with two environments: `native` (PC unit tests, Unity framework) and `esp32` (Arduino framework, board `esp32dev`, i.e. DevKitC WROOM-32). | One tool builds both the testable pure-logic code and the target firmware. Arduino gives ready drivers for SHT31, DS3231, OLED and SD. ESP-IDF APIs stay reachable for OTA and watchdog. |
| D-002 | 2026-11-21 | **KiCad 9.0.9** (from the official KiCad PPA), not Ubuntu's KiCad 7.0. | KiCad 7's command-line tool cannot run the electrical rules check (ERC) or design rules check (DRC). The phase 4 gate needs both from the command line. |
| D-003 | 2026-11-21 | Created a personal cloud environment **"Seri-tool"** for this repo. Its allowed hosts: PlatformIO registry + its download mirror, Espressif downloads, wokwi.com, Launchpad (KiCad PPA). | The default environment blocked the PlatformIO registry and the KiCad PPA. |
| D-004 | 2026-11-21 | ngspice 42 (Ubuntu), CadQuery 2.8.0 (pip), wokwi-cli 0.29.1 (static binary from its GitHub release; it isn't published on npm). | Latest available through allowed channels. All exercised by `tools/check_toolchain.sh`. |
| D-005 | 2026-11-21 | Time zone fixed at **IST (UTC+5:30)**. Timestamps are stored in UTC on SD and over MQTT, and shown and printed in IST. | India has one time zone and no daylight saving. UTC storage avoids ambiguity in the cloud. |
| D-006 | 2026-11-21 | Raw SHT31 reads every 6 s; the median of 5 forms the 30 s sample. | Spreads the 5 reads across the window, so the median rejects short spikes (e.g. a door opening) rather than just electrical noise. |
| D-007 | 2026-11-21 | Every SMS template must fit one segment: 70 UCS-2 characters for Indian scripts, 160 for English. Enforced by a unit test. | Multi-part SMS can arrive out of order or partially on basic phones, and costs more. |
| D-008 | 2026-11-21 | Debounce mains-fail / mains-return by 2 minutes. | Rural supply flickers often. Without debounce, this alone would flood SMS (violates "must not happen"). |
| D-009 | 2026-11-21 | Stage changes clear active events silently (log + print only). | A farmer changing the stage should not receive a burst of "recovered" texts. |
| D-010 | 2026-11-21 | SMS cap counters and settings are kept in ESP32 NVS flash. | A reboot loop must not reset the cap and cause a flood. |
| D-011 | 2026-11-21 | OTA uses ESP32 dual app partitions with signed images and rollback. | The standard, proven ESP32 mechanism. A bad update must never brick a unit in a remote shed. |
