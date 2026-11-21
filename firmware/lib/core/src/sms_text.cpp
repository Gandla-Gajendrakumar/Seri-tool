#include "seri/sms_text.h"
#include <cstdio>
#include "seri/time_util.h"

namespace seri {
namespace {

enum T {
  kTempHigh, kTempLow, kRhHigh, kRhLow, kSensorFault, kMainsFail, kLowBattery, kPaperOut, kSdFault,
  kClockInvalid,                                              // indexed by EventType
  kTempOk, kTempOk2, kRhOk, kRhOk2, kSensorOk, kMainsBack, kBatteryOk, kPaperOk, kSdOk, kClockOk,
  kStill, kFor, kMin, kHours, kSafe, kLimit, kTextCount
};

// ---------------------------------------------------------------------------------------
// WORDING. Indian-language text MUST be reviewed by a native speaker before field use
// (bench test plan item). Keep every phrase short: UCS-2 SMS holds only 70 characters.
// ---------------------------------------------------------------------------------------
const char* const kText[kLanguageCount][kTextCount] = {
    {// English
     "Temp HIGH", "Temp LOW", "Humidity HIGH", "Humidity LOW", "Sensor fault, check probe cable",
     "Power cut, on battery", "Battery low", "Printer paper finished", "Memory card fault",
     "Clock not set",
     "Temp normal again", "Temp normal again", "Humidity normal again", "Humidity normal again",
     "Sensor OK again", "Power back", "Battery OK", "Printer paper OK", "Memory card OK", "Clock set",
     "STILL", "for ", "min", "h", "safe",
     "SMS limit for today reached. Device keeps monitoring and printing."},
    {// Hindi
     "तापमान ज्यादा", "तापमान कम", "नमी ज्यादा", "नमी कम", "सेंसर खराब, प्रोब तार जाँचें",
     "बिजली गई, बैटरी पर", "बैटरी कम", "प्रिंटर कागज खत्म", "मेमोरी कार्ड खराब",
     "घड़ी सेट नहीं",
     "तापमान फिर ठीक", "तापमान फिर ठीक", "नमी फिर ठीक", "नमी फिर ठीक",
     "सेंसर फिर ठीक", "बिजली वापस आई", "बैटरी ठीक", "प्रिंटर कागज ठीक", "मेमोरी कार्ड ठीक", "घड़ी सेट",
     "अभी भी", "", "मि", "घं", "सही",
     "आज की SMS सीमा पूरी। निगरानी और प्रिंट जारी है।"},
    {// Telugu
     "ఉష్ణోగ్రత ఎక్కువ", "ఉష్ణోగ్రత తక్కువ", "తేమ ఎక్కువ", "తేమ తక్కువ", "సెన్సర్ లోపం, ప్రోబ్ వైర్ చూడండి",
     "కరెంట్ పోయింది, బ్యాటరీపై", "బ్యాటరీ తక్కువ", "ప్రింటర్ పేపర్ అయిపోయింది", "మెమరీ కార్డ్ లోపం",
     "గడియారం సెట్ కాలేదు",
     "ఉష్ణోగ్రత మళ్ళీ సరి", "ఉష్ణోగ్రత మళ్ళీ సరి", "తేమ మళ్ళీ సరి", "తేమ మళ్ళీ సరి",
     "సెన్సర్ మళ్ళీ సరి", "కరెంట్ వచ్చింది", "బ్యాటరీ సరి", "ప్రింటర్ పేపర్ సరి", "మెమరీ కార్డ్ సరి",
     "గడియారం సెట్ అయింది",
     "ఇంకా", "", "ని", "గం", "సరి",
     "ఈరోజు SMS పరిమితి పూర్తి. నిఘా, ప్రింట్ కొనసాగుతాయి."},
    {// Kannada
     "ತಾಪಮಾನ ಹೆಚ್ಚು", "ತಾಪಮಾನ ಕಡಿಮೆ", "ತೇವಾಂಶ ಹೆಚ್ಚು", "ತೇವಾಂಶ ಕಡಿಮೆ", "ಸೆನ್ಸರ್ ದೋಷ, ಪ್ರೋಬ್ ವೈರ್ ನೋಡಿ",
     "ಕರೆಂಟ್ ಹೋಗಿದೆ, ಬ್ಯಾಟರಿಯಲ್ಲಿ", "ಬ್ಯಾಟರಿ ಕಡಿಮೆ", "ಪ್ರಿಂಟರ್ ಪೇಪರ್ ಮುಗಿದಿದೆ", "ಮೆಮೊರಿ ಕಾರ್ಡ್ ದೋಷ",
     "ಗಡಿಯಾರ ಸೆಟ್ ಆಗಿಲ್ಲ",
     "ತಾಪಮಾನ ಮತ್ತೆ ಸರಿ", "ತಾಪಮಾನ ಮತ್ತೆ ಸರಿ", "ತೇವಾಂಶ ಮತ್ತೆ ಸರಿ", "ತೇವಾಂಶ ಮತ್ತೆ ಸರಿ",
     "ಸೆನ್ಸರ್ ಮತ್ತೆ ಸರಿ", "ಕರೆಂಟ್ ಬಂದಿದೆ", "ಬ್ಯಾಟರಿ ಸರಿ", "ಪ್ರಿಂಟರ್ ಪೇಪರ್ ಸರಿ", "ಮೆಮೊರಿ ಕಾರ್ಡ್ ಸರಿ",
     "ಗಡಿಯಾರ ಸೆಟ್ ಆಗಿದೆ",
     "ಇನ್ನೂ", "", "ನಿ", "ಗಂ", "ಸರಿ",
     "ಇಂದಿನ SMS ಮಿತಿ ಮುಗಿದಿದೆ. ನಿಗಾ, ಪ್ರಿಂಟ್ ಮುಂದುವರಿದಿದೆ."},
};

const char* const kStage[kLanguageCount][kStageCount] = {
    {"Incubation", "Chawki", "Instar 3", "Instar 4", "Instar 5"},
    {"अंडे", "चॉकी", "अवस्था 3", "अवस्था 4", "अवस्था 5"},
    {"గుడ్లు", "చాకీ", "దశ 3", "దశ 4", "దశ 5"},
    {"ಮೊಟ್ಟೆ", "ಚಾಕಿ", "ಹಂತ 3", "ಹಂತ 4", "ಹಂತ 5"},
};
const char* const kLangName[kLanguageCount] = {"English", "हिंदी", "తెలుగు", "ಕನ್ನಡ"};

const char* txt(Language l, int key) { return kText[static_cast<int>(l)][key]; }

std::string fmtDeci(Deci v) {  // 312 -> "31.2", 260 -> "26", -105 -> "-10.5"
  char b[16];
  const Deci a = v < 0 ? -v : v;
  if (a % 10 == 0) std::snprintf(b, sizeof b, "%s%ld", v < 0 ? "-" : "", static_cast<long>(a / 10));
  else std::snprintf(b, sizeof b, "%s%ld.%ld", v < 0 ? "-" : "", static_cast<long>(a / 10), static_cast<long>(a % 10));
  return b;
}
std::string fmtTemp(Deci v) {  // always one decimal for readings
  char b[16];
  const Deci a = v < 0 ? -v : v;
  std::snprintf(b, sizeof b, "%s%ld.%ldC", v < 0 ? "-" : "", static_cast<long>(a / 10), static_cast<long>(a % 10));
  return b;
}
std::string fmtRh(Deci v) { return std::to_string((v + 5) / 10) + "%"; }
std::string hhmm(int64_t utc) {
  CivilTime t = toIst(utc);
  char b[8];
  std::snprintf(b, sizeof b, "%02d:%02d", t.hour, t.minute);
  return b;
}
bool isTemp(EventType t) { return t == EventType::TempHigh || t == EventType::TempLow; }

void appendUtf8CodePoints(const std::string& s, std::vector<uint32_t>& out) {
  for (size_t i = 0; i < s.size();) {
    unsigned char c = static_cast<unsigned char>(s[i]);
    uint32_t cp; int n;
    if (c < 0x80) { cp = c; n = 1; }
    else if ((c >> 5) == 6) { cp = c & 0x1F; n = 2; }
    else if ((c >> 4) == 14) { cp = c & 0x0F; n = 3; }
    else { cp = c & 0x07; n = 4; }
    for (int k = 1; k < n && i + k < s.size(); ++k) cp = (cp << 6) | (static_cast<unsigned char>(s[i + k]) & 0x3F);
    out.push_back(cp);
    i += n;
  }
}

// GSM 03.38 default alphabet (basic table) and extension table characters.
const char* const kGsmBasic =
    "@£$¥èéùìòÇ\nØø\rÅåΔ_ΦΓΛΩΠΨΣΘΞÆæßÉ !\"#¤%&'()*+,-./0123456789:;<=>?"
    "¡ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÑÜ§¿abcdefghijklmnopqrstuvwxyzäöñüà";
const char* const kGsmExt = "\f^{}\\[~]|€";
bool inSet(uint32_t cp, const char* set) {
  std::vector<uint32_t> v;
  appendUtf8CodePoints(set, v);
  for (uint32_t x : v) if (x == cp) return true;
  return false;
}

}  // namespace

const char* stageName(Stage s, Language l) {
  return kStage[static_cast<int>(l)][static_cast<int>(s) % kStageCount];
}
const char* languageName(Language l) { return kLangName[static_cast<int>(l) % kLanguageCount]; }

bool deviceNameValid(const std::string& n) {
  if (n.empty() || n.size() > kMaxDeviceName) return false;
  for (char c : n)
    if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')) return false;
  return true;
}

SegmentInfo smsSegments(const std::string& utf8) {
  std::vector<uint32_t> cps;
  appendUtf8CodePoints(utf8, cps);
  int gsm = 0;
  bool ucs2 = false;
  for (uint32_t cp : cps) {
    if (inSet(cp, kGsmBasic)) gsm += 1;
    else if (inSet(cp, kGsmExt)) gsm += 2;
    else { ucs2 = true; break; }
  }
  SegmentInfo si{ucs2, 0, 0};
  if (!ucs2) {
    si.units = gsm;
    si.segments = gsm <= 160 ? 1 : (gsm + 152) / 153;
  } else {
    for (uint32_t cp : cps) si.units += cp > 0xFFFF ? 2 : 1;
    si.segments = si.units <= 70 ? 1 : (si.units + 66) / 67;
  }
  return si;
}

std::string formatSms(const SmsJob& j, const SmsContext& c) {
  const Language L = c.lang;
  const std::string head = c.deviceName + ": ";
  if (j.kind == SmsKind::LimitReached) return head + txt(L, kLimit);

  const int ti = static_cast<int>(j.type);
  const std::string time = hhmm(j.time);
  const bool climate = isClimate(j.type);
  auto val = [&](Deci v) { return isTemp(j.type) ? fmtTemp(v) : fmtRh(v); };
  auto dur = [&]() {
    const int64_t mins = (j.time - j.startedAt) / 60;
    if (mins < 120) return std::string(txt(L, kFor)) + std::to_string(mins) + txt(L, kMin);
    return std::string(txt(L, kFor)) + std::to_string(mins / 60) + txt(L, kHours);
  };
  const bool showBattery = j.type == EventType::MainsFail || j.type == EventType::LowBattery;
  const std::string batt = std::to_string(c.batteryPct) + "%";

  switch (j.kind) {
    case SmsKind::Start:
      if (climate) {
        std::string band = isTemp(j.type) ? fmtDeci(c.band.tLo) + "-" + fmtDeci(c.band.tHi) + "C"
                                          : fmtRh(c.band.rhLo).substr(0, fmtRh(c.band.rhLo).size() - 1) + "-" + fmtRh(c.band.rhHi);
        return head + txt(L, ti) + " " + val(j.value) + " (" + txt(L, kSafe) + " " + band + ") " +
               stageName(c.stage, L) + " " + time;
      }
      return head + txt(L, ti) + (showBattery ? " " + batt : "") + " " + time;
    case SmsKind::Repeat:
      if (climate) return head + txt(L, kStill) + " " + txt(L, ti) + " " + val(c.currentValue) + " " + dur() + " " + time;
      return head + txt(L, kStill) + " " + txt(L, ti) + (showBattery ? " " + batt : "") + " " + dur() + " " + time;
    case SmsKind::Recovery:
      if (climate) return head + txt(L, kTempOk + ti) + " " + val(j.value) + " " + time;
      return head + txt(L, kTempOk + ti) + " " + time;
    default:
      return head;
  }
}

}  // namespace seri
