#pragma once
#include <Preferences.h>
#include "seri/hal.h"

class NvsPersist : public seri::hal::Persist {
 public:
  void begin() { prefs_.begin("seri", false); }
  bool load(const char* key, std::string& blob) override {
    size_t n = prefs_.getBytesLength(key);
    if (n == 0) return false;
    blob.resize(n);
    return prefs_.getBytes(key, &blob[0], n) == n;
  }
  bool save(const char* key, const std::string& blob) override {
    return prefs_.putBytes(key, blob.data(), blob.size()) == blob.size();
  }
 private:
  Preferences prefs_;
};
