// Carrier-board sense lines: mains-present output, battery divider voltage, SD card-detect.
#include <math.h>
#include <stdio.h>
#include "host_api_ctl.h"

int main(void) {
  host_set_attr("mains", 1);
  host_set_attr("batteryMv", 4100);
  host_set_attr("sdPresent", 1);
  chip_init();
  host_advance(20e6);
  int mains = host_pin("MAINS_OK"), batt = host_pin("BATT_SENSE"), cd = host_pin("SD_CD");
  CHECK(mains >= 0 && batt >= 0 && cd >= 0, "pins exist");
  CHECK(host_pin_level[mains] == 1, "mains present -> MAINS_OK high");
  CHECK(fabsf(host_dac[batt] - 2.05f) < 0.001f, "4100 mV through 1:2 divider -> 2.05 V");
  CHECK(host_pin_level[cd] == 0, "card inserted -> SD_CD low (switch closed to GND)");
  host_set_attr("mains", 0);
  host_set_attr("batteryMv", 3350);
  host_set_attr("sdPresent", 0);
  host_advance(20e6);
  CHECK(host_pin_level[mains] == 0, "mains cut -> MAINS_OK low");
  CHECK(fabsf(host_dac[batt] - 1.675f) < 0.001f, "3350 mV -> 1.675 V");
  CHECK(host_pin_level[cd] == 1, "card removed -> SD_CD high (pull-up)");
  fprintf(stderr, "carrier: %d passed, %d failed\n", passes, fails);
  return fails != 0;
}
