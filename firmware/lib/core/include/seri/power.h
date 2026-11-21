#pragma once
// Power-dependent behaviour (P-05, F-OTA-03).
#include <cstdint>

namespace seri {

// Rough state of charge from resting cell voltage. PROVISIONAL generic Li-ion curve; replaced by
// the chosen 18650 cell's datasheet discharge curve in phase 4.
int batteryPercent(int32_t millivolts);
// MQTT upload interval: 5 min on mains, 15 min on battery. Alerts are never slowed down.
int64_t uploadIntervalSec(bool onBattery);
// Firmware updates only on mains with at least 50 % battery, so a power cut mid-update can't brick it.
bool otaAllowed(bool onBattery, int batteryPct);

}  // namespace seri
