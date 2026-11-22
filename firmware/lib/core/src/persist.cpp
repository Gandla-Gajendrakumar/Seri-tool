#include "seri/persist.h"

namespace seri {
namespace {

constexpr char kMagic0 = 'S', kMagic1 = 'R';
constexpr uint8_t kSettingsVersion = 1, kAlertVersion = 1;

struct Writer {
  std::string b;
  void u8(uint8_t v) { b += static_cast<char>(v); }
  void i64(int64_t v) { for (int i = 0; i < 8; ++i) u8(static_cast<uint8_t>(static_cast<uint64_t>(v) >> (8 * i))); }
  void i32(int32_t v) { for (int i = 0; i < 4; ++i) u8(static_cast<uint8_t>(static_cast<uint32_t>(v) >> (8 * i))); }
  void str(const std::string& s) { u8(static_cast<uint8_t>(s.size() > 255 ? 255 : s.size())); b.append(s, 0, 255); }
};
struct Reader {
  const std::string& b;
  size_t p = 0;
  bool okFlag = true;
  bool need(size_t n) { if (p + n > b.size()) okFlag = false; return okFlag; }
  uint8_t u8() { return need(1) ? static_cast<uint8_t>(b[p++]) : 0; }
  int64_t i64() { uint64_t v = 0; for (int i = 0; i < 8; ++i) v |= static_cast<uint64_t>(u8()) << (8 * i); return static_cast<int64_t>(v); }
  int32_t i32() { uint32_t v = 0; for (int i = 0; i < 4; ++i) v |= static_cast<uint32_t>(u8()) << (8 * i); return static_cast<int32_t>(v); }
  std::string str() { size_t n = u8(); if (!need(n)) return ""; std::string s = b.substr(p, n); p += n; return s; }
};

std::string seal(uint8_t version, const std::string& payload) {
  Writer w;
  w.u8(kMagic0); w.u8(kMagic1); w.u8(version);
  w.b += payload;
  uint32_t c = crc32(w.b);
  w.i32(static_cast<int32_t>(c));
  return w.b;
}
// Returns payload if magic, version and CRC are right.
bool unseal(const std::string& blob, uint8_t version, std::string& payload) {
  if (blob.size() < 7 || blob[0] != kMagic0 || blob[1] != kMagic1 || static_cast<uint8_t>(blob[2]) != version) return false;
  const std::string body = blob.substr(0, blob.size() - 4);
  Reader r{blob};
  r.p = blob.size() - 4;
  if (static_cast<uint32_t>(r.i32()) != crc32(body)) return false;
  payload = body.substr(3);
  return true;
}

}  // namespace

uint32_t crc32(const std::string& data) {
  uint32_t c = 0xFFFFFFFFu;
  for (unsigned char ch : data) {
    c ^= ch;
    for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1u)));
  }
  return ~c;
}

std::string encodeSettings(const Settings& s) {
  Writer w;
  for (int i = 0; i < kMaxRecipients; ++i) { w.str(s.phone[i]); w.u8(static_cast<uint8_t>(s.lang[i])); }
  w.u8(static_cast<uint8_t>(s.stage));
  w.i32(s.tol.t); w.i32(s.tol.rh);
  w.u8(s.buzzerOn ? 1 : 0);
  w.str(s.deviceName); w.str(s.pin); w.str(s.wifiSsid); w.str(s.wifiPass);
  return seal(kSettingsVersion, w.b);
}

bool decodeSettings(const std::string& blob, Settings& out) {
  std::string payload;
  if (!unseal(blob, kSettingsVersion, payload)) return false;
  Reader r{payload};
  Settings s;
  for (int i = 0; i < kMaxRecipients; ++i) {
    s.phone[i] = r.str();
    uint8_t l = r.u8();
    if (l >= kLanguageCount) return false;
    s.lang[i] = static_cast<Language>(l);
  }
  uint8_t st = r.u8();
  if (st >= kStageCount) return false;
  s.stage = static_cast<Stage>(st);
  s.tol.t = r.i32(); s.tol.rh = r.i32();
  if (!toleranceValid(s.tol)) return false;
  s.buzzerOn = r.u8() != 0;
  s.deviceName = r.str(); s.pin = r.str(); s.wifiSsid = r.str(); s.wifiPass = r.str();
  if (!r.okFlag || r.p != payload.size()) return false;
  out = s;
  return true;
}

std::string encodeAlertState(const AlertState& a) {
  Writer w;
  w.i64(a.day);
  for (const auto& r : a.r) { w.u8(r.repeatsToday); w.u8(r.totalToday); w.u8(r.reserved); w.u8(r.limitNotified); }
  for (const auto& e : a.ep) {
    w.u8(e.active); w.u8(e.haveEnded);
    w.i64(e.start); w.i64(e.lastRepeat); w.i64(e.lastEnd); w.i64(e.lastValue);
    w.u8(e.toldMask);
  }
  return seal(kAlertVersion, w.b);
}

bool decodeAlertState(const std::string& blob, AlertState& out) {
  std::string payload;
  if (!unseal(blob, kAlertVersion, payload)) return false;
  Reader r{payload};
  AlertState a;
  a.day = r.i64();
  for (auto& x : a.r) { x.repeatsToday = r.u8(); x.totalToday = r.u8(); x.reserved = r.u8(); x.limitNotified = r.u8() != 0; }
  for (auto& e : a.ep) {
    e.active = r.u8() != 0; e.haveEnded = r.u8() != 0;
    e.start = r.i64(); e.lastRepeat = r.i64(); e.lastEnd = r.i64(); e.lastValue = r.i64();
    e.toldMask = r.u8();
  }
  if (!r.okFlag || r.p != payload.size()) return false;
  out = a;
  return true;
}

}  // namespace seri
