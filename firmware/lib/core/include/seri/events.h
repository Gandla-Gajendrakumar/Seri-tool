#pragma once
// Event detection: turns samples and status inputs into start / clear events.
// Requirements F-EVT-01..07, F-MEAS-04. Pure logic; time is passed in (UTC seconds).
#include <cstdint>
#include <vector>
#include "seri/profiles.h"
#include "seri/types.h"

namespace seri {

enum class EventType : uint8_t {
  TempHigh = 0, TempLow, RhHigh, RhLow,                       // climate
  SensorFault, MainsFail, LowBattery, PaperOut, SdFault, ClockInvalid,  // system
  Count
};
constexpr int kEventTypeCount = static_cast<int>(EventType::Count);
inline bool isClimate(EventType t) { return static_cast<int>(t) <= static_cast<int>(EventType::RhLow); }

enum class Edge : uint8_t {
  Start,        // condition confirmed
  Clear,        // condition confirmed gone (a "recovery")
  SilentClear,  // cleared because the farmer changed stage: log + print only, no SMS (D-009)
};

struct Event {
  EventType type;
  Edge edge;
  int64_t time;
  Deci value;  // climate: the reading (deci); LowBattery: millivolts; others: 0
};

struct EngineConfig {
  int64_t climateOnsetSec = 300;  // 5 min continuously out of band (brief)
  int64_t climateClearSec = 30;   // 2 consecutive in-band samples (F-EVT-02)
  Deci tHyst = 5;                 // 0.5 C (brief)
  Deci rhHyst = 20;               // 2 %RH (F-EVT-02, assumed)
  int64_t sensorOnsetSec = 60;    // 3 consecutive invalid 30 s samples
  int64_t sensorClearSec = 60;    // 3 consecutive valid samples
  int64_t mainsOnsetSec = 120;    // flicker debounce (D-008)
  int64_t mainsClearSec = 120;
  int32_t battLowMv = 3700;       // D-038: alert while the modem can still send it (A7670 needs >= 3.4 V)
  int32_t battClearMv = 3850;
  int64_t battOnsetSec = 60;      // ignore short sags during modem bursts
  int64_t sdOnsetSec = 30;        // 2 consecutive failed checks 30 s apart
};

// Time-based debounce shared by every event type.
struct Debounce {
  int64_t onsetSec = 0, clearSec = 0;
  bool active = false;
  bool haveBad = false, haveGood = false;
  int64_t badSince = 0, goodSince = 0;
  // bad = condition present; good = condition clearly gone (may differ from !bad: hysteresis).
  // Returns +1 on start, -1 on clear, 0 otherwise.
  int update(int64_t now, bool bad, bool good);
  void resetPending() { haveBad = false; haveGood = false; }
};

class EventEngine {
 public:
  explicit EventEngine(const Band& band, const EngineConfig& cfg = EngineConfig{});

  void onSample(int64_t now, const Sample& s, std::vector<Event>& out);
  // Farmer changed stage or tolerance.
  void setBand(int64_t now, const Band& band, std::vector<Event>& out);
  void onMains(int64_t now, bool present, std::vector<Event>& out);
  void onBattery(int64_t now, int32_t millivolts, bool onBattery, std::vector<Event>& out);
  void onPaper(int64_t now, bool paperPresent, std::vector<Event>& out);
  void onSd(int64_t now, bool sdOk, std::vector<Event>& out);
  void onClock(int64_t now, bool clockValid, std::vector<Event>& out);

  bool active(EventType t) const { return deb_[static_cast<int>(t)].active; }
  const Band& band() const { return band_; }

 private:
  void step(EventType t, int64_t now, bool bad, bool good, Deci value, std::vector<Event>& out);
  Band band_;
  EngineConfig cfg_;
  Debounce deb_[kEventTypeCount];
  Sample last_{false, 0, 0};
};

}  // namespace seri
