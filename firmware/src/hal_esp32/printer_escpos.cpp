#include "printer_escpos.h"
#include <vector>

void EscPosPrinter::begin() {
  pinMode(en_, OUTPUT);
  powerOn();
  // Baud auto-detect (D-041): units ship at 9600 or 19200. The first rate that answers a status
  // query wins; a TX-only printer gets 9600.
  for (uint32_t b : {9600u, 19200u}) {
    s_.begin(b, SERIAL_8N1, rx_, tx_);
    baud_ = b;
    if (queryStatus() >= 0) break;
    s_.end();
    baud_ = 9600;
  }
  if (!s_) s_.begin(baud_, SERIAL_8N1, rx_, tx_);
  configure();
}

void EscPosPrinter::powerOn() {
  offPending_ = false;
  if (powered_) return;
  digitalWrite(en_, HIGH);
  delay(kWakeMs);
  powered_ = true;
}

void EscPosPrinter::configure() {
  const uint8_t init[] = {0x1B, 0x40};  // ESC @
  s_.write(init, sizeof init);
  // ESC 7 n1 n2 n3 (CSN-A2 manual p48): n1 = max heating dots in units of 8 (default 9 = 80 dots),
  // n2 = heating time, n3 = heating interval. Fewer dots heated at once lowers the peak current
  // at the cost of slower printing (D-039). The real peak current is a bench measurement.
  const uint8_t heat[] = {0x1B, 0x37, static_cast<uint8_t>(lowPower_ ? 3 : 9), 80, 2};
  s_.write(heat, sizeof heat);
}

void EscPosPrinter::setLowPower(bool on) {
  lowPower_ = on;
  if (on) {                         // battery: boost off until the next line
    if (powered_) { offPending_ = true; offAt_ = millis() + kHoldMs; }
  } else {                          // mains: always powered, so paper status can be polled
    powerOn();
    configure();
  }
}

void EscPosPrinter::poll() {
  if (offPending_ && static_cast<int32_t>(millis() - offAt_) >= 0) {
    digitalWrite(en_, LOW);
    powered_ = false;
    offPending_ = false;
  }
}

int EscPosPrinter::queryStatus() {
  while (s_.available()) s_.read();
  auto wait = [&](uint32_t ms) {
    const uint32_t deadline = millis() + ms;
    while (static_cast<int32_t>(deadline - millis()) > 0) { if (s_.available()) return s_.read(); delay(1); }
    return -1;
  };
  const uint8_t escv[] = {0x1B, 0x76, 0x00};  // ESC v 0
  s_.write(escv, sizeof escv);
  int b = wait(100);
  if (b == 'P') {                              // A2 ASCII form "P1V72T30"
    const int d = wait(20);
    while (wait(5) >= 0) {}
    if (d == '0' || d == '1') return d == '1';
  } else if (b >= 0) {
    return (b & 0x04) == 0;                    // CSN-A2 byte: bit 2 = paper out
  }
  const uint8_t dle[] = {0x10, 0x04, 0x04};   // DLE EOT 4: paper sensor status
  s_.write(dle, sizeof dle);
  b = wait(100);
  if (b >= 0) return (b & 0x60) == 0;
  return -1;
}

bool EscPosPrinter::paperPresent() {
  if (!powered_) return lastPaper_;           // boost off on battery: last known state
  const int st = queryStatus();
  if (st >= 0) { statusSeen_ = true; lastPaper_ = st == 1; }
  return statusSeen_ ? lastPaper_ : true;
}

bool EscPosPrinter::printLine(const std::string& utf8, seri::Language lang) {
  if (!powered_) { powerOn(); configure(); }
  bool ok = paperPresent();
  if (ok) {
    bool ascii = true;
    for (unsigned char c : utf8) if (c >= 0x80) { ascii = false; break; }
    if (ascii) {
      s_.print(utf8.c_str());
      s_.write('\n');
    } else {
      std::vector<uint8_t> bits;
      int wb = 0, h = 0;
      ok = raster_ && raster_(utf8, lang, bits, wb, h);
      if (ok) printRaster(bits, wb, h);
    }
  }
  if (lowPower_) { offPending_ = true; offAt_ = millis() + kHoldMs; }
  return ok;
}

void EscPosPrinter::printRaster(const std::vector<uint8_t>& bits, int widthBytes, int height) {
  const uint8_t hdr[] = {0x1D, 0x76, 0x30, 0x00, static_cast<uint8_t>(widthBytes & 0xFF), static_cast<uint8_t>(widthBytes >> 8),
                         static_cast<uint8_t>(height & 0xFF), static_cast<uint8_t>(height >> 8)};
  s_.write(hdr, sizeof hdr);
  s_.write(bits.data(), bits.size());
}
