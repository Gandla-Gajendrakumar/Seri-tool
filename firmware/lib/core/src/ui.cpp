#include "seri/ui.h"
#include "fmt.h"
#include "seri/sms_text.h"

namespace seri {
namespace {

// ---------------------------------------------------------------------------------------
// UI WORDING (native-speaker review required, like the SMS tables). Short words: the OLED
// line is 128 px wide; the glyph-atlas test checks every string fits.
// ---------------------------------------------------------------------------------------
const char* const kUi[kLanguageCount][kUiTextCount] = {
    {"All OK", "Menu", "Stage", "Language", "Range +/-", "Phone numbers", "Buzzer", "Print test", "Back", "On", "Off",
     "Saved", "Owner", "Number", "Change", "Remove", "Empty", "Wrong number", "Mains", "Battery", "No signal",
     "Alarm silenced", "Printer test OK"},
    {"सब ठीक", "मेनू", "अवस्था", "भाषा", "सीमा +/-", "फोन नंबर", "आवाज", "प्रिंट जाँच", "वापस", "चालू", "बंद",
     "सेव हुआ", "मालिक", "नंबर", "बदलें", "हटाएँ", "खाली", "गलत नंबर", "बिजली", "बैटरी", "सिग्नल नहीं",
     "अलार्म बंद", "प्रिंटर ठीक है"},
    {"అన్నీ సరి", "మెనూ", "దశ", "భాష", "పరిధి +/-", "ఫోన్ నంబర్లు", "శబ్దం", "ప్రింట్ పరీక్ష", "వెనుకకు", "ఆన్", "ఆఫ్",
     "సేవ్ అయింది", "యజమాని", "నంబర్", "మార్చు", "తీసివేయి", "ఖాళీ", "తప్పు నంబర్", "కరెంట్", "బ్యాటరీ", "సిగ్నల్ లేదు",
     "అలారం ఆపారు", "ప్రింటర్ సరి"},
    {"ಎಲ್ಲಾ ಸರಿ", "ಮೆನು", "ಹಂತ", "ಭಾಷೆ", "ಮಿತಿ +/-", "ಫೋನ್ ನಂಬರ್", "ಶಬ್ದ", "ಪ್ರಿಂಟ್ ಪರೀಕ್ಷೆ", "ಹಿಂದೆ", "ಆನ್", "ಆಫ್",
     "ಉಳಿಸಲಾಗಿದೆ", "ಮಾಲೀಕ", "ನಂಬರ್", "ಬದಲಿಸಿ", "ತೆಗೆಯಿರಿ", "ಖಾಲಿ", "ತಪ್ಪು ನಂಬರ್", "ಕರೆಂಟ್", "ಬ್ಯಾಟರಿ", "ಸಿಗ್ನಲ್ ಇಲ್ಲ",
     "ಅಲಾರಂ ನಿಲ್ಲಿಸಿದೆ", "ಪ್ರಿಂಟರ್ ಸರಿ"},
};

constexpr int kMenuItems = 7;  // Stage, Language, Range, Phones, Buzzer, Print test, Back
constexpr int64_t kIdleSec = 60, kMessageSec = 3;

// Shows a scrolling window of up to 4 items with the cursor highlighted.
Screen listScreen(const std::vector<std::string>& items, int cursor) {
  Screen s;
  const int n = static_cast<int>(items.size());
  int first = cursor < 4 ? 0 : cursor - 3;
  if (first > n - 4) first = n > 4 ? n - 4 : 0;
  for (int i = first; i < n && i < first + 4; ++i) s.lines.push_back(items[i]);
  s.highlight = cursor - first;
  return s;
}
int wrap(int v, int n) { return ((v % n) + n) % n; }

}  // namespace

const char* uiText(UiText t, Language l) { return kUi[static_cast<int>(l) % kLanguageCount][t]; }


void Ui::go(Page p, int cursor) {
  page_ = p;
  cursor_ = cursor;
}

void Ui::message(UiText t, int64_t now) {
  msg_ = t;
  msgUntil_ = now + kMessageSec;
  page_ = Page::Message;
}

void Ui::tick(int64_t now) {
  uiNow_ = now;
  if (page_ == Page::Message && now >= msgUntil_) go(Page::Home);
  else if (page_ != Page::Home && page_ != Page::Message && now - lastPress_ > kIdleSec) go(Page::Home);
}

void Ui::press(Button b, int64_t now) {
  lastPress_ = now;
  uiNow_ = now;
  const int step = b == Button::Down ? 1 : (b == Button::Up ? -1 : 0);
  Settings s = m_.settings();
  switch (page_) {
    case Page::Home:
    case Page::Message:
      if (b != Button::Ok) return;
      if (m_.buzzing()) { m_.silenceBuzzer(); message(kUiSilenced, now); return; }
      go(Page::Menu);
      return;
    case Page::Menu:
      if (step) { cursor_ = wrap(cursor_ + step, kMenuItems); return; }
      switch (cursor_) {
        case 0: go(Page::Stage, static_cast<int>(s.stage)); return;
        case 1: go(Page::Language, static_cast<int>(s.lang[0])); return;
        case 2: editTol_ = s.tol; go(Page::RangeT); return;
        case 3: go(Page::Phones); return;
        case 4: s.buzzerOn = !s.buzzerOn; m_.changeSettings(s); return;
        case 5: m_.printText(uiText(kUiPrinterOk, s.lang[0])); return;
        default: go(Page::Home); return;
      }
    case Page::Stage:
      if (step) { cursor_ = wrap(cursor_ + step, kStageCount); return; }
      s.stage = static_cast<Stage>(cursor_);
      m_.changeSettings(s);
      message(kUiSaved, now);
      return;
    case Page::Language:
      if (step) { cursor_ = wrap(cursor_ + step, kLanguageCount); return; }
      s.lang[0] = static_cast<Language>(cursor_);
      m_.changeSettings(s);
      message(kUiSaved, now);
      return;
    case Page::RangeT:  // UP = wider; 0.5 C steps between 0.5 and 3.0
      if (b == Button::Up && editTol_.t < 30) editTol_.t += 5;
      else if (b == Button::Down && editTol_.t > 5) editTol_.t -= 5;
      else if (b == Button::Ok) go(Page::RangeRh);
      return;
    case Page::RangeRh:  // 1 % steps between 2 and 15
      if (b == Button::Up && editTol_.rh < 150) editTol_.rh += 10;
      else if (b == Button::Down && editTol_.rh > 20) editTol_.rh -= 10;
      else if (b == Button::Ok) { s.tol = editTol_; m_.changeSettings(s); message(kUiSaved, now); }
      return;
    case Page::Phones:  // 3 slots + Back
      if (step) { cursor_ = wrap(cursor_ + step, kMaxRecipients + 1); return; }
      if (cursor_ == kMaxRecipients) { go(Page::Menu, 3); return; }
      slot_ = cursor_;
      go(Page::PhoneSlot);
      return;
    case Page::PhoneSlot: {  // Change [, Remove], Back
      const int n = slot_ == 0 ? 2 : 3;
      if (step) { cursor_ = wrap(cursor_ + step, n); return; }
      if (cursor_ == 0) {
        digits_ = s.phone[slot_].empty() ? "6000000000" : s.phone[slot_];
        digitPos_ = 0;
        go(Page::PhoneEdit);
      } else if (cursor_ == 1 && slot_ != 0) {
        s.phone[slot_].clear();
        m_.changeSettings(s);
        message(kUiSaved, now);
      } else {
        go(Page::Phones, slot_);
      }
      return;
    }
    case Page::PhoneEdit: {
      char& d = digits_[digitPos_];
      const int lo = digitPos_ == 0 ? 6 : 0;  // Indian mobiles start with 6-9
      if (b == Button::Up) d = static_cast<char>(d >= '9' ? '0' + lo : d + 1);
      else if (b == Button::Down) d = static_cast<char>(d <= '0' + lo ? '9' : d - 1);
      else if (++digitPos_ == 10) {
        if (normalizeNumber(digits_).empty()) { message(kUiWrongNumber, now); return; }
        for (int i = 0; i < kMaxRecipients; ++i) if (i != slot_ && s.phone[i] == digits_) s.phone[i].clear();
        s.phone[slot_] = digits_;
        if (slot_ > 0 && s.lang[slot_] == Language::English) s.lang[slot_] = s.lang[0];
        m_.changeSettings(s);
        message(kUiSaved, now);
      }
      return;
    }
  }
}

std::vector<std::string> Ui::listItems() const {
  const Language L = lang();
  const Settings& s = m_.settings();
  auto T = [L](UiText t) { return std::string(uiText(t, L)); };
  std::vector<std::string> items;
  switch (page_) {
    case Page::Menu:
      return {T(kUiStage), T(kUiLanguage), T(kUiRange), T(kUiPhones),
              T(kUiBuzzer) + ": " + T(s.buzzerOn ? kUiOn : kUiOff), T(kUiPrintTest), T(kUiBack)};
    case Page::Stage:
      for (int i = 0; i < kStageCount; ++i) items.push_back(stageName(static_cast<Stage>(i), L));
      return items;
    case Page::Language:
      for (int i = 0; i < kLanguageCount; ++i) items.push_back(languageName(static_cast<Language>(i)));
      return items;
    case Page::Phones:
      for (int i = 0; i < kMaxRecipients; ++i)
        items.push_back(std::to_string(i + 1) + " " + (s.phone[i].empty() ? T(kUiEmpty) : s.phone[i]));
      items.push_back(T(kUiBack));
      return items;
    case Page::PhoneSlot:
      items.push_back(T(kUiChange));
      if (slot_ != 0) items.push_back(T(kUiRemove));
      items.push_back(T(kUiBack));
      return items;
    default:
      return items;
  }
}

Screen Ui::render(const UiStatus& st) const {
  const Language L = lang();
  const bool large = L != Language::English && page_ != Page::PhoneEdit;  // D-033
  const Settings& s = m_.settings();
  auto T = [L](UiText t) { return std::string(uiText(t, L)); };
  Screen sc;
  switch (page_) {
    case Page::Home: {
      const Sample& x = m_.lastSample();
      const std::string reading = x.valid ? fmt::temp(x.t) + " " + fmt::rh(x.rh) : "--.-C --%";
      const std::string link = st.wifi ? "WiFi" : (st.network ? "4G" : "4G x");
      const std::string stage = std::string(stageName(s.stage, L)) + " " + link;
      std::string status = T(kUiAllOk);
      for (int i = 0; i < kEventTypeCount; ++i)
        if (m_.active(static_cast<EventType>(i))) { status = shortEventPhrase(static_cast<EventType>(i), false, L); break; }
      const std::string power = m_.onBattery() ? T(kUiBattery) + " " + std::to_string(m_.batteryPct()) + "%" : T(kUiMains);
      if (large) {  // one item at a time, changing every 3 s
        sc.large = true;
        sc.header = reading + " " + fmt::hhmm(st.now);
        const std::string items[3] = {status, stage, power};
        sc.body = items[(uiNow_ / 3) % 3];
        return sc;
      }
      sc.bigFirstLine = true;
      sc.lines = {reading, stage, status, power + " " + fmt::hhmm(st.now)};
      return sc;
    }
    case Page::Message:
      if (large) { sc.large = true; sc.body = T(msg_); return sc; }
      sc.lines = {"", T(msg_), "", ""};
      return sc;
    case Page::RangeT:
    case Page::RangeRh: {
      const std::string tval = "+/-" + fmt::signedTenths(editTol_.t, true) + "C", rval = "+/-" + fmt::rh(editTol_.rh);
      if (large) {
        sc.large = true;
        sc.header = page_ == Page::RangeT ? "1/2" : "2/2";
        sc.body = page_ == Page::RangeT ? tval : rval;
        return sc;
      }
      sc.lines = {T(kUiRange), tval, rval, ""};
      sc.highlight = page_ == Page::RangeT ? 1 : 2;
      return sc;
    }
    case Page::PhoneEdit: {
      std::string marker(10, ' ');
      marker[digitPos_] = '^';
      sc.lines = {T(slot_ == 0 ? kUiOwner : kUiNumber), digits_, marker, ""};
      return sc;
    }
    default: {
      const std::vector<std::string> items = listItems();
      if (large) {
        sc.large = true;
        sc.header = std::to_string(cursor_ + 1) + "/" + std::to_string(items.size());
        sc.body = items.empty() ? "" : items[cursor_];
        return sc;
      }
      return listScreen(items, cursor_);
    }
  }
}

}  // namespace seri
