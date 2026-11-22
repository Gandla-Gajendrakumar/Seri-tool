#include "seri/monitor.h"
#include <cstdlib>
#include "fmt.h"
#include "seri/persist.h"
#include "seri/power.h"
#include "seri/sample.h"
#include "seri/sms_text.h"
#include "seri/time_util.h"

namespace seri {
namespace {
const char* const kTypeName[kEventTypeCount] = {"TempHigh", "TempLow", "RhHigh", "RhLow", "SensorFault",
                                                "MainsFail", "LowBattery", "PaperOut", "SdFault", "ClockInvalid"};
const char* const kEdgeName[] = {"Start", "Clear", "SilentClear"};
}  // namespace

Monitor::Monitor(Hardware hw, const MonitorConfig& cfg)
    : hw_(hw), cfg_(cfg), engine_(bandFor(Stage::Chawki, Tolerance{})), summary_(0), scheduler_(cfg.summaryHourIst) {}

void Monitor::begin() {
  const int64_t now = hw_.clock.nowUtc();
  std::string blob;
  Settings s;
  if (hw_.persist.load("settings", blob) && decodeSettings(blob, s)) settings_ = s;
  else saveSettings();
  AlertState st;
  if (hw_.persist.load("alerts", blob) && decodeAlertState(blob, st)) alerts_ = AlertPolicy(AlertConfig{}, st);
  engine_ = EventEngine(bandFor(settings_.stage, settings_.tol));
  summary_ = DailySummary(now);
  scheduler_ = SummaryScheduler(cfg_.summaryHourIst);
  onBattery_ = !hw_.power.mainsPresent();
  batteryPct_ = batteryPercent(hw_.power.batteryMv());
  nextRaw_ = nextPower_ = nextPaper_ = nextSd_ = nextPoll_ = now + 1;
  nextAlertTick_ = now + 60;
  nextUpload_ = now + uploadIntervalSec(onBattery_);
  sdWasOk_ = hw_.storage.ok();
  lastDay_ = istDay(now);
  logLine("B," + std::to_string(now) + ",BOOT");
  clockSync(now);
  begun_ = true;
  indicate();
}

bool Monitor::anyActive() const {
  for (int i = 0; i < kEventTypeCount; ++i) if (engine_.active(static_cast<EventType>(i))) return true;
  return false;
}

void Monitor::tick() {
  if (!begun_) return;
  const int64_t now = hw_.clock.nowUtc();
  std::vector<Event> ev;
  if (istDay(now) != lastDay_) { lastDay_ = istDay(now); settings_.pinFailuresToday = 0; }

  if (now >= nextPower_) {
    nextPower_ = now + cfg_.powerCheckSec;
    const bool mains = hw_.power.mainsPresent();
    const int32_t mv = hw_.power.batteryMv();
    onBattery_ = !mains;
    batteryPct_ = batteryPercent(mv);
    engine_.onMains(now, mains, ev);
    engine_.onBattery(now, mv, onBattery_, ev);
    handle(ev, now); ev.clear();
  }
  if (now >= nextRaw_) {
    nextRaw_ = now + cfg_.rawReadSec;
    raw_.push_back(hw_.sensor.read());
    if (raw_.size() >= 5) sampleStep(now);
  }
  if (now >= nextPaper_) {
    nextPaper_ = now + cfg_.paperCheckSec;
    engine_.onPaper(now, hw_.printer.paperPresent(), ev);
    handle(ev, now); ev.clear();
    printStep();
  }
  if (now >= nextSd_) { nextSd_ = now + cfg_.sdCheckSec; sdStep(now); }
  if (now >= nextAlertTick_) {
    nextAlertTick_ = now + 60;
    std::vector<SmsJob> jobs;
    alerts_.tick(now, recipientMask(settings_), jobs);
    if (!jobs.empty()) { enqueueJobs(jobs, now); saveAlerts(); }
  }
  if (now >= nextPoll_) { nextPoll_ = now + cfg_.smsPollSec; commandStep(now); }
  sendStep(now);
  uploadStep(now);
  for (const auto& due : scheduler_.tick(now, onBattery_, summary_)) {
    const DayReport& r = due.report;
    logLine("D," + std::to_string(now) + ",SUMMARY," + std::to_string(r.tMin) + "," + std::to_string(r.tMax) + "," +
            std::to_string(r.rhMin) + "," + std::to_string(r.rhMax) + "," + std::to_string(r.tOutSec) + "," +
            std::to_string(r.rhOutSec) + "," + std::to_string(r.alerts) + (due.late ? ",late" : ""));
    for (const auto& line : printSummary(r, settings_.stage, engine_.band(), ownerLang(), due.late))
      printOrQueue(line, ownerLang());
  }
  if (now >= nextClockSync_) clockSync(now);
  indicate();
}

void Monitor::sampleStep(int64_t now) {
  Sample s = medianSample(raw_.data(), raw_.size());
  raw_.clear();
  last_ = s;
  std::vector<Event> ev;
  engine_.onSample(now, s, ev);
  summary_.addSample(now, s, engine_.band());
  pendingSamples_.push_back({now, s});
  logLine(s.valid ? "S," + std::to_string(now) + "," + std::to_string(s.t) + "," + std::to_string(s.rh)
                  : "S," + std::to_string(now) + ",invalid");
  handle(ev, now);
}

void Monitor::handle(const std::vector<Event>& evs, int64_t now) {
  if (evs.empty()) return;
  for (const Event& e : evs) {
    logLine("E," + std::to_string(e.time) + "," + kTypeName[static_cast<int>(e.type)] + "," +
            kEdgeName[static_cast<int>(e.edge)] + "," + std::to_string(e.value));
    printOrQueue(printEventLine(e, ownerLang()), ownerLang());
    summary_.addEvent(e);
    if (e.edge == Edge::Start && settings_.buzzerOn) buzzerOn_ = true;
    std::vector<SmsJob> jobs;
    alerts_.onEvent(e, recipientMask(settings_), jobs);
    enqueueJobs(jobs, now);
  }
  saveAlerts();
  if (!anyActive()) buzzerOn_ = false;
}

void Monitor::enqueueJobs(const std::vector<SmsJob>& jobs, int64_t now) {
  for (const SmsJob& j : jobs) {
    const std::string& number = settings_.phone[j.recipient];
    if (number.empty()) continue;
    const bool temp = j.type == EventType::TempHigh || j.type == EventType::TempLow;
    SmsContext ctx{settings_.deviceName, settings_.stage, engine_.band(), settings_.lang[j.recipient], now,
                   temp ? last_.t : last_.rh, batteryPct_};
    outbox_.push_back(OutgoingSms{number, formatSms(j, ctx), now, now, 0});
  }
}

void Monitor::sendStep(int64_t now) {
  if (outbox_.empty() || now < outbox_.front().nextTry) return;
  OutgoingSms& o = outbox_.front();
  if (hw_.modem.sendSms(o.number, o.text)) {
    logLine("M," + std::to_string(now) + ",sent," + o.number);
    outbox_.pop_front();
    return;
  }
  ++o.attempts;
  if (now - o.firstTry >= cfg_.smsGiveUpSec) {
    logLine("M," + std::to_string(now) + ",gave-up," + o.number);
    printOrQueue(fmt::ddmmHhmm(now) + " SMS NOT SENT", Language::English);
    outbox_.pop_front();
    return;
  }
  const int64_t backoff = std::min<int64_t>(60LL << std::min(o.attempts - 1, 2), 240);  // 1, 2, 4, 4... min
  o.nextTry = now + backoff;
}

void Monitor::printOrQueue(const std::string& line, Language l) {
  if (backlog_.empty() && hw_.printer.paperPresent() && hw_.printer.printLine(line, l)) return;
  backlog_.push_back({line, l});
  if (backlog_.size() > cfg_.printBacklogMax) backlog_.pop_front();
}

void Monitor::printStep() {
  for (int n = 0; n < 20 && !backlog_.empty() && hw_.printer.paperPresent(); ++n) {
    if (!hw_.printer.printLine(backlog_.front().first, backlog_.front().second)) break;
    backlog_.pop_front();
  }
}

void Monitor::logLine(const std::string& line) {
  if (hw_.trace) hw_.trace->line(line);
  if (hw_.storage.ok() && hw_.storage.appendLog(line)) return;
  ramLog_.push_back(line);
  if (ramLog_.size() > cfg_.ramLogLines) ramLog_.pop_front();
}

void Monitor::sdStep(int64_t now) {
  const bool ok = hw_.storage.ok();
  std::vector<Event> ev;
  engine_.onSd(now, ok, ev);
  engine_.onClock(now, hw_.clock.valid(), ev);
  if (ok && !sdWasOk_) {  // card is back: move everything held in RAM onto it
    while (!ramLog_.empty() && hw_.storage.appendLog(ramLog_.front())) ramLog_.pop_front();
    while (!ramQueue_.empty() && hw_.storage.queuePush(ramQueue_.front())) ramQueue_.pop_front();
  }
  sdWasOk_ = ok;
  handle(ev, now);
}

void Monitor::uploadStep(int64_t now) {
  if (now >= nextUpload_) {
    nextUpload_ = now + uploadIntervalSec(onBattery_);
    std::string rec = "{\"d\":\"" + settings_.deviceName + "\",\"s\":[";
    for (size_t i = 0; i < pendingSamples_.size(); ++i) {
      const auto& p = pendingSamples_[i];
      if (i) rec += ",";
      rec += p.second.valid ? "[" + std::to_string(p.first) + "," + std::to_string(p.second.t) + "," +
                                  std::to_string(p.second.rh) + "]"
                            : "[" + std::to_string(p.first) + ",null,null]";
    }
    rec += "],\"m\":" + std::string(onBattery_ ? "0" : "1") + ",\"b\":" + std::to_string(batteryPct_) + "}";
    pendingSamples_.clear();
    if (!(hw_.storage.ok() && hw_.storage.queuePush(rec))) {
      ramQueue_.push_back(rec);
      if (ramQueue_.size() > cfg_.ramRecords) ramQueue_.pop_front();
    }
  }
  flushQueue();
  (void)now;
}

void Monitor::flushQueue() {
  const bool wifi = hw_.wifi.connected();
  if (!wifi && !hw_.modem.dataReady()) return;
  const std::string topic = "seri/" + settings_.deviceName + "/data";
  auto publish = [&](const std::string& rec) {
    return wifi ? hw_.wifi.mqttPublish(topic, rec) : hw_.modem.mqttPublish(topic, rec);
  };
  std::string rec;
  if (hw_.storage.ok() && hw_.storage.queueFront(rec)) {
    if (publish(rec)) { hw_.storage.queuePop(); lastUploadOk_ = hw_.clock.nowUtc(); }
  } else if (!ramQueue_.empty()) {
    if (publish(ramQueue_.front())) { ramQueue_.pop_front(); lastUploadOk_ = hw_.clock.nowUtc(); }
  }
}

void Monitor::commandStep(int64_t now) {
  std::string from, body;
  while (hw_.modem.pollSms(from, body)) {
    const Stage oldStage = settings_.stage;
    const Tolerance oldTol = settings_.tol;
    CmdOutcome o = applyCommand(settings_, from, body);
    logLine("C," + std::to_string(now) + "," + std::to_string(static_cast<int>(o.result)) + "," +
            std::to_string(static_cast<int>(o.type)));
    if (o.result == CmdResult::Ignored) continue;
    const std::string reply = formatReply(o, settings_, StatusInfo{last_, onBattery_, batteryPct_, now});
    const std::string to = normalizeNumber(from);
    if (!reply.empty() && !to.empty()) outbox_.push_back(OutgoingSms{to, reply, now, now, 0});
    const bool stageChanged = settings_.stage != oldStage;
    if (stageChanged || settings_.tol.t != oldTol.t || settings_.tol.rh != oldTol.rh) applyBand(now, stageChanged);
    saveSettings();
  }
}

void Monitor::applyBand(int64_t now, bool printStage) {
  std::vector<Event> ev;
  if (printStage) {
    printOrQueue(printStageLine(now, settings_.stage, ownerLang()), ownerLang());
    logLine("G," + std::to_string(now) + ",STAGE," + std::to_string(static_cast<int>(settings_.stage)));
  }
  engine_.setBand(now, bandFor(settings_.stage, settings_.tol), ev);
  handle(ev, now);
}

void Monitor::changeSettings(const Settings& s) {
  if (!toleranceValid(s.tol) || static_cast<int>(s.stage) >= kStageCount) return;
  const bool stageChanged = s.stage != settings_.stage;
  const bool tolChanged = s.tol.t != settings_.tol.t || s.tol.rh != settings_.tol.rh;
  settings_ = s;
  if (stageChanged || tolChanged) applyBand(hw_.clock.nowUtc(), stageChanged);
  saveSettings();
}

void Monitor::clockSync(int64_t now) {
  nextClockSync_ = now + cfg_.clockSyncSec;
  int64_t t = 0;
  if (!(hw_.wifi.ntpTime(t) || hw_.modem.networkTime(t))) return;
  if (std::llabs(t - now) > 2 || !hw_.clock.valid()) {
    hw_.clock.set(t);
    logLine("K," + std::to_string(t) + ",CLOCK,corrected," + std::to_string(now));
  }
}

void Monitor::silenceBuzzer() {
  buzzerOn_ = false;
  indicate();
}

void Monitor::saveSettings() { hw_.persist.save("settings", encodeSettings(settings_)); }
void Monitor::saveAlerts() { hw_.persist.save("alerts", encodeAlertState(alerts_.state())); }

void Monitor::indicate() {
  const bool any = anyActive();
  hw_.indicators.red(any);
  hw_.indicators.green(!any);
  hw_.indicators.buzzer(buzzerOn_);
}

}  // namespace seri
