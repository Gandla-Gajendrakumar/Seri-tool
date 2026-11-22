#include "seri/print.h"
#include <algorithm>
#include <cstdio>
#include "fmt.h"
#include "seri/sms_text.h"

namespace seri {
namespace {

// English printer labels are short to fit 32 columns. Indian languages reuse the SMS phrases.
const char* const kEnStart[kEventTypeCount] = {"TEMP HIGH", "TEMP LOW", "RH HIGH", "RH LOW", "SENSOR FAULT",
                                               "POWER CUT", "BATTERY LOW", "PAPER OUT", "SD CARD FAULT", "CLOCK NOT SET"};
const char* const kEnOk[kEventTypeCount] = {"TEMP OK", "TEMP OK", "RH OK", "RH OK", "SENSOR OK",
                                            "POWER BACK", "BATTERY OK", "PAPER OK", "SD CARD OK", "CLOCK SET"};

enum W { kTitle, kTemp, kMin, kMax, kHum, kOut, kNoData, kAlerts, kLate, kStageW, kWordCount };
const char* const kWords[kLanguageCount][kWordCount] = {
    {"DAILY SUMMARY", "Temp", "min", "max", "Humidity", "out of range", "No sensor data", "Alerts",
     "PRINTED LATE (POWER CUT)", "STAGE"},
    {"दैनिक सारांश", "तापमान", "न्यूनतम", "अधिकतम", "नमी", "सीमा से बाहर", "सेंसर डेटा नहीं", "चेतावनियाँ",
     "देर से छपा (बिजली कटौती)", "अवस्था"},
    {"రోజువారీ సారాంశం", "ఉష్ణోగ్రత", "కనిష్ఠ", "గరిష్ఠ", "తేమ", "పరిధి బయట", "సెన్సర్ డేటా లేదు", "హెచ్చరికలు",
     "ఆలస్యంగా ముద్రణ (కరెంట్ కోత)", "దశ"},
    {"ದೈನಂದಿನ ಸಾರಾಂಶ", "ತಾಪಮಾನ", "ಕನಿಷ್ಠ", "ಗರಿಷ್ಠ", "ತೇವಾಂಶ", "ಮಿತಿ ಹೊರಗೆ", "ಸೆನ್ಸರ್ ಡೇಟಾ ಇಲ್ಲ", "ಎಚ್ಚರಿಕೆಗಳು",
     "ತಡವಾಗಿ ಮುದ್ರಣ (ಕರೆಂಟ್ ಕಡಿತ)", "ಹಂತ"},
};
std::string w(Language l, int k) { return kWords[static_cast<int>(l)][k]; }

std::string stamp(int64_t utc, bool withDate = true) { return withDate ? fmt::ddmmHhmm(utc) : fmt::hhmm(utc); }
std::string temp(Deci v) { return fmt::temp(v); }
std::string rh(Deci v) { return fmt::rh(v); }
std::string bandT(Deci v) { return fmt::bandTemp(v); }
std::string hm(int64_t sec) {
  char b[32];
  std::snprintf(b, sizeof b, "%ldh%02ldm", static_cast<long>(sec / 3600), static_cast<long>((sec % 3600) / 60));
  return b;
}
std::string volts(int32_t mv) {
  if (mv < 0) mv = 0;
  char b[32];
  std::snprintf(b, sizeof b, "%ld.%02ldV", static_cast<long>(mv / 1000), static_cast<long>((mv % 1000) / 10));
  return b;
}

}  // namespace

std::string printEventLine(const Event& e, Language l) {
  const bool ok = e.edge != Edge::Start;
  const int ti = static_cast<int>(e.type);
  std::string label = l == Language::English ? (ok ? kEnOk[ti] : kEnStart[ti]) : shortEventPhrase(e.type, ok, l);
  std::string line = stamp(e.time) + " " + label;
  if (e.type == EventType::TempHigh || e.type == EventType::TempLow) line += " " + temp(e.value);
  else if (e.type == EventType::RhHigh || e.type == EventType::RhLow) line += " " + rh(e.value);
  else if (e.type == EventType::LowBattery && !ok) line += " " + volts(e.value);
  return line;
}

std::string printStageLine(int64_t now, Stage s, Language l) {
  return stamp(now) + " " + w(l, kStageW) + " -> " + stageName(s, l);
}

void DailySummary::addSample(int64_t now, const Sample& s, const Band& b) {
  const int64_t dt = havePrev_ ? std::min<int64_t>(now - prev_, 60) : 30;
  havePrev_ = true;
  prev_ = now;
  ++rep_.samples;
  if (!s.valid) {
    ++rep_.invalidSamples;
    rep_.noDataSec += dt;
    return;
  }
  if (rep_.samples - rep_.invalidSamples == 1) {
    rep_.tMin = rep_.tMax = s.t;
    rep_.rhMin = rep_.rhMax = s.rh;
    rep_.tMinAt = rep_.tMaxAt = now;
  } else {
    if (s.t < rep_.tMin) { rep_.tMin = s.t; rep_.tMinAt = now; }
    if (s.t > rep_.tMax) { rep_.tMax = s.t; rep_.tMaxAt = now; }
    rep_.rhMin = std::min(rep_.rhMin, s.rh);
    rep_.rhMax = std::max(rep_.rhMax, s.rh);
  }
  if (s.t > b.tHi || s.t < b.tLo) rep_.tOutSec += dt;
  if (s.rh > b.rhHi || s.rh < b.rhLo) rep_.rhOutSec += dt;
}

void DailySummary::addEvent(const Event& e) {
  if (e.edge == Edge::Start) ++rep_.alerts;
}

DayReport DailySummary::close(int64_t now) {
  DayReport r = rep_;
  r.to = now;
  rep_ = DayReport{};
  rep_.from = now;
  return r;
}

std::vector<std::string> printSummary(const DayReport& r, Stage s, const Band& b, Language l, bool late) {
  std::vector<std::string> out;
  const bool have = r.samples - r.invalidSamples > 0;
  out.push_back("======= " + w(l, kTitle) + " =======");
  out.push_back(stamp(r.from) + " - " + stamp(r.to));
  out.push_back(std::string(stageName(s, l)) + " " + bandT(b.tLo) + "-" + bandT(b.tHi) + "C " +
                fmt::rhInt(b.rhLo) + "-" + rh(b.rhHi));
  out.push_back(w(l, kTemp) + " " + w(l, kMin) + " " + (have ? temp(r.tMin) + " " + stamp(r.tMinAt, false) : "--"));
  out.push_back(w(l, kTemp) + " " + w(l, kMax) + " " + (have ? temp(r.tMax) + " " + stamp(r.tMaxAt, false) : "--"));
  out.push_back(w(l, kHum) + " " + (have ? rh(r.rhMin) + " - " + rh(r.rhMax) : "--"));
  out.push_back(w(l, kTemp) + " " + w(l, kOut) + " " + hm(r.tOutSec));
  out.push_back(w(l, kHum) + " " + w(l, kOut) + " " + hm(r.rhOutSec));
  if (r.noDataSec > 0) out.push_back(w(l, kNoData) + " " + hm(r.noDataSec));
  out.push_back(w(l, kAlerts) + " " + std::to_string(r.alerts));
  if (late) out.push_back(w(l, kLate));
  out.push_back(std::string(kPrintCols, '-'));
  return out;
}

std::vector<SummaryScheduler::Due> SummaryScheduler::tick(int64_t now, bool onBattery, DailySummary& acc) {
  auto nextAfter = [this](int64_t t) {
    CivilTime c = toIst(t);
    int64_t cand = fromIst(CivilTime{c.year, c.month, c.day, hour_, 0, 0});
    return cand <= t ? cand + 86400 : cand;
  };
  if (nextDue_ < 0) nextDue_ = nextAfter(acc.periodStart());
  std::vector<Due> out;
  if (now >= nextDue_) {
    DayReport r = acc.close(now);
    nextDue_ = nextAfter(now);
    held_.push_back(r);
    if (held_.size() > 7) held_.erase(held_.begin());
    if (onBattery) return out;
    for (size_t i = 0; i < held_.size(); ++i) out.push_back(Due{held_[i], i + 1 < held_.size()});
    held_.clear();
    return out;
  }
  if (!onBattery && !held_.empty()) {
    for (auto& r : held_) out.push_back(Due{r, true});
    held_.clear();
  }
  return out;
}

}  // namespace seri
