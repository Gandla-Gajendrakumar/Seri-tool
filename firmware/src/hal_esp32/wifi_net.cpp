#include "wifi_net.h"
#include <time.h>
#include "board.h"
#include "ca_cert.h"

void WifiNet::configure(const std::string& ssid, const std::string& pass, const std::string& clientId) {
  if (ssid == ssid_ && pass == pass_) return;
  ssid_ = ssid;
  pass_ = pass;
  client_ = clientId;
  WiFi.disconnect(true);
  if (ssid_.empty()) { WiFi.mode(WIFI_OFF); return; }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_.c_str(), pass_.c_str());
  tls_.setCACert(kMqttCaPem);
  mqtt_.setServer(cfg::kMqttHost, cfg::kMqttPort);
  mqtt_.setBufferSize(2048);
}

bool WifiNet::connected() { return !ssid_.empty() && WiFi.status() == WL_CONNECTED; }

bool WifiNet::ntpTime(int64_t& utc) {
  if (!connected()) return false;
  if (!ntpStarted_) { configTime(0, 0, "pool.ntp.org", "time.google.com"); ntpStarted_ = true; }
  const time_t t = time(nullptr);
  if (t < 1700000000) return false;
  utc = static_cast<int64_t>(t);
  return true;
}

bool WifiNet::mqttPublish(const std::string& topic, const std::string& payload) {
  if (!connected()) return false;
  if (!mqtt_.connected()) {
    if (lastTry_ != 0 && millis() - lastTry_ < 30000) return false;
    lastTry_ = millis();
    if (!mqtt_.connect(client_.c_str())) return false;
  }
  mqtt_.loop();
  return mqtt_.publish(topic.c_str(), payload.c_str());
}
