#include "seri/provision.h"

#include <cstdlib>
#include <vector>

namespace seri {
namespace {
constexpr size_t kMaxField = 128;
const char* const kBegin = "-----BEGIN CERTIFICATE-----";
const char* const kEnd = "-----END CERTIFICATE-----";

std::string trim(const std::string& s) {
  size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
  return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}
bool hasSpace(const std::string& s) { return s.find_first_of(" \t") != std::string::npos; }

// length-prefixed fields: "<len>:<bytes>" ...
void put(std::string& out, const std::string& v) { out += std::to_string(v.size()) + ":" + v; }
bool get(const std::string& in, size_t& pos, std::string& v) {
  size_t colon = in.find(':', pos);
  if (colon == std::string::npos || colon == pos) return false;
  for (size_t i = pos; i < colon; ++i) if (in[i] < '0' || in[i] > '9') return false;
  size_t len = std::strtoul(in.c_str() + pos, nullptr, 10);
  if (colon + 1 + len > in.size()) return false;
  v = in.substr(colon + 1, len);
  pos = colon + 1 + len;
  return true;
}
}  // namespace

bool MqttConfig::valid() const { return !host.empty() && port > 0 && validPem(caPem); }

bool validPem(const std::string& pem) {
  if (pem.size() > kMaxPem) return false;
  size_t b = pem.find(kBegin), e = pem.find(kEnd);
  return b != std::string::npos && e != std::string::npos && e > b;
}

bool parseProvisioning(const std::string& text, MqttConfig& out, std::string& err) {
  MqttConfig c = out;
  c.host.clear();
  c.port = 8883;
  size_t start = 0;
  while (start <= text.size()) {
    size_t nl = text.find('\n', start);
    std::string line = trim(text.substr(start, nl == std::string::npos ? std::string::npos : nl - start));
    start = nl == std::string::npos ? text.size() + 1 : nl + 1;
    if (line.empty() || line[0] == '#') continue;
    size_t eq = line.find('=');
    if (eq == std::string::npos) { err = "bad line (no '='): " + line.substr(0, 20); return false; }
    std::string k = trim(line.substr(0, eq)), v = trim(line.substr(eq + 1));
    if (v.size() > kMaxField) { err = k + " too long"; return false; }
    if (k == "mqtt_host") {
      if (v.empty() || hasSpace(v)) { err = "mqtt_host invalid"; return false; }
      c.host = v;
    } else if (k == "mqtt_port") {
      char* end = nullptr;
      long p = std::strtol(v.c_str(), &end, 10);
      if (v.empty() || *end || p < 1 || p > 65535) { err = "mqtt_port invalid"; return false; }
      c.port = static_cast<int>(p);
    } else if (k == "mqtt_user") c.user = v;
    else if (k == "mqtt_pass") c.pass = v;
    else if (k == "client_id") {
      if (hasSpace(v)) { err = "client_id invalid"; return false; }
      c.clientId = v;
    } else { err = "unknown key: " + k; return false; }
  }
  if (c.host.empty()) { err = "mqtt_host missing"; return false; }
  out = c;
  return true;
}

std::string encodeMqtt(const MqttConfig& c) {
  std::string out = "M1;";
  for (const std::string* f : {&c.host, &c.user, &c.pass, &c.clientId, &c.caPem}) put(out, *f);
  put(out, std::to_string(c.port));
  return out;
}

bool decodeMqtt(const std::string& blob, MqttConfig& out) {
  if (blob.compare(0, 3, "M1;") != 0) return false;
  size_t pos = 3;
  MqttConfig c;
  std::string port;
  for (std::string* f : {&c.host, &c.user, &c.pass, &c.clientId, &c.caPem, &port})
    if (!get(blob, pos, *f)) return false;
  c.port = std::atoi(port.c_str());
  if (pos != blob.size() || c.port < 1 || c.port > 65535) return false;
  out = c;
  return true;
}

}  // namespace seri
