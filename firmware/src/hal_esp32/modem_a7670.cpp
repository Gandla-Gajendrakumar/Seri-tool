#include "modem_a7670.h"
#include "board.h"
#include "ca_cert.h"
#include "seri/time_util.h"

namespace {
uint32_t regCacheMs() { return cfg::kSim ? 1500 : 10000; }
uint32_t mqttRetryMs() { return cfg::kSim ? 2000 : 30000; }

bool gsmSafe(const std::string& s) {
  for (unsigned char c : s) {
    if (c < 0x20 || c > 0x7E) return false;
    if (c == '[' || c == ']' || c == '{' || c == '}' || c == '\\' || c == '^' || c == '~' || c == '|' || c == '`') return false;
  }
  return true;
}
// UTF-8 -> UCS2 big-endian hex, 4 hex digits per character (BMP only, which covers all Indian scripts)
String ucs2Hex(const std::string& s) {
  String out;
  static const char* hx = "0123456789ABCDEF";
  for (size_t i = 0; i < s.size();) {
    unsigned char c = static_cast<unsigned char>(s[i]);
    uint32_t cp;
    int n;
    if (c < 0x80) { cp = c; n = 1; }
    else if ((c >> 5) == 6) { cp = c & 0x1F; n = 2; }
    else if ((c >> 4) == 14) { cp = c & 0x0F; n = 3; }
    else { cp = '?'; n = 4; }
    for (int k = 1; k < n && i + k < s.size(); ++k) cp = (cp << 6) | (static_cast<unsigned char>(s[i + k]) & 0x3F);
    if (cp > 0xFFFF) cp = '?';
    for (int sh = 12; sh >= 0; sh -= 4) out += hx[(cp >> sh) & 0xF];
    i += n;
  }
  return out;
}
}  // namespace

void A7670Modem::begin(const std::string& clientId) {
  client_ = clientId;
  s_.begin(115200, SERIAL_8N1, rx_, tx_);
  pinMode(pwr_, OUTPUT);
  digitalWrite(pwr_, HIGH);
  if (!alive()) {  // power-on pulse on PWRKEY (low for >= 50 ms per hardware design guide; 1 s used)
    digitalWrite(pwr_, LOW);
    delay(1000);
    digitalWrite(pwr_, HIGH);
    for (int i = 0; i < 20 && !alive(); ++i) delay(500);
  }
  cmd("ATE0", 1000);
  cmd("AT+CMGF=1", 1000);
  cmd("AT+CNMI=2,1,0,0,0", 1000);  // store new SMS, notify with +CMTI
  cmd("AT+CTZU=1", 1000);          // take time zone / time from the network
}

bool A7670Modem::alive() { return cmd("AT", 500); }

bool A7670Modem::readLine(String& line, uint32_t deadline) {
  line = "";
  while (static_cast<int32_t>(deadline - millis()) > 0) {
    while (s_.available()) {
      char ch = static_cast<char>(s_.read());
      if (ch == '\n') {
        line.trim();
        if (line.length()) return true;
        continue;
      }
      if (ch != '\r') line += ch;
    }
    delay(1);
  }
  return false;
}

bool A7670Modem::isUrc(const String& line) {
  if (line.startsWith("+CMTI:")) return true;  // new SMS: picked up at the next poll
  if (line.startsWith("+CMQTTCONNLOST")) { mqttUp_ = false; return true; }
  if (line.startsWith("+CMQTTSTART:")) { started_ = true; return true; }
  return false;
}

void A7670Modem::drain() {
  String l;
  while (s_.available() && readLine(l, millis() + 20)) isUrc(l);
}

bool A7670Modem::cmd(const String& c, uint32_t timeoutMs, String* resp) {
  drain();
  s_.print(c);
  s_.print("\r");
  const uint32_t deadline = millis() + timeoutMs;
  String line;
  while (readLine(line, deadline)) {
    if (line == "OK") return true;
    if (line == "ERROR" || line.startsWith("+CME ERROR") || line.startsWith("+CMS ERROR")) return false;
    if (line == c) continue;  // echo (before ATE0 takes effect)
    if (isUrc(line)) continue;
    if (resp) { *resp += line; *resp += "\n"; }
  }
  return false;
}

bool A7670Modem::waitFor(const char* ok, const char* fail, uint32_t timeoutMs) {
  const uint32_t deadline = millis() + timeoutMs;
  String line;
  while (readLine(line, deadline)) {
    if (line.startsWith(ok)) return true;
    if (line == "ERROR" || line.startsWith("+CME ERROR") || line.startsWith("+CMS ERROR")) return false;
    if (fail && line.startsWith(fail)) return false;
    isUrc(line);
  }
  return false;
}

bool A7670Modem::waitPrompt(uint32_t timeoutMs) {
  const uint32_t deadline = millis() + timeoutMs;
  String line;
  while (static_cast<int32_t>(deadline - millis()) > 0) {
    while (s_.available()) {
      char ch = static_cast<char>(s_.read());
      if (ch == '>') return true;
      if (ch == '\n') {
        line.trim();
        if (line == "ERROR" || line.startsWith("+CMS ERROR") || line.startsWith("+CME ERROR")) return false;
        line = "";
      } else if (ch != '\r') {
        line += ch;
      }
    }
    delay(1);
  }
  return false;
}

bool A7670Modem::registered() {
  if (regMs_ != 0 && millis() - regMs_ < regCacheMs()) return reg_;
  regMs_ = millis();
  auto check = [this](const char* c) {
    String r;
    if (!cmd(c, 1000, &r)) return false;
    int comma = r.indexOf(',');
    if (comma < 0) return false;
    int stat = r.substring(comma + 1).toInt();
    return stat == 1 || stat == 5;  // registered home / roaming
  };
  reg_ = check("AT+CEREG?") || check("AT+CREG?");  // LTE, else GSM fallback
  if (!reg_) mqttUp_ = false;
  return reg_;
}

bool A7670Modem::sendSms(const std::string& number10, const std::string& utf8) {
  if (!registered()) return false;
  const bool gsm = gsmSafe(utf8);
  cmd(gsm ? "AT+CSCS=\"GSM\"" : "AT+CSCS=\"UCS2\"", 1000);
  cmd(gsm ? "AT+CSMP=17,167,0,0" : "AT+CSMP=17,167,0,8", 1000);  // DCS 0 = GSM 7-bit, 8 = UCS2
  const std::string num = "+91" + number10;
  drain();
  s_.print("AT+CMGS=\"");
  s_.print(gsm ? String(num.c_str()) : ucs2Hex(num));
  s_.print("\"\r");
  if (!waitPrompt(5000)) return false;
  if (gsm) s_.print(utf8.c_str());
  else s_.print(ucs2Hex(utf8));
  s_.write(static_cast<uint8_t>(0x1A));
  const bool ok = waitFor("+CMGS:", nullptr, cfg::kSim ? 5000 : 60000);
  if (ok) waitFor("OK", nullptr, 1000);
  return ok;
}

bool A7670Modem::pollSms(std::string& from, std::string& body) {
  if (inbox_.empty()) {
    if (!registered()) return false;
    cmd("AT+CSCS=\"GSM\"", 1000);
    String r;
    if (!cmd("AT+CMGL=\"REC UNREAD\"", 5000, &r)) return false;
    // +CMGL: <idx>,"REC UNREAD","<from>",...\n<text>\n
    int pos = 0;
    while ((pos = r.indexOf("+CMGL:", pos)) >= 0) {
      int q1 = r.indexOf('"', r.indexOf(',', r.indexOf(',', pos) + 1));
      int q2 = q1 >= 0 ? r.indexOf('"', q1 + 1) : -1;
      int eol = r.indexOf('\n', pos);
      int eot = eol >= 0 ? r.indexOf('\n', eol + 1) : -1;
      if (q1 < 0 || q2 < 0 || eol < 0) break;
      String sender = r.substring(q1 + 1, q2);
      String text = eot >= 0 ? r.substring(eol + 1, eot) : r.substring(eol + 1);
      inbox_.push_back({sender.c_str(), text.c_str()});
      pos = eol;
    }
    if (!inbox_.empty()) cmd("AT+CMGD=0,1", 5000);  // delete messages already read
  }
  if (inbox_.empty()) return false;
  from = inbox_.front().first;
  body = inbox_.front().second;
  inbox_.pop_front();
  return true;
}

bool A7670Modem::networkTime(int64_t& utc) {
  if (!registered()) return false;
  String r;
  if (!cmd("AT+CCLK?", 1000, &r)) return false;
  int q = r.indexOf('"');  // "yy/MM/dd,hh:mm:ss+zz"
  if (q < 0 || r.length() < static_cast<unsigned>(q) + 21) return false;
  String t = r.substring(q + 1);
  int yy = t.substring(0, 2).toInt(), mo = t.substring(3, 5).toInt(), dd = t.substring(6, 8).toInt();
  int hh = t.substring(9, 11).toInt(), mi = t.substring(12, 14).toInt(), ss = t.substring(15, 17).toInt();
  int tz = t.substring(18, 20).toInt() * (t[17] == '-' ? -1 : 1);  // quarter hours
  if (yy < 24) return false;  // module clock not yet set by the network
  utc = seri::fromUtc(seri::CivilTime{2000 + yy, mo, dd, hh, mi, ss}) - tz * 900;
  return true;
}

bool A7670Modem::ensureMqtt() {
  if (mqttUp_) return true;
  if (mqttTryMs_ != 0 && millis() - mqttTryMs_ < mqttRetryMs()) return false;
  mqttTryMs_ = millis();
  if (!started_) {
    drain();
    s_.print("AT+CMQTTSTART\r");
    started_ = waitFor("+CMQTTSTART: 0", "+CMQTTSTART:", 5000);
    if (!started_) return false;
  }
  String certs;
  cmd("AT+CCERTLIST", 2000, &certs);
  if (certs.indexOf("ca.pem") < 0) {
    drain();
    s_.printf("AT+CCERTDOWN=\"ca.pem\",%u\r", static_cast<unsigned>(strlen(kMqttCaPem)));
    if (!waitPrompt(2000)) return false;
    s_.print(kMqttCaPem);
    if (!waitFor("OK", nullptr, 5000)) return false;
  }
  cmd("AT+CSSLCFG=\"sslversion\",0,4", 1000);  // TLS 1.2
  cmd("AT+CSSLCFG=\"authmode\",0,1", 1000);    // verify the server certificate
  cmd("AT+CSSLCFG=\"cacert\",0,\"ca.pem\"", 1000);
  cmd(String("AT+CMQTTACCQ=0,\"") + client_.c_str() + "\",1", 2000);
  cmd("AT+CMQTTSSLCFG=0,0", 1000);
  drain();
  s_.printf("AT+CMQTTCONNECT=0,\"tcp://%s:%d\",60,1\r", cfg::kMqttHost, cfg::kMqttPort);
  mqttUp_ = waitFor("+CMQTTCONNECT: 0,0", "+CMQTTCONNECT:", cfg::kSim ? 5000 : 30000);
  return mqttUp_;
}

bool A7670Modem::dataReady() { return registered() && ensureMqtt(); }

bool A7670Modem::mqttPublish(const std::string& topic, const std::string& payload) {
  if (!mqttUp_) return false;
  auto sendRaw = [this](const char* what, const std::string& data) {
    drain();
    s_.printf("AT+CMQTT%s=0,%u\r", what, static_cast<unsigned>(data.size()));
    if (!waitPrompt(2000)) return false;
    s_.write(reinterpret_cast<const uint8_t*>(data.data()), data.size());
    return waitFor("OK", nullptr, 3000);
  };
  if (!sendRaw("TOPIC", topic) || !sendRaw("PAYLOAD", payload)) { mqttUp_ = false; return false; }
  drain();
  s_.print("AT+CMQTTPUB=0,1,60\r");
  const bool ok = waitFor("+CMQTTPUB: 0,0", "+CMQTTPUB:", cfg::kSim ? 5000 : 15000);
  if (!ok) mqttUp_ = false;
  return ok;
}
