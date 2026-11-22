#include "seri/profiles.h"

namespace seri {
namespace {
const Profile kProfiles[kStageCount] = {
#define PROFILE(stage, key, tMin, tMax, rhMin, rhMax, source) \
  {Stage::stage, key, tMin, tMax, rhMin, rhMax, source},
#include "stage_profiles.inc"
#undef PROFILE
};
Deci clamp(Deci v, Deci lo, Deci hi) { return v < lo ? lo : (v > hi ? hi : v); }
}  // namespace

const Profile& profileFor(Stage s) {
  int i = static_cast<int>(s);
  if (i < 0 || i >= kStageCount) i = 0;
  return kProfiles[i];
}

Band bandFor(Stage s, const Tolerance& tol) {
  const Profile& p = profileFor(s);
  return Band{p.tMin - tol.t, p.tMax + tol.t, clamp(p.rhMin - tol.rh, 0, 1000),
              clamp(p.rhMax + tol.rh, 0, 1000)};
}

bool toleranceValid(const Tolerance& tol) {
  return tol.t >= 5 && tol.t <= 30 && tol.rh >= 20 && tol.rh <= 150;
}

}  // namespace seri
