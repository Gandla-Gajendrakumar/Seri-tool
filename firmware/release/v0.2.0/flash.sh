#!/usr/bin/env bash
# Flash Seri-tool firmware to an ESP32-DevKitC over USB. REMOVE jumper JP2 on the carrier first (D-043).
# Usage: ./flash.sh /dev/ttyUSB0      (needs: pip install esptool)
set -e
cd "$(dirname "$0")"
sha256sum -c SHA256SUMS
esptool.py --chip esp32 --port "${1:-/dev/ttyUSB0}" --baud 460800 write_flash -z \
  0x1000 bootloader.bin 0x8000 partitions.bin 0xe000 boot_app0.bin 0x10000 firmware.bin
