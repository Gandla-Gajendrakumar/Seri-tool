#pragma once
// Byte encoding of Settings and AlertState for flash (R-08). Format: magic "SR", version byte,
// payload, CRC-32. Anything corrupt or from another version is rejected, and the caller falls back to
// defaults (settings) or keeps counting from zero (alerts).
#include <string>
#include "seri/alerts.h"
#include "seri/commands.h"

namespace seri {

std::string encodeSettings(const Settings& s);
bool decodeSettings(const std::string& blob, Settings& out);
std::string encodeAlertState(const AlertState& a);
bool decodeAlertState(const std::string& blob, AlertState& out);
uint32_t crc32(const std::string& data);

}  // namespace seri
