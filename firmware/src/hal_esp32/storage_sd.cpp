#include "storage_sd.h"
#include <Arduino.h>

namespace {
const char* kLog = "/log.csv";
const char* kQueue = "/queue.txt";
const char* kPos = "/qpos.txt";
constexpr uint32_t kLogMax = 8u * 1024 * 1024;  // rotate the log at 8 MB
}  // namespace

void SdStorage::begin() {
  pinMode(cd_, INPUT);  // GPIO34 has no internal pull-up: external pull-up on the carrier board
  mount();
}

bool SdStorage::mount() {
  if (digitalRead(cd_) == HIGH) return false;  // no card
  mounted_ = SD.begin(cs_);
  failed_ = !mounted_;
  if (mounted_) loadQueueState();
  return mounted_;
}

void SdStorage::unmount() {
  if (mounted_) SD.end();
  mounted_ = false;
  frontCache_.clear();
}

bool SdStorage::ok() {
  if (digitalRead(cd_) == HIGH) { unmount(); return false; }
  if (!mounted_ || failed_) { unmount(); mount(); }
  return mounted_ && !failed_;
}

bool SdStorage::appendLog(const std::string& line) {
  if (!mounted_) return false;
  File f = SD.open(kLog, FILE_APPEND);
  if (!f) { failed_ = true; return false; }
  if (f.size() > kLogMax) {
    f.close();
    SD.remove("/log.old");
    SD.rename(kLog, "/log.old");
    f = SD.open(kLog, FILE_APPEND);
    if (!f) { failed_ = true; return false; }
  }
  const bool ok = f.println(line.c_str()) > 0;
  f.close();
  if (!ok) failed_ = true;
  return ok;
}

void SdStorage::loadQueueState() {
  qpos_ = 0;
  qcount_ = 0;
  File p = SD.open(kPos, FILE_READ);
  if (p) { qpos_ = static_cast<uint32_t>(p.readStringUntil('\n').toInt()); p.close(); }
  File q = SD.open(kQueue, FILE_READ);
  if (!q) { qpos_ = 0; return; }
  if (qpos_ > q.size()) qpos_ = 0;
  q.seek(qpos_);
  while (q.available()) if (q.read() == '\n') ++qcount_;
  q.close();
}

void SdStorage::saveQueuePos() {
  File p = SD.open(kPos, FILE_WRITE);
  if (!p) { failed_ = true; return; }
  p.println(qpos_);
  p.close();
}

bool SdStorage::queuePush(const std::string& record) {
  if (!mounted_) return false;
  File q = SD.open(kQueue, FILE_APPEND);
  if (!q) { failed_ = true; return false; }
  const bool ok = q.println(record.c_str()) > 0;
  q.close();
  if (ok) ++qcount_;
  else failed_ = true;
  return ok;
}

bool SdStorage::queueFront(std::string& record) {
  if (!mounted_ || qcount_ == 0) return false;
  if (frontCache_.empty()) {
    File q = SD.open(kQueue, FILE_READ);
    if (!q) { failed_ = true; return false; }
    q.seek(qpos_);
    String s = q.readStringUntil('\n');
    q.close();
    s.trim();
    frontCache_ = s.c_str();
  }
  record = frontCache_;
  return !record.empty();
}

bool SdStorage::queuePop() {
  if (!mounted_ || qcount_ == 0) return false;
  std::string rec;
  if (!queueFront(rec)) return false;
  qpos_ += static_cast<uint32_t>(rec.size()) + 2;  // println adds CR LF
  --qcount_;
  frontCache_.clear();
  if (qcount_ == 0) {  // everything sent: start a fresh queue file
    SD.remove(kQueue);
    qpos_ = 0;
  }
  saveQueuePos();
  return true;
}

size_t SdStorage::queueSize() { return mounted_ ? qcount_ : 0; }
