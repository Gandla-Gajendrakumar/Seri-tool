#pragma once
// SMS wording in English, Hindi, Telugu, Kannada (F-SMS-02/03, decision D-007).
// Every message must fit ONE SMS segment: 160 GSM-7 characters (English) or 70 UCS-2
// characters (Indian scripts). Numbers always use Western digits 0-9.
#include <string>
#include "seri/alerts.h"
#include "seri/profiles.h"

namespace seri {

struct SmsContext {
  std::string deviceName;  // <= 10 ASCII characters, e.g. "Shed-1"
  Stage stage;
  Band band;
  Language lang;
  int64_t now;             // UTC seconds
  Deci currentValue;       // latest reading for the event's quantity (used by repeats)
  int batteryPct;          // 0-100
};

std::string formatSms(const SmsJob& job, const SmsContext& ctx);
const char* stageName(Stage s, Language l);
const char* languageName(Language l);  // in its own script, for the language menu
// Short phrase for an event ("Temp HIGH" / ok=true: "Temp normal again") in a language.
const char* eventPhrase(EventType t, bool ok, Language l);
// Short form for one OLED line or one printed line: the phrase up to its first comma.
std::string shortEventPhrase(EventType t, bool ok, Language l);

struct SegmentInfo {
  bool ucs2;     // false = GSM-7
  int units;     // characters (GSM-7, counting escapes as 2) or UTF-16 code units
  int segments;  // number of SMS parts needed
};
SegmentInfo smsSegments(const std::string& utf8);

// After a network outage in which alerts had to be given up (F-SMS-06), one text per number:
// "Shed-1: No signal 07:00-19:00. 2 alerts not sent. Now 27.5C 85%"  (decision D-037)
std::string formatOfflineSummary(const std::string& device, Language l, int64_t from, int64_t to, int missed,
                                 const Sample& now);

constexpr int kMaxDeviceName = 10;
bool deviceNameValid(const std::string& name);

}  // namespace seri
