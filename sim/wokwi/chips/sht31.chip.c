// Virtual Sensirion SHT31 temperature/humidity sensor (I2C address 0x44).
// Implements what the firmware uses, per the SHT3x-DIS datasheet:
//   0x2400 single-shot measurement, high repeatability, no clock stretching
//   0x30A2 soft reset, 0xF32D read status
//   reply: T(MSB,LSB,CRC) RH(MSB,LSB,CRC); CRC-8 poly 0x31 init 0xFF
//   T = -45 + 175 * raw/65535 ; RH = 100 * raw/65535
// Controls (changed by scenarios): temperature (C), humidity (%), connected (0 = unplugged: NACK).
#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  uint32_t attr_t, attr_rh, attr_connected;
  uint8_t cmd[2];
  int cmd_len;
  uint8_t out[6];
  int out_len, out_pos;
} chip_t;

static uint8_t crc8(const uint8_t* d, int n) {
  uint8_t c = 0xFF;
  for (int i = 0; i < n; i++) {
    c ^= d[i];
    for (int b = 0; b < 8; b++) c = (c & 0x80) ? (uint8_t)((c << 1) ^ 0x31) : (uint8_t)(c << 1);
  }
  return c;
}

static void measure(chip_t* c) {
  float t = attr_read_float(c->attr_t), rh = attr_read_float(c->attr_rh);
  if (rh < 0) rh = 0;
  if (rh > 100) rh = 100;
  uint16_t rt = (uint16_t)((t + 45.0f) * 65535.0f / 175.0f + 0.5f);
  uint16_t rr = (uint16_t)(rh * 65535.0f / 100.0f + 0.5f);
  c->out[0] = rt >> 8; c->out[1] = rt & 0xFF; c->out[2] = crc8(c->out, 2);
  c->out[3] = rr >> 8; c->out[4] = rr & 0xFF; c->out[5] = crc8(c->out + 3, 2);
  c->out_len = 6;
  c->out_pos = 0;
}

static bool on_connect(void* ud, uint32_t address, bool read) {
  chip_t* c = ud;
  (void)address;
  if (!attr_read(c->attr_connected)) return false;  // unplugged probe: no ACK
  if (!read) c->cmd_len = 0;
  return true;
}

static uint8_t on_read(void* ud) {
  chip_t* c = ud;
  if (c->out_pos < c->out_len) return c->out[c->out_pos++];
  return 0xFF;
}

static bool on_write(void* ud, uint8_t data) {
  chip_t* c = ud;
  if (c->cmd_len < 2) c->cmd[c->cmd_len++] = data;
  if (c->cmd_len == 2) {
    uint16_t cmd = (uint16_t)(c->cmd[0] << 8 | c->cmd[1]);
    if (cmd == 0x2400) measure(c);
    else if (cmd == 0xF32D) {  // status register: all clear
      c->out[0] = 0; c->out[1] = 0; c->out[2] = crc8(c->out, 2);
      c->out_len = 3; c->out_pos = 0;
    } else if (cmd == 0x30A2) c->out_len = 0;
  }
  return true;
}

static void on_disconnect(void* ud) { (void)ud; }

void chip_init(void) {
  chip_t* c = calloc(1, sizeof(chip_t));
  c->attr_t = attr_init_float("temperature", 27.5f);
  c->attr_rh = attr_init_float("humidity", 85.0f);
  c->attr_connected = attr_init("connected", 1);
  const i2c_config_t cfg = {
      .user_data = c, .address = 0x44, .scl = pin_init("SCL", INPUT), .sda = pin_init("SDA", INPUT),
      .connect = on_connect, .read = on_read, .write = on_write, .disconnect = on_disconnect,
  };
  i2c_init(&cfg);
  printf("sht31: ready\n");
}
