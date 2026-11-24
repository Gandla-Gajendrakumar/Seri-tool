#pragma once
// Per-device MQTT broker settings (F-UP-04, D-060). The installer copies two files to the microSD card:
//   seri.cfg   key=value lines: mqtt_host (required), mqtt_port (default 8883), mqtt_user, mqtt_pass,
//              client_id ('#' comments and blank lines allowed; unknown keys are refused to catch typos)
//   ca.pem     the broker's CA certificate (PEM, <= 4096 bytes)
// At boot the Monitor imports them into flash and deletes both files, so the password does not stay on a
// removable card. The password is never logged.
#include <string>

namespace seri {

struct MqttConfig {
  std::string host;
  int port = 8883;
  std::string user, pass, clientId, caPem;
  bool valid() const;  // host set and a CA certificate present (TLS is mandatory)
};

constexpr const char* kProvisionFile = "seri.cfg";
constexpr const char* kCaFile = "ca.pem";
constexpr size_t kMaxPem = 4096;

// Parses seri.cfg into `out` (CA not included). On failure `out` is unchanged and `err` says why.
bool parseProvisioning(const std::string& text, MqttConfig& out, std::string& err);
bool validPem(const std::string& pem);
std::string encodeMqtt(const MqttConfig& c);  // flash blob
bool decodeMqtt(const std::string& blob, MqttConfig& out);

}  // namespace seri
