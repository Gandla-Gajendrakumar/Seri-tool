#pragma once
// Stage climate profiles. The VALUES live in exactly one place: src/stage_profiles.inc.
#include "seri/types.h"

namespace seri {

struct Profile {
  Stage stage;
  const char* key;     // short ASCII id, e.g. "chawki"
  Deci tMin, tMax;     // target temperature range, deci-degC
  Deci rhMin, rhMax;   // target humidity range, deci-%RH
  const char* source;  // where the figures come from
};

struct Tolerance {
  Deci t = 10;   // +/-1.0 C   (default from brief)
  Deci rh = 50;  // +/-5 %RH   (default from brief)
};

// Alarm band = target range widened by the tolerance on both sides (Q-3, answered).
struct Band {
  Deci tLo, tHi, rhLo, rhHi;
};

const Profile& profileFor(Stage s);
Band bandFor(Stage s, const Tolerance& tol);
// Limits accepted from buttons/SMS for the tolerance.
bool toleranceValid(const Tolerance& tol);

}  // namespace seri
