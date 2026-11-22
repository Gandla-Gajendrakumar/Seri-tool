#include "printer_escpos.h"
#include <vector>

void EscPosPrinter::begin() {
  s_.begin(9600, SERIAL_8N1, rx_, tx_);
  const uint8_t init[] = {0x1B, 0x40};  // ESC @
  s_.write(init, sizeof init);
}

bool EscPosPrinter::paperPresent() {
  while (s_.available()) s_.read();
  const uint8_t q[] = {0x10, 0x04, 0x04};  // DLE EOT 4: paper sensor status
  s_.write(q, sizeof q);
  const uint32_t deadline = millis() + 100;
  while (static_cast<int32_t>(deadline - millis()) > 0) {
    if (s_.available()) {
      const int b = s_.read();
      statusSeen_ = true;
      lastPaper_ = (b & 0x60) == 0;
      return lastPaper_;
    }
    delay(1);
  }
  return statusSeen_ ? lastPaper_ : true;
}

bool EscPosPrinter::printLine(const std::string& utf8, seri::Language lang) {
  if (!paperPresent()) return false;
  bool ascii = true;
  for (unsigned char c : utf8) if (c >= 0x80) { ascii = false; break; }
  if (ascii) {
    s_.print(utf8.c_str());
    s_.write('\n');
    return true;
  }
  std::vector<uint8_t> bits;
  int wb = 0, h = 0;
  if (!raster_ || !raster_(utf8, lang, bits, wb, h)) return false;
  printRaster(bits, wb, h);
  return true;
}

void EscPosPrinter::printRaster(const std::vector<uint8_t>& bits, int widthBytes, int height) {
  const uint8_t hdr[] = {0x1D, 0x76, 0x30, 0x00, static_cast<uint8_t>(widthBytes & 0xFF), static_cast<uint8_t>(widthBytes >> 8),
                         static_cast<uint8_t>(height & 0xFF), static_cast<uint8_t>(height >> 8)};
  s_.write(hdr, sizeof hdr);
  s_.write(bits.data(), bits.size());
}
