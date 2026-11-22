#pragma once
// The application: everything the device does, written only against the HAL (seri/hal.h).
// The firmware calls tick() every ~100 ms; all timing comes from the Clock, so tests (and the
// accelerated Wokwi build) can run days of behaviour in seconds.
#include <deque>
#include <map>
#include <string>
#include <vector>
#include "seri/alerts.h"
#include "seri/commands.h"
#include "seri/events.h"
#include "seri/hal.h"
#include "seri/print.h"

namespace seri {

struct Hardware {
  hal::Clock& clock;
  hal::Sensor& sensor;
  hal::Power& power;
  hal::Modem& modem;
  hal::Wifi& wifi;
  hal::Printer& printer;
  hal::Storage& storage;
  hal::Indicators& indicators;
  hal::Persist& persist;
  hal::Trace* trace = nullptr;
};

struct MonitorConfig {
  int64_t rawReadSec = 6;          // 5 raw reads per 30 s sample (D-006)
  int64_t powerCheckSec = 1;
  int64_t paperCheckSec = 5;
  int64_t sdCheckSec = 30;
  int64_t smsPollSec = 5;
  int64_t clockSyncSec = 86400;    // F-CLK-01
  int64_t smsGiveUpSec = 3600;     // F-SMS-06
  size_t ramRecords = 48;          // upload records kept in RAM while SD is out (4 h, R-04)
  size_t ramLogLines = 400;
  size_t printBacklogMax = 200;    // F-PRN-05
  int summaryHourIst = 6;
  int32_t modemCutoffMv = 3600;    // D-038: below this a 2 A modem burst pulls VBAT under 3.4 V (SPICE sweep)
  int64_t printModemGapSec = 2;    // D-039: on battery, no modem activity within 2 s of printing
};

struct OutgoingSms {
  std::string number;
  std::string text;
  int64_t firstTry, nextTry;
  int attempts;
};

class Monitor {
 public:
  explicit Monitor(Hardware hw, const MonitorConfig& cfg = MonitorConfig{});
  void begin();
  void tick();

  // Local UI hooks (buttons / OLED).
  void silenceBuzzer();
  bool buzzing() const { return buzzerOn_; }
  void printText(const std::string& text);  // printed with date/time, e.g. printer test from the menu
  const Settings& settings() const { return settings_; }
  void changeSettings(const Settings& s);  // validated; re-applies the band if stage/tolerance changed
  const Sample& lastSample() const { return last_; }
  bool onBattery() const { return onBattery_; }
  int batteryPct() const { return batteryPct_; }
  bool active(EventType t) const { return engine_.active(t); }
  bool anyActive() const;
  size_t outboxSize() const { return outbox_.size(); }
  size_t printBacklog() const { return backlog_.size(); }
  size_t ramRecords() const { return ramQueue_.size(); }
  int64_t lastUploadOk() const { return lastUploadOk_; }
  int offlineSummariesSent() const { return offlineSummaries_; }

 private:
  void sampleStep(int64_t now);
  void handle(const std::vector<Event>& evs, int64_t now);
  void enqueueJobs(const std::vector<SmsJob>& jobs, int64_t now);
  void sendStep(int64_t now);
  void printOrQueue(const std::string& line, Language l);
  void printStep();
  void logLine(const std::string& line);
  void sdStep(int64_t now);
  void uploadStep(int64_t now);
  void flushQueue();
  void commandStep(int64_t now);
  void clockSync(int64_t now);
  void applyBand(int64_t now, bool printStage);
  void saveSettings();
  void saveAlerts();
  void indicate();
  Language ownerLang() const { return settings_.lang[0]; }

  Hardware hw_;
  MonitorConfig cfg_;
  Settings settings_;
  EventEngine engine_;
  AlertPolicy alerts_;
  DailySummary summary_;
  SummaryScheduler scheduler_;
  Sample last_{false, 0, 0};
  std::vector<RawReading> raw_;
  std::vector<std::pair<int64_t, Sample>> pendingSamples_;  // for the next upload record
  std::deque<OutgoingSms> outbox_;
  std::deque<std::pair<std::string, Language>> backlog_;
  std::deque<std::string> ramQueue_, ramLog_;
  bool onBattery_ = false, buzzerOn_ = false, sdWasOk_ = true;
  int batteryPct_ = 100;
  int32_t batteryMv_ = 4200;
  int64_t lastPrintAt_ = -1000;
  bool modemAllowed(int64_t now) const;
  int64_t nextRaw_ = 0, nextPower_ = 0, nextPaper_ = 0, nextSd_ = 0, nextPoll_ = 0, nextAlertTick_ = 0;
  int64_t nextUpload_ = 0, nextClockSync_ = 0, lastUploadOk_ = 0, lastDay_ = -1;
  bool begun_ = false;
  // Alerts given up during an outage, per number: count and when the first one was raised (D-037).
  std::map<std::string, std::pair<int, int64_t>> missed_;
  int offlineSummaries_ = 0;
};

}  // namespace seri
