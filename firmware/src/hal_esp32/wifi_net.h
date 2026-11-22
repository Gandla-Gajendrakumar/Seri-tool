#pragma once
// Wi-Fi uplink (preferred over cellular for uploads) with NTP and MQTT over TLS (PubSubClient).
// In the Wokwi scenarios no Wi-Fi network is configured, so this path is compiled but not exercised
// (stated in the phase-2 evidence).
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <string>
#include "seri/hal.h"

class WifiNet : public seri::hal::Wifi {
 public:
  void configure(const std::string& ssid, const std::string& pass, const std::string& clientId);
  bool connected() override;
  bool ntpTime(int64_t& utc) override;
  bool mqttPublish(const std::string& topic, const std::string& payload) override;
 private:
  std::string ssid_, pass_, client_;
  WiFiClientSecure tls_;
  PubSubClient mqtt_{tls_};
  uint32_t lastTry_ = 0;
  bool ntpStarted_ = false;
};
