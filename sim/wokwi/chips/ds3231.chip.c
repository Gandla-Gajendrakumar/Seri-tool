// Virtual Maxim DS3231 real-time clock (I2C address 0x68), per the DS3231 datasheet register map:
//   0x00-0x06 seconds, minutes, hours (24 h), day, date, month, year (BCD)
//   0x0E control, 0x0F status (bit 7 = OSF, oscillator stopped), 0x11-0x12 temperature
// Extra for simulation:
//   attr  start_epoch : initial time (UTC seconds) when the simulation starts
//   attr  speed       : clock runs this many times faster than simulated time (accelerated tests)
//   ctrl  osf         : set to 1 to simulate a dead coin cell (time lost, OSF set)
#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  uint32_t attr_osf;
  double base_epoch;    // epoch at base_nanos
  double base_nanos;
  double speed;
  bool osf;
  uint8_t ptr;
  bool got_ptr;
  uint8_t latched[7];   // time registers latched at the start of a read
  uint8_t control, status;
  uint8_t wbuf[19];
  bool write_time;
} chip_t;

static uint8_t bcd(int v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }
static int unbcd(uint8_t b) { return (b >> 4) * 10 + (b & 0x0F); }

// days since 1970-01-01 <-> civil date (Howard Hinnant's algorithms)
static void civil_from_days(long long z, int* y, int* m, int* d) {
  z += 719468;
  long long era = (z >= 0 ? z : z - 146096) / 146097;
  unsigned doe = (unsigned)(z - era * 146097);
  unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  unsigned mp = (5 * doy + 2) / 153;
  *d = (int)(doy - (153 * mp + 2) / 5 + 1);
  *m = (int)(mp < 10 ? mp + 3 : mp - 9);
  *y = (int)(yoe + era * 400 + (*m <= 2));
}
static long long days_from_civil(int y, int m, int d) {
  y -= m <= 2;
  long long era = (y >= 0 ? y : y - 399) / 400;
  unsigned yoe = (unsigned)(y - era * 400);
  unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + (long long)doe - 719468;
}

static double now_epoch(chip_t* c) {
  if (attr_read(c->attr_osf) && !c->osf) {  // coin cell "removed": time resets, OSF set
    c->osf = true;
    c->status |= 0x80;
    c->base_epoch = 946684800.0;  // 2000-01-01, what a reset DS3231 reads
    c->base_nanos = get_sim_nanos_d();
  }
  return c->base_epoch + (get_sim_nanos_d() - c->base_nanos) / 1e9 * c->speed;
}

static void latch(chip_t* c) {
  long long e = (long long)now_epoch(c);
  long long days = e / 86400, secs = e % 86400;
  int y, m, d;
  civil_from_days(days, &y, &m, &d);
  c->latched[0] = bcd((int)(secs % 60));
  c->latched[1] = bcd((int)(secs / 60 % 60));
  c->latched[2] = bcd((int)(secs / 3600));  // 24-hour mode
  c->latched[3] = (uint8_t)(((days + 4) % 7) + 1);  // 1970-01-01 was Thursday; 1 = Sunday
  c->latched[4] = bcd(d);
  c->latched[5] = (uint8_t)(bcd(m) | (y >= 2100 ? 0x80 : 0));
  c->latched[6] = bcd(y % 100);
}

static uint8_t reg_read(chip_t* c, uint8_t r) {
  if (r <= 6) return c->latched[r];
  if (r == 0x0E) return c->control;
  if (r == 0x0F) return c->status;
  if (r == 0x11) return 25;  // temperature MSB: 25 C
  return 0;
}

static bool on_connect(void* ud, uint32_t address, bool read) {
  chip_t* c = ud;
  (void)address;
  if (read) latch(c);
  else { c->got_ptr = false; c->write_time = false; }
  return true;
}

static uint8_t on_read(void* ud) {
  chip_t* c = ud;
  uint8_t v = reg_read(c, c->ptr);
  c->ptr = (uint8_t)((c->ptr + 1) % 0x13);
  return v;
}

static bool on_write(void* ud, uint8_t data) {
  chip_t* c = ud;
  if (!c->got_ptr) { c->ptr = data; c->got_ptr = true; return true; }
  if (c->ptr <= 6) { c->wbuf[c->ptr] = data; if (c->ptr == 6) c->write_time = true; }
  else if (c->ptr == 0x0E) c->control = data;
  else if (c->ptr == 0x0F) c->status = (uint8_t)(c->status & data & 0x80) | (data & 0x7F);  // OSF only clears
  c->ptr = (uint8_t)((c->ptr + 1) % 0x13);
  return true;
}

static void on_disconnect(void* ud) {
  chip_t* c = ud;
  if (!c->write_time) return;
  int y = 2000 + unbcd(c->wbuf[6]) + ((c->wbuf[5] & 0x80) ? 100 : 0);
  long long days = days_from_civil(y, unbcd(c->wbuf[5] & 0x1F), unbcd(c->wbuf[4]));
  c->base_epoch = (double)(days * 86400 + unbcd(c->wbuf[2] & 0x3F) * 3600 + unbcd(c->wbuf[1]) * 60 + unbcd(c->wbuf[0]));
  c->base_nanos = get_sim_nanos_d();
  c->write_time = false;
  printf("ds3231: time set to %lld\n", (long long)c->base_epoch);
}

void chip_init(void) {
  chip_t* c = calloc(1, sizeof(chip_t));
  c->attr_osf = attr_init("osf", 0);
  c->base_epoch = (double)attr_read(attr_init("start_epoch", 1795237200));  // 2026-11-21 05:00 UTC = 10:30 IST
  c->speed = (double)attr_read(attr_init("speed", 1));
  c->base_nanos = get_sim_nanos_d();
  const i2c_config_t cfg = {
      .user_data = c, .address = 0x68, .scl = pin_init("SCL", INPUT), .sda = pin_init("SDA", INPUT),
      .connect = on_connect, .read = on_read, .write = on_write, .disconnect = on_disconnect,
  };
  i2c_init(&cfg);
  printf("ds3231: ready, speed x%d\n", (int)c->speed);
}
