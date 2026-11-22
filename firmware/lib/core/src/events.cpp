#include "seri/events.h"

namespace seri {

int Debounce::update(int64_t now, bool bad, bool good) {
  if (!active) {
    haveGood = false;
    if (!bad) { haveBad = false; return 0; }
    if (!haveBad) { haveBad = true; badSince = now; }
    if (now - badSince >= onsetSec) { active = true; haveBad = false; return +1; }
    return 0;
  }
  haveBad = false;
  if (!good) { haveGood = false; return 0; }
  if (!haveGood) { haveGood = true; goodSince = now; }
  if (now - goodSince >= clearSec) { active = false; haveGood = false; return -1; }
  return 0;
}

EventEngine::EventEngine(const Band& band, const EngineConfig& cfg) : band_(band), cfg_(cfg) {
  auto set = [this](EventType t, int64_t on, int64_t off) {
    deb_[static_cast<int>(t)].onsetSec = on;
    deb_[static_cast<int>(t)].clearSec = off;
  };
  for (EventType t : {EventType::TempHigh, EventType::TempLow, EventType::RhHigh, EventType::RhLow})
    set(t, cfg_.climateOnsetSec, cfg_.climateClearSec);
  set(EventType::SensorFault, cfg_.sensorOnsetSec, cfg_.sensorClearSec);
  set(EventType::MainsFail, cfg_.mainsOnsetSec, cfg_.mainsClearSec);
  set(EventType::LowBattery, cfg_.battOnsetSec, 0);
  set(EventType::PaperOut, 0, 0);
  set(EventType::SdFault, cfg_.sdOnsetSec, 0);
  set(EventType::ClockInvalid, 0, 0);
}

void EventEngine::step(EventType t, int64_t now, bool bad, bool good, Deci value,
                       std::vector<Event>& out) {
  int r = deb_[static_cast<int>(t)].update(now, bad, good);
  if (r > 0) out.push_back(Event{t, Edge::Start, now, value});
  if (r < 0) out.push_back(Event{t, Edge::Clear, now, value});
}

void EventEngine::onSample(int64_t now, const Sample& s, std::vector<Event>& out) {
  step(EventType::SensorFault, now, !s.valid, s.valid, 0, out);
  if (!s.valid) return;  // an invalid sample neither confirms nor resets climate conditions
  last_ = s;
  const Band& b = band_;
  step(EventType::TempHigh, now, s.t > b.tHi, s.t <= b.tHi - cfg_.tHyst, s.t, out);
  step(EventType::TempLow, now, s.t < b.tLo, s.t >= b.tLo + cfg_.tHyst, s.t, out);
  step(EventType::RhHigh, now, s.rh > b.rhHi, s.rh <= b.rhHi - cfg_.rhHyst, s.rh, out);
  step(EventType::RhLow, now, s.rh < b.rhLo, s.rh >= b.rhLo + cfg_.rhHyst, s.rh, out);
}

void EventEngine::setBand(int64_t now, const Band& band, std::vector<Event>& out) {
  band_ = band;
  struct C { EventType t; bool stillBad; Deci v; };
  const C cs[] = {
      {EventType::TempHigh, last_.t > band.tHi, last_.t},
      {EventType::TempLow, last_.t < band.tLo, last_.t},
      {EventType::RhHigh, last_.rh > band.rhHi, last_.rh},
      {EventType::RhLow, last_.rh < band.rhLo, last_.rh},
  };
  for (const C& c : cs) {
    Debounce& d = deb_[static_cast<int>(c.t)];
    if (d.active && last_.valid && !c.stillBad) {
      d.active = false;
      out.push_back(Event{c.t, Edge::SilentClear, now, c.v});
    }
    d.resetPending();  // a new band starts every onset/clear timer afresh
  }
}

void EventEngine::onMains(int64_t now, bool present, std::vector<Event>& out) {
  step(EventType::MainsFail, now, !present, present, 0, out);
}

void EventEngine::onBattery(int64_t now, int32_t mv, bool onBattery, std::vector<Event>& out) {
  const bool bad = onBattery && mv <= cfg_.battLowMv;
  const bool good = !onBattery || mv >= cfg_.battClearMv;
  step(EventType::LowBattery, now, bad, good, mv, out);
}

void EventEngine::onPaper(int64_t now, bool paperPresent, std::vector<Event>& out) {
  step(EventType::PaperOut, now, !paperPresent, paperPresent, 0, out);
}

void EventEngine::onSd(int64_t now, bool sdOk, std::vector<Event>& out) {
  step(EventType::SdFault, now, !sdOk, sdOk, 0, out);
}

void EventEngine::onClock(int64_t now, bool clockValid, std::vector<Event>& out) {
  step(EventType::ClockInvalid, now, !clockValid, clockValid, 0, out);
}

}  // namespace seri
