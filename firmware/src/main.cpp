// Placeholder entry point. Real firmware arrives in phase 2; phase 0 only proves the
// ESP32 cross-compiler works.
#include <Arduino.h>
void setup() { Serial.begin(115200); Serial.println("seri-tool phase0"); }
void loop() { delay(1000); }
