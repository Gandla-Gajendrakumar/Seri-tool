#!/usr/bin/env bash
# Builds the hand-off outputs: PCB fabrication files, drawings, and the firmware release binaries.
#   hardware/fab/   Gerbers + Excellon drill (zipped for the fab), schematic PDF, assembly PDF, board STEP
#   firmware/release/v<ver>/  bootloader, partitions, boot_app0, application, flash.sh, SHA256SUMS
set -eu
cd "$(dirname "$0")/.."
k=hardware/kicad/carrier
fab=hardware/fab
rm -rf $fab && mkdir -p $fab/gerbers
kicad-cli pcb export gerbers --layers F.Cu,B.Cu,F.Mask,B.Mask,F.Silkscreen,B.Silkscreen,F.Paste,Edge.Cuts \
  --subtract-soldermask -o $fab/gerbers/ $k/carrier.kicad_pcb > /dev/null
kicad-cli pcb export drill --format excellon --excellon-separate-th --generate-map --map-format pdf \
  -o $fab/gerbers/ $k/carrier.kicad_pcb > /dev/null
(cd $fab/gerbers && zip -q -X ../carrier-v1-gerbers.zip *)
kicad-cli sch export pdf -o $fab/carrier-v1-schematic.pdf $k/carrier.kicad_sch > /dev/null
kicad-cli pcb export pdf --layers F.Fab,F.Silkscreen,Edge.Cuts --include-border-title -o $fab/carrier-v1-assembly-top.pdf $k/carrier.kicad_pcb > /dev/null
kicad-cli pcb export step --subst-models -f -o $fab/carrier-v1.step $k/carrier.kicad_pcb > /dev/null 2>&1 || echo "  (board STEP export skipped)"
cp bom/bom.csv $fab/carrier-v1-bom.csv
echo "fab: $(ls $fab/gerbers | wc -l) Gerber/drill files, $(du -h $fab/carrier-v1-gerbers.zip | cut -f1) zip"

ver=$(grep -o 'kFirmwareVersion = "[^"]*"' firmware/include/board.h | cut -d'"' -f2)
rel=firmware/release/v$ver
(cd firmware && pio run -e esp32 > /tmp/release_build.log 2>&1) || { tail -20 /tmp/release_build.log; exit 1; }
b=firmware/.pio/build/esp32
boot_app0=$(find ~/.platformio/packages -name boot_app0.bin | head -1)
rm -rf $rel && mkdir -p $rel
cp $b/bootloader.bin $b/partitions.bin $b/firmware.bin "$boot_app0" $rel/
cat > $rel/flash.sh <<'SH'
#!/usr/bin/env bash
# Flash Seri-tool firmware to an ESP32-DevKitC over USB. REMOVE jumper JP2 on the carrier first (D-043).
# Usage: ./flash.sh /dev/ttyUSB0      (needs: pip install esptool)
set -e
cd "$(dirname "$0")"
sha256sum -c SHA256SUMS
esptool.py --chip esp32 --port "${1:-/dev/ttyUSB0}" --baud 460800 write_flash -z \
  0x1000 bootloader.bin 0x8000 partitions.bin 0xe000 boot_app0.bin 0x10000 firmware.bin
SH
chmod +x $rel/flash.sh
(cd $rel && sha256sum bootloader.bin partitions.bin boot_app0.bin firmware.bin > SHA256SUMS)
echo "firmware: $rel ($(du -h $rel/firmware.bin | cut -f1) application)"
