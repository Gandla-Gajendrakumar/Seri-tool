// Phase 1 entry point. Real firmware (HAL, tasks, drivers) arrives in phase 2.
// For now this proves the pure logic in lib/core compiles and runs on the ESP32 itself.
#include <Arduino.h>
#include <vector>
#include "seri/events.h"
#include "seri/sms_text.h"

void setup() {
  Serial.begin(115200);
  seri::EventEngine engine(seri::bandFor(seri::Stage::Chawki, seri::Tolerance{}));
  std::vector<seri::Event> ev;
  for (int i = 0; i <= 10; ++i) engine.onSample(i * 30, seri::Sample{true, 305, 850}, ev);
  seri::SmsJob job{0, seri::SmsKind::Start, seri::EventType::TempHigh, 300, 305, 300};
  seri::SmsContext ctx{"Shed-1", seri::Stage::Chawki, engine.band(), seri::Language::Telugu, 300, 305, 100};
  Serial.printf("seri-tool phase1: %u event(s); SMS: %s\n", static_cast<unsigned>(ev.size()),
                seri::formatSms(job, ctx).c_str());
}
void loop() { delay(1000); }
