// Validates provisioning files with the firmware's own parser (seri/provision.h), so a generated
// seri.cfg / ca.pem is accepted by the device exactly as the device would accept it.
// Usage: check_cfg seri.cfg ca.pem      exit 0 = valid
#include <cstdio>
#include <fstream>
#include <sstream>
#include "seri/provision.h"

static bool slurp(const char* p, std::string& out) {
  std::ifstream f(p, std::ios::binary);
  if (!f) return false;
  std::stringstream ss;
  ss << f.rdbuf();
  out = ss.str();
  return true;
}

int main(int argc, char** argv) {
  if (argc != 3) { std::fprintf(stderr, "usage: check_cfg seri.cfg ca.pem\n"); return 2; }
  std::string cfg, pem, err;
  seri::MqttConfig c;
  if (!slurp(argv[1], cfg) || !seri::parseProvisioning(cfg, c, err)) { std::printf("REJECTED %s: %s\n", argv[1], err.c_str()); return 1; }
  if (!slurp(argv[2], pem) || !seri::validPem(pem)) { std::printf("REJECTED %s: not a PEM certificate (<= 4096 bytes)\n", argv[2]); return 1; }
  c.caPem = pem;
  std::printf("OK host=%s port=%d client_id=%s user=%s\n", c.host.c_str(), c.port, c.clientId.c_str(), c.user.c_str());
  return c.valid() ? 0 : 1;
}
