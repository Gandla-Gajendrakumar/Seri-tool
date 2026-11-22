#pragma once
// Shared value types. Temperatures and humidities are stored as integers in TENTHS
// (deci-degrees C, deci-percent RH) so comparisons and hysteresis are exact. 25.3 C == 253.
#include <cstdint>

namespace seri {

using Deci = int32_t;
constexpr Deci deci(double v) { return static_cast<Deci>(v * 10.0 + (v >= 0 ? 0.5 : -0.5)); }

enum class Stage : uint8_t { Incubation = 0, Chawki, Instar3, Instar4, Instar5, Spinning, Count };
constexpr int kStageCount = static_cast<int>(Stage::Count);

enum class Language : uint8_t { English = 0, Hindi, Telugu, Kannada, Count };
constexpr int kLanguageCount = static_cast<int>(Language::Count);

// One 30-second sample (median of 5 raw reads).
struct Sample {
  bool valid;
  Deci t;   // deci-degC
  Deci rh;  // deci-%RH
};

}  // namespace seri
