#pragma once
// 58 mm TTL thermal panel printer, ESC/POS, 9600 8N1 on UART1.
// Paper status via DLE EOT 4 (Epson real-time status, paper sensor): bits 5-6 set = paper end.
// If the printer never answers (TX-only models), paper is assumed present (Q-14: choose a model
// with a status line, or add a paper switch).
// Indian-script lines are printed as raster images (GS v 0) built by the TextRenderer (phase 2, UI task).
#include <Arduino.h>
#include <functional>
#include "seri/hal.h"

class EscPosPrinter : public seri::hal::Printer {
 public:
  using Rasteriser = std::function<bool(const std::string& utf8, seri::Language l, std::vector<uint8_t>& bits, int& widthBytes, int& height)>;
  EscPosPrinter(HardwareSerial& s, int rx, int tx) : s_(s), rx_(rx), tx_(tx) {}
  void begin();
  void setRasteriser(Rasteriser r) { raster_ = std::move(r); }
  bool paperPresent() override;
  bool printLine(const std::string& utf8, seri::Language lang) override;
  void setLowPower(bool on) override;
  bool statusSupported() const { return statusSeen_; }
 private:
  void printRaster(const std::vector<uint8_t>& bits, int widthBytes, int height);
  HardwareSerial& s_;
  int rx_, tx_;
  bool statusSeen_ = false, lastPaper_ = true;
  Rasteriser raster_;
};
