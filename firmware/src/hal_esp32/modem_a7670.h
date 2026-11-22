#pragma once
// SIMCom A7670C driver (AT commands over UART2, 115200 8N1).
// SMS: text mode; GSM alphabet for plain-ASCII messages (160 chars), UCS2 hex for Indian scripts (70).
// MQTT: the module's built-in client over TLS (AT+CMQTT*), so cellular upload needs no TCP stack here.
// Command names follow the SIMCom "A76XX Series AT Command Manual"; responses must be re-verified
// on a real module (bench test plan) because the simulator chip was written from the same manual.
#include <Arduino.h>
#include <deque>
#include <string>
#include <utility>
#include "seri/hal.h"

class A7670Modem : public seri::hal::Modem {
 public:
  A7670Modem(HardwareSerial& s, int rx, int tx, int pwrKey) : s_(s), rx_(rx), tx_(tx), pwr_(pwrKey) {}
  void begin(const std::string& clientId);
  bool sendSms(const std::string& number10, const std::string& utf8) override;
  bool pollSms(std::string& from, std::string& body) override;
  bool networkTime(int64_t& utc) override;
  bool dataReady() override;
  bool mqttPublish(const std::string& topic, const std::string& payload) override;
  bool lastKnownRegistered() const { return reg_; }  // for the screen; no AT traffic

 private:
  bool readLine(String& line, uint32_t deadline);
  bool cmd(const String& c, uint32_t timeoutMs, String* resp = nullptr);
  // Waits for a line starting with `ok`; a line starting with `fail` (or ERROR) means failure.
  bool waitFor(const char* ok, const char* fail, uint32_t timeoutMs);
  bool waitPrompt(uint32_t timeoutMs);
  bool isUrc(const String& line);
  void drain();
  bool registered();
  bool ensureMqtt();
  bool alive();

  HardwareSerial& s_;
  int rx_, tx_, pwr_;
  std::string client_;
  bool reg_ = false, mqttUp_ = false, started_ = false;
  uint32_t regMs_ = 0, mqttTryMs_ = 0;
  std::deque<std::pair<std::string, std::string>> inbox_;
};
