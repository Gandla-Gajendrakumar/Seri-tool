#include "seri/sample.h"
#include <algorithm>
#include <cmath>

namespace seri {
namespace {
constexpr size_t kMaxReads = 16;
Deci toDeci(float v) { return static_cast<Deci>(std::lround(static_cast<double>(v) * 10.0)); }
float median(float* v, size_t n) {
  std::sort(v, v + n);
  return (n % 2) ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2.0f;
}
}  // namespace

Sample medianSample(const RawReading* reads, size_t n) {
  float ts[kMaxReads], rhs[kMaxReads];
  size_t k = 0;
  for (size_t i = 0; i < n && k < kMaxReads; ++i) {
    const RawReading& r = reads[i];
    if (!r.ok || !std::isfinite(r.t) || !std::isfinite(r.rh)) continue;
    if (r.t < -40.0f || r.t > 125.0f || r.rh < 0.0f || r.rh > 100.0f) continue;  // SHT31 range
    ts[k] = r.t;
    rhs[k] = r.rh;
    ++k;
  }
  if (k < 3) return Sample{false, 0, 0};
  return Sample{true, toDeci(median(ts, k)), toDeci(median(rhs, k))};
}

}  // namespace seri
