#pragma once
// 58 mm TTL thermal panel printer, ESC/POS on UART1, 9600 or 19200 8N1 (auto-detected at begin, D-041).
// Paper status: ESC v 0 (CSN-A2 manual v4.0 p40-41: one byte, bit 2 = paper out; the A2 variant answers
// with ASCII "P1V72T30", P0 = paper out), falling back to DLE EOT 4 (Epson: bits 5-6 = paper end).
// If the printer never answers (TX-only models), paper is assumed present (Q-14: choose a model
// with a status line, or add a paper switch).
// Power (D-046): the 5 V boost is switched by `powerEn`. On mains it stays on. On battery it is off
// between prints: turned on before a line, and off again kHoldMs after the last line (call poll()).
// Indian-script lines are printed as raster images (GS v 0) built by the TextRenderer (phase 2, UI task).
#include <Arduino.h>
#include <functional>
#include "seri/hal.h"

class EscPosPrinter : public seri::hal::Printer {
 public:
  using Rasteriser = std::function<bool(const std::string& utf8, seri::Language l, std::vector<uint8_t>& bits, int& widthBytes, int& height)>;
  EscPosPrinter(HardwareSerial& s, int rx, int tx, int powerEn) : s_(s), rx_(rx), tx_(tx), en_(powerEn) {}
  void begin();
  void poll();  // from loop(): switches the boost off once printing has finished on battery
  uint32_t baud() const { return baud_; }
  void setRasteriser(Rasteriser r) { raster_ = std::move(r); }
  bool paperPresent() override;
  bool printLine(const std::string& utf8, seri::Language lang) override;
  void setLowPower(bool on) override;
  bool statusSupported() const { return statusSeen_; }
 private:
  void printRaster(const std::vector<uint8_t>& bits, int widthBytes, int height);
  void powerOn();
  void configure();          // ESC @ + heating settings
  int queryStatus();         // 1 = paper, 0 = no paper, -1 = no answer
  static constexpr uint32_t kWakeMs = 500;   // ASSUMED printer start-up time after power-on: bench item
  static constexpr uint32_t kHoldMs = 4000;  // keep powered after the last line so it can finish
  HardwareSerial& s_;
  int rx_, tx_, en_;
  uint32_t baud_ = 9600, offAt_ = 0;
  bool statusSeen_ = false, lastPaper_ = true, lowPower_ = false, powered_ = false, offPending_ = false;
  Rasteriser raster_;
};
