#pragma once
// SMS configuration commands (F-SMS-07, Q-6 answered: digits only, no English needed).
//
//   10 <PIN>            make this phone the owner (PIN printed on the device label)
//   11 <1-6>            stage: 1 eggs, 2 chawki, 3 instar III, 4 instar IV, 5 instar V, 6 spinning
//   12 <1-4>            my SMS language: 1 English, 2 Hindi, 3 Telugu, 4 Kannada
//   13 <t> <r>          tolerance: t in tenths of a degree (5-30), r in % (2-15). "13 10 5" = +/-1.0 C, +/-5 %
//   20                  send status now
//   21 <phone number>   add a number (owner only)
//   22 <1-2>            remove extra number 1 or 2 (owner only)
//   30 <ssid> <pass>    set Wi-Fi (owner only; installer use)
//   41 <0|1>            buzzer off / on
// Spaces, '*', '#' and ',' all work as separators so any feature phone can type them.
#include <cstdint>
#include <string>
#include "seri/alerts.h"
#include "seri/profiles.h"
#include "seri/types.h"

namespace seri {

enum class CmdType : uint8_t {
  Invalid, ClaimOwner, SetStage, SetLanguage, SetTolerance, Status, AddNumber, RemoveNumber, SetWifi, Buzzer
};

struct Command {
  CmdType type = CmdType::Invalid;
  int a = 0, b = 0;
  std::string s1, s2;  // PIN / phone number / Wi-Fi name and password
};

Command parseCommand(const std::string& body);
// "+91 98765-43210", "09876543210", "919876543210" -> "9876543210". "" if not an Indian mobile.
std::string normalizeNumber(const std::string& raw);

struct Settings {
  std::string phone[kMaxRecipients];  // slot 0 = owner; "" = empty
  Language lang[kMaxRecipients] = {Language::English, Language::English, Language::English};
  Stage stage = Stage::Chawki;
  Tolerance tol;
  bool buzzerOn = true;
  std::string deviceName = "Shed-1";
  std::string pin = "000000";  // set at production, printed on the label
  std::string wifiSsid, wifiPass;
  uint8_t pinFailuresToday = 0;  // reset daily by the caller
};

uint8_t recipientMask(const Settings& s);
int slotOf(const Settings& s, const std::string& number);  // -1 if not registered

enum class CmdResult : uint8_t { Ok, Ignored, BadCode, NotAllowed, Full };
struct CmdOutcome {
  CmdResult result;
  CmdType type;
  int slot;  // sender's slot after the command (-1 unknown)
  std::string detail = {};  // e.g. the number that was added
};

// Checks who sent it and applies it to the settings. Unknown senders are ignored silently
// (no reply, no cost), except a correct owner PIN. 5 wrong PINs a day lock PIN claims.
CmdOutcome applyCommand(Settings& s, const std::string& fromRaw, const std::string& body);

struct StatusInfo {
  Sample sample;
  bool onBattery;
  int batteryPct;
  int64_t now;
};
// Reply text in the sender's language; empty when no reply should be sent.
std::string formatReply(const CmdOutcome& o, const Settings& s, const StatusInfo& st);

}  // namespace seri
