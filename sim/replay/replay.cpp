// Phase-3 replay: runs the real device logic (lib/core Monitor) on fake hardware through a whole
// rearing cycle from a CSV, one tick per device second, and writes:
//   actual.txt  events (E) and SMS decisions (Q) in the same format as the oracle's expected.txt
//   stats.txt   what was printed, sent, uploaded and logged, per day
// Build/run: tools/phase3_gate.sh
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "fakes.h"
#include "seri/monitor.h"
#include "seri/provision.h"
#include "seri/time_util.h"

using namespace seri;

namespace {
struct Row {
  int64_t t;
  float temp, rh;
  bool mains, net, sd, paper, sensor;
  int32_t batt;
  std::string from, body;
};

std::vector<std::string> split(const std::string& s) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : s) {
    if (c == ',') { out.push_back(cur); cur.clear(); }
    else if (c != '\r') cur += c;
  }
  out.push_back(cur);
  return out;
}

struct Collector : hal::Trace {
  std::vector<std::string> lines;
  void line(const std::string& l) override { lines.push_back(l); }
};

std::string ist(int64_t t) {
  CivilTime c = toIst(t);
  char b[32];
  std::snprintf(b, sizeof b, "%04d-%02d-%02d %02d:%02d:%02d", c.year, c.month, c.day, c.hour, c.minute, c.second);
  return b;
}
std::string istDate(int64_t t) { return ist(t).substr(0, 10); }
}  // namespace

int main(int argc, char** argv) {
  if (argc != 4) { std::fprintf(stderr, "usage: replay cycle.csv actual.txt stats.txt\n"); return 2; }
  std::ifstream in(argv[1]);
  std::string line;
  std::getline(in, line);  // header
  std::vector<Row> rows;
  while (std::getline(in, line)) {
    auto f = split(line);
    rows.push_back(Row{std::atoll(f[0].c_str()), std::strtof(f[1].c_str(), nullptr), std::strtof(f[2].c_str(), nullptr),
                       f[3] == "1", f[5] == "1", f[6] == "1", f[7] == "1", f[8] == "1", std::atoi(f[4].c_str()), f[9], f[10]});
  }
  const int64_t t0 = rows.front().t, end = t0 + static_cast<int64_t>(rows.size()) * 30;

  fakes::Clock clock; fakes::Sensor sensor; fakes::Power power; fakes::Modem modem; fakes::Wifi wifi;
  fakes::Printer printer; fakes::Storage storage; fakes::Indicators ind; fakes::Persist persist;
  Collector trace;
  clock.t = t0;
  modem.clock = &clock;
  {  // a provisioned device (D-060): broker settings already in flash, as after the installer's SD import
    MqttConfig b;
    b.host = "broker.replay.test";
    b.caPem = "-----BEGIN CERTIFICATE-----\nREPLAY\n-----END CERTIFICATE-----\n";
    persist.kv["mqtt"] = encodeMqtt(b);
  }
  Monitor m(Hardware{clock, sensor, power, modem, wifi, printer, storage, ind, persist, &trace});
  m.begin();  // fresh device: factory settings, no phone numbers

  std::map<std::string, int> printedPerDay, smsSentPerDay;
  size_t printedSeen = 0, sentSeen = 0;
  int64_t lastSampleT = t0, maxGap = 0;
  size_t maxOutbox = 0, maxBacklog = 0, maxRam = 0;
  for (int64_t s = t0 + 1; s < end; ++s) {
    const Row& r = rows[static_cast<size_t>((s - t0) / 30)];
    if (s == r.t && !r.from.empty()) modem.inbox.push_back({r.from, r.body});
    sensor.t = r.temp; sensor.rh = r.rh; sensor.connected = r.sensor;
    power.mains = r.mains; power.mv = r.batt;
    modem.network = r.net;
    storage.present = r.sd;
    printer.paper = r.paper;
    clock.t = s;
    m.tick();
    for (; printedSeen < printer.lines.size(); ++printedSeen) printedPerDay[istDate(s)]++;
    for (; sentSeen < modem.sent.size(); ++sentSeen) smsSentPerDay[istDate(s)]++;
    maxOutbox = std::max(maxOutbox, m.outboxSize());
    maxBacklog = std::max(maxBacklog, m.printBacklog());
    maxRam = std::max(maxRam, m.ramRecords());
  }

  // ---- actual.txt: same format as the oracle ----
  std::ofstream act(argv[2]);
  act << "# Phase-3 replay actual output (real Monitor on fake hardware)\n";
  std::map<std::pair<std::string, int>, std::map<std::string, int>> days;
  int events = 0, starts = 0, decisions = 0, samples = 0;
  for (const std::string& l : trace.lines) {
    auto f = split(l);
    if (f[0] == "E") {
      act << "E " << ist(std::atoll(f[1].c_str())) << " " << f[2] << " " << f[3] << "\n";
      ++events;
      starts += f[3] == "Start";
    } else if (f[0] == "Q") {
      const int64_t t = std::atoll(f[1].c_str());
      act << "Q " << ist(t) << " " << f[2] << " " << f[3] << " slot" << f[4] << "\n";
      days[{istDate(t), std::atoi(f[4].c_str())}][f[2]]++;
      ++decisions;
    } else if (f[0] == "S") {
      const int64_t t = std::atoll(f[1].c_str());
      maxGap = std::max(maxGap, t - lastSampleT);
      lastSampleT = t;
      ++samples;
    }
  }
  for (auto& kv : days) {
    auto& c = kv.second;
    act << "DAY " << kv.first.first << " slot" << kv.first.second << " start=" << c["Start"] << " repeat=" << c["Repeat"]
        << " recovery=" << c["Recovery"] << " limit=" << c["LimitReached"]
        << " total=" << (c["Start"] + c["Repeat"] + c["Recovery"] + c["LimitReached"]) << "\n";
  }
  act << "TOTAL events=" << events << " starts=" << starts << " sms_decisions=" << decisions << "\n";

  // ---- stats.txt ----
  std::ofstream st(argv[3]);
  int summaries = 0, late = 0, notSent = 0;
  for (auto& p : printer.lines) {
    summaries += p.find("DAILY SUMMARY") != std::string::npos;
    late += p.find("PRINTED LATE") != std::string::npos;
    notSent += p.find("SMS NOT SENT") != std::string::npos;
  }
  st << "samples=" << samples << " max_gap_between_samples_s=" << maxGap << "\n";
  st << "printed_lines=" << printer.lines.size() << " daily_summaries=" << summaries << " printed_late=" << late
     << " sms_not_sent_lines=" << notSent << "\n";
  st << "offline_summaries=" << m.offlineSummariesSent() << "\n";
  st << "sms_sent=" << modem.sent.size() << " uploads=" << modem.published.size() << " queue_left=" << storage.queue.size()
     << " sd_log_lines=" << storage.log.size() << "\n";
  st << "max_outbox=" << maxOutbox << " max_print_backlog=" << maxBacklog << " max_ram_records=" << maxRam << "\n";
  st << "settings: stage=" << static_cast<int>(m.settings().stage) << " owner=" << m.settings().phone[0]
     << " helper=" << m.settings().phone[1] << " helper_lang=" << static_cast<int>(m.settings().lang[1]) << "\n";
  for (auto& kv : printedPerDay) st << "PRINTED " << kv.first << " " << kv.second << "\n";
  for (auto& kv : smsSentPerDay) st << "SENT " << kv.first << " " << kv.second << "\n";
  std::printf("replay: %d events, %d SMS decisions, %zu SMS sent, %zu printed lines, %d summaries\n", events, decisions,
              modem.sent.size(), printer.lines.size(), summaries);
  return 0;
}
