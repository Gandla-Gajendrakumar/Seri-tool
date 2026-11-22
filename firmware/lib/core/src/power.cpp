#include "seri/power.h"

namespace seri {
namespace {
struct Pt { int32_t mv; int pct; };
// PROVISIONAL generic Li-ion resting-voltage curve (see header).
const Pt kCurve[] = {{3000, 0}, {3300, 2}, {3400, 5}, {3500, 10}, {3600, 20}, {3700, 35},
                     {3800, 50}, {3900, 65}, {4000, 80}, {4100, 90}, {4200, 100}};
constexpr int kN = sizeof(kCurve) / sizeof(kCurve[0]);
}  // namespace

int batteryPercent(int32_t mv) {
  if (mv <= kCurve[0].mv) return 0;
  if (mv >= kCurve[kN - 1].mv) return 100;
  for (int i = 1; i < kN; ++i) {
    if (mv <= kCurve[i].mv) {
      const Pt a = kCurve[i - 1], b = kCurve[i];
      const int32_t num = (mv - a.mv) * (b.pct - a.pct) * 2;
      const int32_t den = (b.mv - a.mv);
      return a.pct + static_cast<int>((num / den + 1) / 2);  // round half up
    }
  }
  return 100;
}

int64_t uploadIntervalSec(bool onBattery) { return onBattery ? 900 : 300; }
bool otaAllowed(bool onBattery, int batteryPct) { return !onBattery && batteryPct >= 50; }

}  // namespace seri
