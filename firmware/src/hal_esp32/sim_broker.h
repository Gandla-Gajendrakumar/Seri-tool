#pragma once
// Wokwi simulation only: a stand-in broker so the upload scenarios run. Real devices are provisioned
// from seri.cfg + ca.pem on the microSD card (D-060); this file is not used in the hardware build.
#include "seri/provision.h"

inline seri::MqttConfig simBroker() {
  seri::MqttConfig c;
  c.host = "broker.sim.test";
  c.clientId = "seri-sim";
  c.caPem = "-----BEGIN CERTIFICATE-----\nSIMULATION-ONLY\n-----END CERTIFICATE-----\n";
  return c;
}
