#pragma once
// Thermal-printer content (F-PRN-01..03) and the daily summary.
// 58 mm printer, 384 dots, font A 12x24 = 32 columns. English lines must fit 32 columns.
// Indian-language lines are rasterised to a bitmap later (phase 2), where their pixel width is
// checked; cheap ESC/POS printers have no Indian fonts (Q-4).
#include <string>
#include <vector>
#include "seri/events.h"
#include "seri/profiles.h"

namespace seri {

constexpr int kPrintCols = 32;

// One line per event: "21/11 14:05 TEMP HIGH 31.2C"
std::string printEventLine(const Event& e, Language l);
// Printed just before any silent clears caused by the change: "21/11 09:00 STAGE -> Instar 4"
std::string printStageLine(int64_t now, Stage s, Language l);

struct DayReport {
  int64_t from = 0, to = 0;          // UTC period covered
  int samples = 0, invalidSamples = 0;
  Deci tMin = 0, tMax = 0, rhMin = 0, rhMax = 0;
  int64_t tMinAt = 0, tMaxAt = 0;
  int64_t tOutSec = 0, rhOutSec = 0, noDataSec = 0;
  int alerts = 0;                    // climate + system event starts
};

class DailySummary {
 public:
  explicit DailySummary(int64_t periodStart) { rep_.from = periodStart; }
  void addSample(int64_t now, const Sample& s, const Band& b);
  void addEvent(const Event& e);
  DayReport close(int64_t now);  // returns the report and starts a new period at `now`
  int64_t periodStart() const { return rep_.from; }
 private:
  DayReport rep_;
  bool havePrev_ = false;
  int64_t prev_ = 0;
};

std::vector<std::string> printSummary(const DayReport& r, Stage s, const Band& b, Language l, bool late);

// Decides when the daily summary prints (default 06:00 IST). On battery it is held and printed,
// marked late, when mains returns (F-PRN-03). Up to 7 held summaries are kept.
class SummaryScheduler {
 public:
  explicit SummaryScheduler(int summaryHourIst = 6) : hour_(summaryHourIst) {}
  struct Due { DayReport report; bool late; };
  std::vector<Due> tick(int64_t now, bool onBattery, DailySummary& acc);
 private:
  int hour_;
  int64_t nextDue_ = -1;
  std::vector<DayReport> held_;
};

}  // namespace seri
