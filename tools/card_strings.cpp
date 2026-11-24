// Dumps the firmware's own stage and language names as JSON, so the farmer card can never disagree with
// what the device accepts and replies (used by tools/gen_card.py).
#include <cstdio>
#include "seri/commands.h"
#include "seri/sms_text.h"
using namespace seri;
int main() {
  std::printf("{\"stages\":[");
  for (int s = 0; s < kStageCount; ++s) {
    std::printf("%s[", s ? "," : "");
    for (int l = 0; l < kLanguageCount; ++l)
      std::printf("%s\"%s\"", l ? "," : "", stageName(static_cast<Stage>(s), static_cast<Language>(l)));
    std::printf("]");
  }
  std::printf("],\"languages\":[");
  for (int l = 0; l < kLanguageCount; ++l) std::printf("%s\"%s\"", l ? "," : "", languageName(static_cast<Language>(l)));
  std::printf("]}\n");
}
