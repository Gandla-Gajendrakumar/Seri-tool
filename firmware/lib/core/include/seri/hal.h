#pragma once
// Hardware abstraction layer (requirement A-02). The application talks to hardware ONLY through
// these interfaces. Version 1 implements them for the ESP32 carrier board (src/hal_esp32/); the
// integrated board later gets its own implementations and the application ports unchanged.
// Native tests use fakes (test/fakes/). Every call must return quickly: no call may block the
// monitoring loop for more than a few hundred milliseconds.
#include <cstddef>
#include <cstdint>
#include <string>
#include "seri/sample.h"
#include "seri/types.h"

#include "seri/provision.h"

namespace seri {
namespace hal {

struct Clock {                       // DS3231 real-time clock
  virtual ~Clock() = default;
  virtual int64_t nowUtc() = 0;      // seconds since 1970, UTC
  virtual bool valid() = 0;          // false if the RTC lost time (oscillator-stop flag)
  virtual void set(int64_t utc) = 0;
};

struct Sensor {                      // SHT31 probe
  virtual ~Sensor() = default;
  virtual RawReading read() = 0;     // one single-shot measurement
};

struct Power {                       // mains-sense input and battery-voltage divider
  virtual ~Power() = default;
  virtual bool mainsPresent() = 0;
  virtual int32_t batteryMv() = 0;
};

struct Modem {                       // SIMCom A7670C
  virtual ~Modem() = default;
  virtual bool registered() = 0;                                  // on the cellular network (cached, cheap)
  virtual void configureMqtt(const MqttConfig& c) = 0;            // broker settings (D-060)
  virtual bool sendSms(const std::string& number10, const std::string& utf8) = 0;
  virtual bool pollSms(std::string& from, std::string& body) = 0;  // true if one was received
  virtual bool networkTime(int64_t& utc) = 0;                     // from the cellular network
  virtual bool dataReady() = 0;                                   // packet data + MQTT usable
  virtual bool mqttPublish(const std::string& topic, const std::string& payload) = 0;
};

struct Wifi {
  virtual ~Wifi() = default;
  virtual bool connected() = 0;
  virtual bool ntpTime(int64_t& utc) = 0;
  virtual void configureMqtt(const MqttConfig& c) = 0;
  virtual bool mqttPublish(const std::string& topic, const std::string& payload) = 0;
};

struct Printer {                     // 58 mm ESC/POS panel printer
  virtual ~Printer() = default;
  virtual bool paperPresent() = 0;
  virtual bool printLine(const std::string& utf8, Language lang) = 0;  // false = not printed
  virtual void setLowPower(bool on) = 0;  // fewer heating dots at once: lower peak current on battery (D-039)
};

struct Storage {                     // microSD: event log + store-and-forward upload queue
  virtual ~Storage() = default;
  virtual bool ok() = 0;
  virtual bool appendLog(const std::string& line) = 0;
  virtual bool queuePush(const std::string& record) = 0;
  virtual bool queueFront(std::string& record) = 0;
  virtual bool queuePop() = 0;
  virtual size_t queueSize() = 0;
  virtual bool readFile(const std::string& name, std::string& out) = 0;  // provisioning files (D-060)
  virtual bool removeFile(const std::string& name) = 0;
};

struct Indicators {                  // buzzer + green/red LEDs
  virtual ~Indicators() = default;
  virtual void buzzer(bool on) = 0;
  virtual void green(bool on) = 0;
  virtual void red(bool on) = 0;
};

// Settings and SMS-cap state survive power loss (ESP32 NVS flash). Blob = versioned bytes.
struct Persist {
  virtual ~Persist() = default;
  virtual bool load(const char* key, std::string& blob) = 0;
  virtual bool save(const char* key, const std::string& blob) = 0;
};

// Optional diagnostic echo of every log line (serial console on the ESP32; used by Wokwi scenarios).
struct Trace {
  virtual ~Trace() = default;
  virtual void line(const std::string& l) = 0;
};

}  // namespace hal
}  // namespace seri
