#pragma once
// SMS flood control (requirements F-EVT-04/05, F-SMS-05, R-01, R-08).
//
// Per registered number, per IST day:
//  * The FIRST text of an event always goes, unless the same kind of event ended less than
//    60 min ago (a "recurrence"). A recurrence counts as a repeat.
//  * Repeats every 30 min while a climate or sensor event lasts, at most 6 per day. Then one
//    "daily limit reached, still monitoring" text.
//  * Recovery ("back to normal") text: always sent for an event the number was told about.
//  * Hard ceiling: 20 texts per number per day, counting everything. Each first text reserves
//    room for its own recovery text, so recoveries are never blocked (decision D-012).
// All state lives in AlertState, a plain struct saved to flash so a reboot cannot reset caps.
#include <cstdint>
#include <vector>
#include "seri/events.h"

namespace seri {

constexpr int kMaxRecipients = 3;

enum class SmsKind : uint8_t { Start, Repeat, Recovery, LimitReached };

struct SmsJob {
  uint8_t recipient;  // slot 0..2
  SmsKind kind;
  EventType type;     // meaningless for LimitReached
  int64_t time;
  Deci value;
  int64_t startedAt;  // when the event started (for "for N min" in repeats)
};

struct AlertConfig {
  int64_t repeatSec = 1800;      // 30 min (brief)
  uint8_t repeatCapPerDay = 6;   // Q-2
  uint8_t hardCapPerDay = 20;    // D-012
  int64_t recurrenceSec = 3600;  // same event again within 60 min counts as a repeat
};

struct AlertState {
  int64_t day = -1;  // IST day number the counters belong to
  struct Recipient {
    uint8_t repeatsToday = 0;
    uint8_t totalToday = 0;
    uint8_t reserved = 0;  // recovery texts owed (room kept under the hard cap)
    bool limitNotified = false;
  } r[kMaxRecipients];
  struct Episode {
    bool active = false;
    bool haveEnded = false;
    int64_t start = 0, lastRepeat = 0, lastEnd = 0;
    int64_t lastValue = 0;
    uint8_t toldMask = 0;  // which recipients were told about this episode
  } ep[kEventTypeCount];
};

bool sendsStartSms(EventType t);
bool sendsRepeatSms(EventType t);
bool sendsRecoverySms(EventType t);

class AlertPolicy {
 public:
  explicit AlertPolicy(const AlertConfig& cfg = AlertConfig{}, const AlertState& st = AlertState{});
  // recipients: bit i set = slot i has a registered number.
  void onEvent(const Event& e, uint8_t recipients, std::vector<SmsJob>& out);
  // Call at least once a minute: sends due repeats, rolls counters at IST midnight.
  void tick(int64_t now, uint8_t recipients, std::vector<SmsJob>& out);
  const AlertState& state() const { return st_; }

 private:
  void rollover(int64_t now);
  bool cappedSend(uint8_t i, SmsKind kind, EventType t, int64_t now, int64_t value, bool reserve,
                  std::vector<SmsJob>& out);
  AlertConfig cfg_;
  AlertState st_;
};

}  // namespace seri
