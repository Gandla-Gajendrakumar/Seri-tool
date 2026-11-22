#include "seri/text_render.h"
#include <cstring>

namespace seri {
namespace {

const GlyphEntry* find(int cell, const std::string& text) {
  unsigned lo = 0, hi = atlas::kEntryCount;
  while (lo < hi) {  // entries are sorted by (cell, UTF-8 bytes)
    const unsigned mid = (lo + hi) / 2;
    const GlyphEntry& e = atlas::kEntries[mid];
    const int c = e.cell != cell ? (e.cell < cell ? -1 : 1) : std::strcmp(e.text, text.c_str());
    if (c == 0) return &e;
    if (c < 0) lo = mid + 1;
    else hi = mid;
  }
  return nullptr;
}

struct Piece { const GlyphEntry* g; int width; };  // g == nullptr: gap or missing-word box

// Splits a line into glyph pieces: Indian-script words are looked up whole; ASCII words per character.
std::vector<Piece> layout(const std::string& s, int cell, std::vector<std::string>* missing) {
  std::vector<Piece> out;
  const int space = cell / 4, gap = cell >= 32 ? 2 : 1;
  size_t i = 0;
  bool first = true;
  while (i <= s.size()) {
    size_t j = s.find(' ', i);
    if (j == std::string::npos) j = s.size();
    const std::string word = s.substr(i, j - i);
    if (!first) out.push_back({nullptr, space});
    first = false;
    bool ascii = true;
    for (unsigned char c : word) if (c >= 0x80) { ascii = false; break; }
    if (ascii) {
      for (char ch : word) {
        const GlyphEntry* g = find(cell, std::string(1, ch));
        if (g) { out.push_back({g, g->width}); out.push_back({nullptr, gap}); }
        else out.push_back({nullptr, cell / 3});
      }
    } else {
      // ASCII glued to a word ("(सही", "आवाज:") is drawn from the ASCII glyphs.
      size_t a = 0, z = word.size();
      while (a < z && static_cast<unsigned char>(word[a]) < 0x80) ++a;
      while (z > a && static_cast<unsigned char>(word[z - 1]) < 0x80) --z;
      auto asciiRun = [&](const std::string& run) {
        for (char ch : run) {
          const GlyphEntry* g = find(cell, std::string(1, ch));
          if (g) { out.push_back({g, g->width}); out.push_back({nullptr, gap}); }
        }
      };
      asciiRun(word.substr(0, a));
      const std::string core = word.substr(a, z - a);
      if (const GlyphEntry* g = find(cell, core)) {
        out.push_back({g, g->width});
      } else {
        if (missing) missing->push_back(core);
        out.push_back({nullptr, -cell});  // negative width: draw an empty box
      }
      if (z < word.size()) { out.push_back({nullptr, gap}); asciiRun(word.substr(z)); }
    }
    i = j + 1;
  }
  return out;
}

}  // namespace

int measureText(const std::string& utf8, int cell, std::vector<std::string>* missing) {
  int w = 0;
  for (const Piece& p : layout(utf8, cell, missing)) w += p.width < 0 ? -p.width : p.width;
  return w;
}

Bitmap renderText(const std::string& utf8, int cell, int widthPad, std::vector<std::string>* missing) {
  std::vector<Piece> pieces = layout(utf8, cell, missing);
  int natural = 0;
  for (const Piece& p : pieces) natural += p.width < 0 ? -p.width : p.width;
  Bitmap b;
  b.width = widthPad > 0 ? widthPad : natural;
  b.height = cell;
  b.bits.assign(static_cast<size_t>(b.widthBytes()) * cell, 0);
  auto set = [&b](int x, int y) {
    if (x >= 0 && x < b.width && y >= 0 && y < b.height) b.bits[y * b.widthBytes() + x / 8] |= 0x80 >> (x % 8);
  };
  int x = 0;
  for (const Piece& p : pieces) {
    if (p.g) {
      const int wb = (p.g->width + 7) / 8;
      const uint8_t* src = atlas::kBits + p.g->offset;
      for (int y = 0; y < cell; ++y)
        for (int gx = 0; gx < p.g->width; ++gx)
          if (src[y * wb + gx / 8] & (0x80 >> (gx % 8))) set(x + gx, y);
      x += p.width;
    } else if (p.width < 0) {  // missing word: hollow box so the gap is visible
      const int w = -p.width;
      for (int k = 2; k < w - 2; ++k) { set(x + k, 2); set(x + k, cell - 3); }
      for (int k = 2; k < cell - 2; ++k) { set(x + 2, k); set(x + w - 3, k); }
      x += w;
    } else {
      x += p.width;
    }
  }
  return b;
}

std::vector<std::string> wrapText(const std::string& utf8, int cell, int width) {
  std::vector<std::string> rows;
  std::string row;
  size_t i = 0;
  while (i <= utf8.size()) {
    size_t j = utf8.find(' ', i);
    if (j == std::string::npos) j = utf8.size();
    const std::string word = utf8.substr(i, j - i);
    if (!word.empty()) {
      const std::string candidate = row.empty() ? word : row + " " + word;
      if (row.empty() || measureText(candidate, cell) <= width) row = candidate;
      else { rows.push_back(row); row = word; }
    }
    i = j + 1;
  }
  if (!row.empty()) rows.push_back(row);
  return rows;
}

}  // namespace seri
