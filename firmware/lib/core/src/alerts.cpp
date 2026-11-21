#include "seri/alerts.h"
#include "seri/time_util.h"

namespace seri {

bool sendsStartSms(EventType) { return true; }
bool sendsRepeatSms(EventType t) { return isClimate(t) || t == EventType::SensorFault; }
bool sendsRecoverySms(EventType t) {
  return isClimate(t) || t == EventType::SensorFault || t == EventType::MainsFail;
}

AlertPolicy::AlertPolicy(const AlertConfig& cfg, const AlertState& st) : cfg_(cfg), st_(st) {}

void AlertPolicy::rollover(int64_t now) {
  const int64_t d = istDay(now);
  if (d == st_.day) return;
  st_.day = d;
  for (auto& r : st_.r) {
    r.repeatsToday = 0;
    r.totalToday = 0;
    r.limitNotified = false;  // reserved recoveries carry over: they are still owed
  }
}

// Sends one text subject to the repeat cap (reserve = also keep room for its recovery).
bool AlertPolicy::cappedSend(uint8_t i, SmsKind kind, EventType t, int64_t now, int64_t value,
                             bool reserve, std::vector<SmsJob>& out) {
  auto& r = st_.r[i];
  const int need = reserve ? 2 : 1;
  const bool repeatRoom = (kind == SmsKind::Start) || r.repeatsToday < cfg_.repeatCapPerDay;
  const bool hardRoom = r.totalToday + r.reserved + need <= cfg_.hardCapPerDay;
  if (repeatRoom && hardRoom) {
    out.push_back(SmsJob{i, kind, t, now, static_cast<Deci>(value), st_.ep[static_cast<int>(t)].start});
    if (kind == SmsKind::Repeat) ++r.repeatsToday;
    ++r.totalToday;
    if (reserve) ++r.reserved;
    return true;
  }
  if (!r.limitNotified && r.totalToday + r.reserved + 1 <= cfg_.hardCapPerDay) {
    out.push_back(SmsJob{i, SmsKind::LimitReached, t, now, 0, 0});
    r.limitNotified = true;
    ++r.totalToday;
  }
  return false;
}

void AlertPolicy::onEvent(const Event& e, uint8_t recipients, std::vector<SmsJob>& out) {
  rollover(e.time);
  auto& ep = st_.ep[static_cast<int>(e.type)];
  if (e.edge == Edge::Start) {
    const bool recurrent = ep.haveEnded && e.time - ep.lastEnd < cfg_.recurrenceSec;
    ep.active = true;
    ep.start = ep.lastRepeat = e.time;
    ep.lastValue = e.value;
    ep.toldMask = 0;
    if (!sendsStartSms(e.type)) return;
    for (uint8_t i = 0; i < kMaxRecipients; ++i) {
      if (!(recipients & (1u << i))) continue;
      const bool reserve = sendsRecoverySms(e.type);
      const SmsKind k = recurrent ? SmsKind::Repeat : SmsKind::Start;
      if (cappedSend(i, k, e.type, e.time, e.value, reserve, out)) ep.toldMask |= (1u << i);
    }
    return;
  }
  if (!ep.active) return;
  for (uint8_t i = 0; i < kMaxRecipients; ++i) {
    if (!(ep.toldMask & (1u << i)) || !sendsRecoverySms(e.type)) continue;
    auto& r = st_.r[i];
    if (r.reserved) --r.reserved;
    if (e.edge == Edge::Clear && (recipients & (1u << i))) {
      out.push_back(SmsJob{i, SmsKind::Recovery, e.type, e.time, e.value, ep.start});
      ++r.totalToday;
    }
  }
  ep.active = false;
  ep.haveEnded = true;
  ep.lastEnd = e.time;
  ep.toldMask = 0;
}

void AlertPolicy::tick(int64_t now, uint8_t recipients, std::vector<SmsJob>& out) {
  rollover(now);
  for (int k = 0; k < kEventTypeCount; ++k) {
    auto& ep = st_.ep[k];
    const EventType t = static_cast<EventType>(k);
    if (!ep.active || !sendsRepeatSms(t) || now - ep.lastRepeat < cfg_.repeatSec) continue;
    ep.lastRepeat = now;
    for (uint8_t i = 0; i < kMaxRecipients; ++i) {
      if (!(recipients & (1u << i))) continue;
      const bool firstTell = !(ep.toldMask & (1u << i));
      if (cappedSend(i, SmsKind::Repeat, t, now, ep.lastValue, firstTell && sendsRecoverySms(t), out))
        ep.toldMask |= (1u << i);
    }
  }
}

}  // namespace seri
