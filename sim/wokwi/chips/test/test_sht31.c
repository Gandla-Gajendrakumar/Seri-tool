#include <math.h>
#include <stdio.h>
#include <string.h>
#include "host_api_ctl.h"

static uint8_t crc8(const uint8_t* d, int n) {  // independent copy of the datasheet CRC
  uint8_t c = 0xFF;
  for (int i = 0; i < n; i++) { c ^= d[i]; for (int b = 0; b < 8; b++) c = (c & 0x80) ? (uint8_t)((c << 1) ^ 0x31) : (uint8_t)(c << 1); }
  return c;
}

int main(void) {
  host_set_attr("temperature", 31.2f);
  host_set_attr("humidity", 92.5f);
  host_set_attr("connected", 1);
  chip_init();
  uint8_t cmd[2] = {0x24, 0x00}, r[6];
  CHECK(host_i2c_write(cmd, 2), "single-shot command ACKed");
  CHECK(host_i2c_read(r, 6), "6 bytes read");
  CHECK(crc8(r, 2) == r[2] && crc8(r + 3, 2) == r[5], "CRC bytes valid");
  float t = -45.0f + 175.0f * (float)(r[0] << 8 | r[1]) / 65535.0f;
  float rh = 100.0f * (float)(r[3] << 8 | r[4]) / 65535.0f;
  CHECK(fabsf(t - 31.2f) < 0.01f, "temperature decodes to 31.2");
  CHECK(fabsf(rh - 92.5f) < 0.01f, "humidity decodes to 92.5");
  // datasheet check value: CRC of 0xBEEF = 0x92
  uint8_t be[2] = {0xBE, 0xEF};
  CHECK(crc8(be, 2) == 0x92, "CRC-8 datasheet example 0xBEEF -> 0x92");
  host_set_attr("temperature", -10.0f);
  host_i2c_write(cmd, 2); host_i2c_read(r, 6);
  t = -45.0f + 175.0f * (float)(r[0] << 8 | r[1]) / 65535.0f;
  CHECK(fabsf(t + 10.0f) < 0.01f, "negative temperature");
  host_set_attr("connected", 0);
  CHECK(!host_i2c_write(cmd, 2), "unplugged probe does not ACK");
  fprintf(stderr, "sht31: %d passed, %d failed\n", passes, fails);
  return fails != 0;
}
