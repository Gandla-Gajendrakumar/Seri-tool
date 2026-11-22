#pragma once
// Draws text lines as 1-bit bitmaps from the pre-shaped glyph atlas (tools/gen_glyphs.py).
// Used by the OLED driver (16 px lines) and for Indian-script printer lines (24 px raster).
#include <cstdint>
#include <string>
#include <vector>

namespace seri {

struct GlyphEntry {
  uint8_t cell;      // line height: 16 (OLED) or 32 (printer)
  uint16_t width;    // pixels
  uint32_t offset;   // into atlas::kBits, rows of (width + 7) / 8 bytes, `cell` rows
  const char* text;  // one word (Indian scripts) or one ASCII character
};
namespace atlas {
extern const GlyphEntry kEntries[];
extern const unsigned kEntryCount;
extern const uint8_t kBits[];
}  // namespace atlas

constexpr int kOledCell = 16, kPrintCell = 24;
constexpr int kOledWidth = 128, kPrintWidth = 384;

struct Bitmap {
  int width = 0, height = 0;
  std::vector<uint8_t> bits;  // row-major, MSB first, (width + 7) / 8 bytes per row
  int widthBytes() const { return (width + 7) / 8; }
  bool get(int x, int y) const { return (bits[y * widthBytes() + x / 8] >> (7 - x % 8)) & 1; }
};

// Width in pixels; words not in the atlas are added to `missing` and drawn as a box.
int measureText(const std::string& utf8, int cell, std::vector<std::string>* missing = nullptr);
// widthPad > 0: bitmap padded (or clipped) to that width, e.g. 384 for the printer.
Bitmap renderText(const std::string& utf8, int cell, int widthPad = 0, std::vector<std::string>* missing = nullptr);

}  // namespace seri
