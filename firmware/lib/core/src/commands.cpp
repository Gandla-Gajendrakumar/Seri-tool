#include "seri/commands.h"
#include <cctype>
#include <vector>
#include "fmt.h"
#include "seri/sms_text.h"

namespace seri {
namespace {

bool allDigits(const std::string& s) {
  if (s.empty()) return false;
  for (char c : s) if (!std::isdigit(static_cast<unsigned char>(c))) return false;
  return true;
}
bool isSep(char c) { return c == ' ' || c == '*' || c == '#' || c == ',' || c == '\t' || c == '\n' || c == '\r'; }
std::vector<std::string> tokens(const std::string& s) {
  std::vector<std::string> out;
  std::string cur;
  for (char c : s) {
    if (isSep(c)) { if (!cur.empty()) out.push_back(cur), cur.clear(); }
    else cur += c;
  }
  if (!cur.empty()) out.push_back(cur);
  return out;
}
bool intIn(const std::string& s, int lo, int hi, int& v) {
  if (!allDigits(s) || s.size() > 4) return false;
  v = std::stoi(s);
  return v >= lo && v <= hi;
}

enum R { kOwnerOnly, kBad, kFull, kRCount };
const char* const kReply[kLanguageCount][kRCount] = {
    {"Only the owner number can do this.", "Code not understood. See the card.",
     "No free slot. Remove a number first: 22 1 or 22 2."},
    {"यह केवल मालिक का नंबर कर सकता है।", "कोड समझ नहीं आया। कार्ड देखें।",
     "जगह खाली नहीं। पहले नंबर हटाएँ: 22 1 या 22 2।"},
    {"ఇది యజమాని నంబర్ మాత్రమే చేయగలదు.", "కోడ్ అర్థం కాలేదు. కార్డ్ చూడండి.",
     "ఖాళీ లేదు. ముందు నంబర్ తీసివేయండి: 22 1 లేదా 22 2."},
    {"ಇದನ್ನು ಮಾಲೀಕರ ನಂಬರ್ ಮಾತ್ರ ಮಾಡಬಹುದು.", "ಕೋಡ್ ಅರ್ಥವಾಗಲಿಲ್ಲ. ಕಾರ್ಡ್ ನೋಡಿ.",
     "ಜಾಗ ಇಲ್ಲ. ಮೊದಲು ನಂಬರ್ ತೆಗೆಯಿರಿ: 22 1 ಅಥವಾ 22 2."},
};

}  // namespace

std::string normalizeNumber(const std::string& raw) {
  std::string d;
  for (char c : raw) {
    if (std::isdigit(static_cast<unsigned char>(c))) d += c;
    else if (c != ' ' && c != '+' && c != '-' && c != '(' && c != ')') return "";
  }
  if (d.size() == 12 && d.compare(0, 2, "91") == 0) d = d.substr(2);
  else if (d.size() == 11 && d[0] == '0') d = d.substr(1);
  if (d.size() != 10 || d[0] < '6') return "";
  return d;
}

Command parseCommand(const std::string& body) {
  Command c;
  std::vector<std::string> t = tokens(body);
  if (t.empty() || !allDigits(t[0])) return c;
  const std::string& code = t[0];
  int v = 0, w = 0;
  if (code == "10" && t.size() == 2 && t[1].size() == 6 && allDigits(t[1])) { c.type = CmdType::ClaimOwner; c.s1 = t[1]; }
  else if (code == "11" && t.size() == 2 && intIn(t[1], 1, kStageCount, v)) { c.type = CmdType::SetStage; c.a = v; }
  else if (code == "12" && t.size() == 2 && intIn(t[1], 1, kLanguageCount, v)) { c.type = CmdType::SetLanguage; c.a = v; }
  else if (code == "13" && t.size() == 3 && intIn(t[1], 5, 30, v) && intIn(t[2], 2, 15, w)) {
    c.type = CmdType::SetTolerance; c.a = v; c.b = w;
  } else if (code == "20" && t.size() == 1) c.type = CmdType::Status;
  else if (code == "21" && t.size() >= 2) {
    std::string rest;
    for (size_t i = 1; i < t.size(); ++i) rest += t[i];
    std::string n = normalizeNumber(rest);
    if (!n.empty()) { c.type = CmdType::AddNumber; c.s1 = n; }
  } else if (code == "22" && t.size() == 2 && intIn(t[1], 1, kMaxRecipients - 1, v)) { c.type = CmdType::RemoveNumber; c.a = v; }
  else if (code == "30") {
    // Wi-Fi: split on spaces only, so '*' and '#' can appear in the password.
    size_t p = body.find("30");
    size_t i = body.find_first_not_of(' ', p + 2);
    size_t j = i == std::string::npos ? i : body.find(' ', i);
    size_t k = j == std::string::npos ? j : body.find_first_not_of(' ', j);
    if (k != std::string::npos) {
      std::string pass = body.substr(k);
      while (!pass.empty() && (pass.back() == ' ' || pass.back() == '\r' || pass.back() == '\n')) pass.pop_back();
      std::string ssid = body.substr(i, j - i);
      if (!ssid.empty() && ssid.size() <= 32 && !pass.empty() && pass.size() <= 63) {
        c.type = CmdType::SetWifi; c.s1 = ssid; c.s2 = pass;
      }
    }
  } else if (code == "41" && t.size() == 2 && intIn(t[1], 0, 1, v)) { c.type = CmdType::Buzzer; c.a = v; }
  return c;
}

uint8_t recipientMask(const Settings& s) {
  uint8_t m = 0;
  for (int i = 0; i < kMaxRecipients; ++i) if (!s.phone[i].empty()) m |= (1u << i);
  return m;
}

int slotOf(const Settings& s, const std::string& number) {
  const std::string n = normalizeNumber(number);
  if (n.empty()) return -1;
  for (int i = 0; i < kMaxRecipients; ++i) if (s.phone[i] == n) return i;
  return -1;
}

CmdOutcome applyCommand(Settings& s, const std::string& fromRaw, const std::string& body) {
  const std::string from = normalizeNumber(fromRaw);
  int slot = from.empty() ? -1 : slotOf(s, from);
  const Command c = parseCommand(body);

  if (c.type == CmdType::ClaimOwner) {
    if (from.empty() || s.pinFailuresToday >= 5) return {CmdResult::Ignored, c.type, slot};
    if (c.s1 != s.pin) {
      ++s.pinFailuresToday;
      return {slot < 0 ? CmdResult::Ignored : CmdResult::BadCode, c.type, slot};
    }
    for (int i = 1; i < kMaxRecipients; ++i) if (s.phone[i] == from) s.phone[i].clear();
    if (s.phone[0] != from) {
      s.phone[0] = from;
      if (slot > 0) s.lang[0] = s.lang[slot];
    }
    return {CmdResult::Ok, c.type, 0};
  }
  if (slot < 0) return {CmdResult::Ignored, c.type, slot};
  if (c.type == CmdType::Invalid) return {CmdResult::BadCode, c.type, slot};
  const bool ownerOnly = c.type == CmdType::AddNumber || c.type == CmdType::RemoveNumber || c.type == CmdType::SetWifi;
  if (ownerOnly && slot != 0) return {CmdResult::NotAllowed, c.type, slot};

  switch (c.type) {
    case CmdType::SetStage: s.stage = static_cast<Stage>(c.a - 1); break;
    case CmdType::SetLanguage: s.lang[slot] = static_cast<Language>(c.a - 1); break;
    case CmdType::SetTolerance: s.tol = Tolerance{c.a, c.b * 10}; break;
    case CmdType::AddNumber: {
      if (slotOf(s, c.s1) >= 0) break;
      int free = -1;
      for (int i = 1; i < kMaxRecipients; ++i) if (s.phone[i].empty()) { free = i; break; }
      if (free < 0) return {CmdResult::Full, c.type, slot};
      s.phone[free] = c.s1;
      s.lang[free] = s.lang[0];
      break;
    }
    case CmdType::RemoveNumber: s.phone[c.a].clear(); break;
    case CmdType::SetWifi: s.wifiSsid = c.s1; s.wifiPass = c.s2; break;
    case CmdType::Buzzer: s.buzzerOn = c.a == 1; break;
    default: break;
  }
  return {CmdResult::Ok, c.type, slot, c.type == CmdType::AddNumber ? c.s1 : std::string()};
}

std::string formatReply(const CmdOutcome& o, const Settings& s, const StatusInfo& st) {
  if (o.result == CmdResult::Ignored) return "";
  const Language L = o.slot >= 0 ? s.lang[o.slot] : Language::English;
  const std::string head = s.deviceName + ": ";
  const int li = static_cast<int>(L);
  switch (o.result) {
    case CmdResult::NotAllowed: return head + kReply[li][kOwnerOnly];
    case CmdResult::BadCode: return head + kReply[li][kBad];
    case CmdResult::Full: return head + kReply[li][kFull];
    default: break;
  }
  switch (o.type) {
    case CmdType::SetStage: return head + "OK - " + stageName(s.stage, L);
    case CmdType::SetLanguage: return head + "OK - " + languageName(L);
    case CmdType::SetTolerance:
      return head + "OK - +/-" + fmt::signedTenths(s.tol.t, true) + "C +/-" + fmt::rh(s.tol.rh);
    case CmdType::AddNumber: return head + "OK - " + o.detail;
    case CmdType::SetWifi: return head + "OK - Wi-Fi " + s.wifiSsid;
    case CmdType::Status: {
      std::string r = head;
      r += st.sample.valid ? fmt::temp(st.sample.t) + " " + fmt::rh(st.sample.rh) : eventPhrase(EventType::SensorFault, false, L);
      r += std::string(" ") + stageName(s.stage, L);
      if (st.onBattery) r += std::string(" ") + eventPhrase(EventType::MainsFail, false, L) + " " + std::to_string(st.batteryPct) + "%";
      return r + " " + fmt::hhmm(st.now);
    }
    default: return head + "OK";
  }
}

}  // namespace seri
