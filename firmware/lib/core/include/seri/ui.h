#pragma once
// Local user interface: 1.3" OLED + 3 buttons (UP, OK, DOWN). Every setup step works without
// English, an app or a smartphone (F-UI-02): all text comes from the 4-language tables in ui_text.
// The UI only produces text lines; the OLED driver draws them (Indian scripts via the glyph atlas).
#include <string>
#include <vector>
#include "seri/monitor.h"

namespace seri {

enum class Button : uint8_t { Up, Ok, Down };

struct Screen {
  std::vector<std::string> lines;  // 4 lines of 16 px on 128x64
  int highlight = -1;              // menu line drawn inverted
  bool bigFirstLine = false;       // home screen: reading in a large font
};

struct UiStatus {
  bool network = false;  // cellular registered
  bool wifi = false;
  int64_t now = 0;
};

enum UiText {
  kUiAllOk, kUiMenu, kUiStage, kUiLanguage, kUiRange, kUiPhones, kUiBuzzer, kUiPrintTest, kUiBack, kUiOn, kUiOff,
  kUiSaved, kUiOwner, kUiNumber, kUiChange, kUiRemove, kUiEmpty, kUiWrongNumber, kUiMains, kUiBattery, kUiNoSignal,
  kUiSilenced, kUiPrinterOk, kUiTextCount
};
const char* uiText(UiText t, Language l);

class Ui {
 public:
  explicit Ui(Monitor& m) : m_(m) {}
  void press(Button b, int64_t now);
  // `now` for press/tick is a wall-clock seconds counter (millis()/1000 on the device), not the
  // device clock, which runs 60x fast in simulation.
  void tick(int64_t now);  // returns to the home screen after 60 s without a press
  Screen render(const UiStatus& st) const;
  Language lang() const { return m_.settings().lang[0]; }

  enum class Page : uint8_t { Home, Menu, Stage, Language, RangeT, RangeRh, Phones, PhoneSlot, PhoneEdit, Message };
  Page page() const { return page_; }

 private:
  void go(Page p, int cursor = 0);
  void message(UiText t, int64_t now);
  Monitor& m_;
  Page page_ = Page::Home;
  int cursor_ = 0;
  int slot_ = 0;
  std::string digits_;
  int digitPos_ = 0;
  Tolerance editTol_;
  UiText msg_ = kUiSaved;
  int64_t lastPress_ = 0, msgUntil_ = 0;
};

}  // namespace seri
