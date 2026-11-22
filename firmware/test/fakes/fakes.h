#pragma once
// Fake hardware for native tests of the application layer. Each fake records what the
// application did and can be told to fail, so faults can be injected deterministically.
#include <deque>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "seri/hal.h"

namespace fakes {
using namespace seri;

struct Clock : hal::Clock {
  int64_t t = 0;
  bool isValid = true;
  std::vector<int64_t> sets;
  int64_t nowUtc() override { return t; }
  bool valid() override { return isValid; }
  void set(int64_t utc) override { t = utc; isValid = true; sets.push_back(utc); }
};

struct Sensor : hal::Sensor {
  float t = 27.5f, rh = 85.0f;
  bool connected = true;
  int reads = 0;
  RawReading read() override { ++reads; return connected ? RawReading{true, t, rh} : RawReading{false, 0, 0}; }
};

struct Power : hal::Power {
  bool mains = true;
  int32_t mv = 4100;
  bool mainsPresent() override { return mains; }
  int32_t batteryMv() override { return mv; }
};

struct Sms { std::string to, text; int64_t at; };

struct Modem : hal::Modem {
  Clock* clock = nullptr;
  bool network = true;
  bool data = true;
  int64_t netTimeOffset = 0;  // network time = clock + offset
  std::vector<Sms> sent;
  int sendAttempts = 0;
  std::deque<std::pair<std::string, std::string>> inbox;
  std::vector<std::pair<std::string, std::string>> published;
  bool registered() override { return network; }
  bool sendSms(const std::string& n, const std::string& t) override {
    ++sendAttempts;
    if (!network) return false;
    sent.push_back({n, t, clock ? clock->t : 0});
    return true;
  }
  bool pollSms(std::string& from, std::string& body) override {
    if (inbox.empty() || !network) return false;
    from = inbox.front().first; body = inbox.front().second; inbox.pop_front();
    return true;
  }
  bool networkTime(int64_t& utc) override {
    if (!network || !clock) return false;
    utc = clock->t + netTimeOffset;
    return true;
  }
  bool dataReady() override { return network && data; }
  bool mqttPublish(const std::string& topic, const std::string& p) override {
    if (!dataReady()) return false;
    published.push_back({topic, p});
    return true;
  }
};

struct Wifi : hal::Wifi {
  bool up = false;
  std::vector<std::pair<std::string, std::string>> published;
  bool connected() override { return up; }
  bool ntpTime(int64_t&) override { return false; }
  bool mqttPublish(const std::string& topic, const std::string& p) override {
    if (!up) return false;
    published.push_back({topic, p});
    return true;
  }
};

struct Printer : hal::Printer {
  bool paper = true;
  std::vector<std::string> lines;
  bool paperPresent() override { return paper; }
  bool printLine(const std::string& s, Language) override {
    if (!paper) return false;
    lines.push_back(s);
    return true;
  }
  int count(const std::string& needle) const {
    int n = 0;
    for (auto& l : lines) if (l.find(needle) != std::string::npos) ++n;
    return n;
  }
};

struct Storage : hal::Storage {
  bool present = true;
  std::vector<std::string> log;
  std::deque<std::string> queue;
  bool ok() override { return present; }
  bool appendLog(const std::string& l) override { if (!present) return false; log.push_back(l); return true; }
  bool queuePush(const std::string& r) override { if (!present) return false; queue.push_back(r); return true; }
  bool queueFront(std::string& r) override { if (!present || queue.empty()) return false; r = queue.front(); return true; }
  bool queuePop() override { if (!present || queue.empty()) return false; queue.pop_front(); return true; }
  size_t queueSize() override { return present ? queue.size() : 0; }
  int count(const std::string& needle) const {
    int n = 0;
    for (auto& l : log) if (l.find(needle) != std::string::npos) ++n;
    return n;
  }
};

struct Indicators : hal::Indicators {
  bool buzz = false, g = false, r = false;
  void buzzer(bool on) override { buzz = on; }
  void green(bool on) override { g = on; }
  void red(bool on) override { r = on; }
};

struct Persist : hal::Persist {
  std::map<std::string, std::string> kv;
  int saves = 0;
  bool load(const char* k, std::string& b) override {
    auto it = kv.find(k);
    if (it == kv.end()) return false;
    b = it->second;
    return true;
  }
  bool save(const char* k, const std::string& b) override { kv[k] = b; ++saves; return true; }
};

}  // namespace fakes
