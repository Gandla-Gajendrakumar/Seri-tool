#include "oled.h"
#include "seri/text_render.h"

void Oled::begin() {
  u8g2_.setI2CAddress(0x3C << 1);
  u8g2_.begin();
  u8g2_.setBusClock(100000);
}

void Oled::show(const seri::Screen& s) {
  u8g2_.clearBuffer();
  for (size_t i = 0; i < s.lines.size() && i < 4; ++i) {
    const int y = static_cast<int>(i) * seri::kOledCell;
    if (i == 0 && s.bigFirstLine) {  // the reading: always ASCII, bold font
      u8g2_.setFont(u8g2_font_helvB12_tr);
      u8g2_.drawStr(0, y + 14, s.lines[0].c_str());
    } else {
      seri::Bitmap b = seri::renderText(s.lines[i], seri::kOledCell);
      u8g2_.drawBitmap(0, y, static_cast<u8g2_uint_t>(b.widthBytes()), static_cast<u8g2_uint_t>(b.height), b.bits.data());
    }
    if (static_cast<int>(i) == s.highlight) {
      u8g2_.setDrawColor(2);  // XOR: invert the selected line
      u8g2_.drawBox(0, y, 128, seri::kOledCell);
      u8g2_.setDrawColor(1);
    }
  }
  u8g2_.sendBuffer();
}
