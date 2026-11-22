#pragma once
#include <Arduino.h>
#include "seri/hal.h"

class SerialTrace : public seri::hal::Trace {
 public:
  void line(const std::string& l) override { Serial.print("LOG "); Serial.println(l.c_str()); }
};
