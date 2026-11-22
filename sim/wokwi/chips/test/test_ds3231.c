#include <stdio.h>
#include <string.h>
#include "host_api_ctl.h"

static int unbcd(uint8_t b) { return (b >> 4) * 10 + (b & 0x0F); }
static void read_time(uint8_t* r) { uint8_t p = 0; host_i2c_write(&p, 1); host_i2c_read(r, 7); }

int main(void) {
  host_set_attr("start_epoch", 1795237200);  // 2026-11-21 05:00:00 UTC (Saturday)
  host_set_attr("speed", 60);
  chip_init();
  uint8_t r[7];
  read_time(r);
  CHECK(unbcd(r[0]) == 0 && unbcd(r[1]) == 0 && unbcd(r[2]) == 5, "05:00:00");
  CHECK(unbcd(r[4]) == 21 && unbcd(r[5] & 0x1F) == 11 && unbcd(r[6]) == 26, "21/11/26");
  CHECK(r[3] == 7, "day-of-week 7 = Saturday (1 = Sunday)");
  host_advance(10e9);  // 10 s simulated at x60 = 10 minutes
  read_time(r);
  CHECK(unbcd(r[1]) == 10 && unbcd(r[2]) == 5, "accelerated: 05:10");
  // set time to 2026-12-31 23:59:50
  uint8_t w[8] = {0x00, 0x50, 0x59, 0x23, 5, 0x31, 0x12, 0x26};
  host_i2c_write(w, 8);
  host_advance(1e9);  // +60 s
  read_time(r);
  CHECK(unbcd(r[6]) == 27 && unbcd(r[5] & 0x1F) == 1 && unbcd(r[4]) == 1, "rolled over to 01/01/27");
  CHECK(unbcd(r[1]) == 0 && unbcd(r[0]) == 50, "00:00:50");
  uint8_t sp = 0x0F, st;
  host_i2c_write(&sp, 1); host_i2c_read(&st, 1);
  CHECK((st & 0x80) == 0, "OSF clear while powered");
  host_set_attr("osf", 1);
  read_time(r);
  host_i2c_write(&sp, 1); host_i2c_read(&st, 1);
  CHECK((st & 0x80) != 0, "OSF set after coin cell loss");
  CHECK(unbcd(r[6]) == 0, "time reset to year 2000");
  uint8_t clr[2] = {0x0F, 0x00};
  host_i2c_write(clr, 2);
  host_i2c_write(&sp, 1); host_i2c_read(&st, 1);
  CHECK((st & 0x80) == 0, "firmware can clear OSF");
  fprintf(stderr, "ds3231: %d passed, %d failed\n", passes, fails);
  return fails != 0;
}
