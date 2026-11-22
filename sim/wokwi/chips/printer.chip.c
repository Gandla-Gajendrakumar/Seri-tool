// Virtual 58 mm TTL thermal panel printer (ESC/POS subset), 9600 8N1.
// Every completed text line is printed as      "PRINTER| <text>"
// Raster images (GS v 0, used for Indian scripts) are printed as "PRINTER| [IMG WxH]" followed by
// a downscaled text-art rendering, so the receipt can be read from the simulation log.
// Commands understood: ESC @ (init), LF, ESC ! n, ESC E n, ESC a n, ESC d n, GS ! n, GS v 0 m xL xH yL yH d...,
//                      ESC 7 n1 n2 n3, ESC 8 n1 n2 (consumed), ESC v n (CSN-A2 status: bit 2 = paper out),
//                      DLE EOT 4 (paper sensor status: 0x12 = paper OK, 0x72 = paper end, Epson semantics).
// Control: paper (0 = paper finished: printable data is discarded and status reports paper end).
#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_MAX 256
#define IMG_MAX (48 * 64)

typedef enum { S_TEXT, S_ESC, S_ESC_ARG, S_GS, S_GS_ARG, S_GSV, S_IMG, S_DLE, S_DLE_EOT, S_ESC_SKIP, S_ESCV } state_t;

typedef struct {
  uart_dev_t uart;
  uint32_t a_paper;
  state_t st;
  char line[LINE_MAX];
  int len;
  uint8_t hdr[6];
  int hdr_len;
  uint8_t img[IMG_MAX];
  int img_need, img_len, img_wbytes, img_h;
  int lines_printed;
  int skip;  // argument bytes still to swallow
} chip_t;

static bool paper(chip_t* c) { return attr_read(c->a_paper) != 0; }

static void flush_line(chip_t* c) {
  c->line[c->len] = 0;
  if (paper(c)) { printf("PRINTER| %s\n", c->line); c->lines_printed++; }
  c->len = 0;
}

static void render_image(chip_t* c) {
  if (!paper(c)) return;
  int w = c->img_wbytes * 8, h = c->img_h;
  printf("PRINTER| [IMG %dx%d]\n", w, h);
  // 4x4 pixel cells -> one character; '#' if 4+ of 16 pixels are black
  char row[LINE_MAX];
  for (int y = 0; y < h; y += 4) {
    int n = 0;
    for (int x = 0; x < w && n < LINE_MAX - 1; x += 4) {
      int black = 0;
      for (int dy = 0; dy < 4 && y + dy < h; dy++)
        for (int dx = 0; dx < 4 && x + dx < w; dx++) {
          int px = x + dx, py = y + dy;
          if (c->img[py * c->img_wbytes + px / 8] & (0x80 >> (px % 8))) black++;
        }
      row[n++] = black >= 4 ? '#' : ' ';
    }
    while (n > 0 && row[n - 1] == ' ') n--;
    row[n] = 0;
    printf("PRINTER|   %s\n", row);
  }
  c->lines_printed++;
}

static void on_rx(void* ud, uint8_t b) {
  chip_t* c = ud;
  switch (c->st) {
    case S_TEXT:
      if (b == 0x1B) c->st = S_ESC;
      else if (b == 0x1D) c->st = S_GS;
      else if (b == 0x10) c->st = S_DLE;
      else if (b == 0x0A) flush_line(c);
      else if (b == 0x0D) {}
      else if (b >= 0x20 && c->len < LINE_MAX - 1) c->line[c->len++] = (char)b;
      break;
    case S_ESC:
      if (b == '@') { c->len = 0; c->st = S_TEXT; }
      else if (b == '!' || b == 'E' || b == 'a' || b == 'd' || b == '-' || b == '3' || b == 'J') c->st = S_ESC_ARG;
      else if (b == '7') { c->skip = 3; c->st = S_ESC_SKIP; }  // heating dots, time, interval
      else if (b == '8') { c->skip = 2; c->st = S_ESC_SKIP; }  // sleep time
      else if (b == 'v') c->st = S_ESCV;
      else c->st = S_TEXT;
      break;
    case S_ESC_ARG: c->st = S_TEXT; break;
    case S_ESC_SKIP: if (--c->skip == 0) c->st = S_TEXT; break;
    case S_ESCV: {  // CSN-A2: one status byte, bit 2 = paper out
      static uint8_t st[1];
      st[0] = paper(c) ? 0x00 : 0x04;
      uart_write(c->uart, st, 1);
      c->st = S_TEXT;
      break;
    }
    case S_GS:
      if (b == '!' || b == 'B') c->st = S_GS_ARG;
      else if (b == 'v') { c->st = S_GSV; c->hdr_len = 0; }
      else c->st = S_TEXT;
      break;
    case S_GS_ARG: c->st = S_TEXT; break;
    case S_GSV:  // '0' m xL xH yL yH
      c->hdr[c->hdr_len++] = b;
      if (c->hdr_len == 6) {
        c->img_wbytes = c->hdr[2] | c->hdr[3] << 8;
        c->img_h = c->hdr[4] | c->hdr[5] << 8;
        c->img_need = c->img_wbytes * c->img_h;
        c->img_len = 0;
        c->st = (c->img_need > 0 && c->img_need <= IMG_MAX) ? S_IMG : S_TEXT;
      }
      break;
    case S_IMG:
      c->img[c->img_len++] = b;
      if (--c->img_need == 0) { render_image(c); c->st = S_TEXT; }
      break;
    case S_DLE: c->st = (b == 0x04) ? S_DLE_EOT : S_TEXT; break;
    case S_DLE_EOT: {
      static uint8_t status[1];
      status[0] = (b == 4) ? (paper(c) ? 0x12 : 0x72) : 0x12;
      uart_write(c->uart, status, 1);
      c->st = S_TEXT;
      break;
    }
  }
}

static void on_write_done(void* ud) { (void)ud; }

void chip_init(void) {
  chip_t* c = calloc(1, sizeof(chip_t));
  c->a_paper = attr_init("paper", 1);
  const uart_config_t cfg = {
      .user_data = c, .rx = pin_init("RX", INPUT), .tx = pin_init("TX", INPUT_PULLUP),
      .baud_rate = 9600, .rx_data = on_rx, .write_done = on_write_done,
  };
  c->uart = uart_init(&cfg);
  printf("printer: ready\n");
}
