#include <stdio.h>
#include <string.h>
#include "host_api_ctl.h"

static int logged(const char* n) { host_out[host_out_len] = 0; return strstr(host_out, n) != NULL; }
static void send(const char* s, int n) { host_uart_send(s, n); }

int main(void) {
  host_set_attr("paper", 1);
  chip_init();
  send("\x1b@", 2);
  send("\x1b!\x08" "21/11 14:05 TEMP HIGH 31.2C\n", 31);
  CHECK(logged("PRINTER| 21/11 14:05 TEMP HIGH 31.2C"), "text line printed, ESC ! skipped");
  host_clear();
  send("\x10\x04\x04", 3);
  CHECK(host_uart_out_len == 1 && host_uart_out[0] == 0x12, "status: paper OK = 0x12");
  // raster: 16x8 image (2 bytes wide), left half black
  char img[8 + 16];
  memcpy(img, "\x1dv0\x00\x02\x00\x08\x00", 8);
  for (int i = 0; i < 16; i++) img[8 + i] = (i % 2 == 0) ? (char)0xFF : 0x00;
  send(img, 24);
  CHECK(logged("PRINTER| [IMG 16x8]"), "raster image rendered");
  CHECK(logged("PRINTER|   ##"), "text-art shows the black half");
  host_set_attr("paper", 0);
  host_clear();
  send("\x10\x04\x04", 3);
  CHECK(host_uart_out_len == 1 && host_uart_out[0] == 0x72, "status: paper end = 0x72");
  send("lost line\n", 10);
  CHECK(!logged("lost line"), "nothing printed without paper");
  fprintf(stderr, "printer: %d passed, %d failed\n", passes, fails);
  return fails != 0;
}
