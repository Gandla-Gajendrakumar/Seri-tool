#pragma once
// 1.3" 128x64 OLED. Real 1.3" modules use the SH1106 controller; Wokwi only simulates the SSD1306
// (same 128x64 page layout, 2-column offset), so the simulation build drives an SSD1306.
// Lines come from the UI; text is drawn from the glyph atlas so Indian scripts appear correctly.
#include <U8g2lib.h>
#include "seri/ui.h"

class Oled {
 public:
  void begin();
  void show(const seri::Screen& s);
  void setPower(bool on) { if (on != on_) { u8g2_.setPowerSave(on ? 0 : 1); on_ = on; } }
 private:
  bool on_ = true;
#ifdef SERI_SIM
  U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2_{U8G2_R0, U8X8_PIN_NONE};
#else
  U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2_{U8G2_R0, U8X8_PIN_NONE};
#endif
};
