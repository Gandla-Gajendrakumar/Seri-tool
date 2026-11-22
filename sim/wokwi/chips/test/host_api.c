#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef printf
#define printf host_printf_unused
#include "../wokwi-api.h"
#undef printf
#include "host_api_ctl.h"

// ---- captured chip output (printf) ----
char host_out[65536];
int host_out_len;
int host_printf(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(host_out + host_out_len, sizeof host_out - (size_t)host_out_len, fmt, ap);
  va_end(ap);
  if (n > 0) host_out_len += n;
  if (host_out_len > (int)sizeof host_out - 1) host_out_len = (int)sizeof host_out - 1;
  return n;
}

// ---- attributes ----
static struct { char name[32]; double value; char str[2048]; } attrs[32];
static int nattrs;
static int find_attr(const char* name) {
  for (int i = 0; i < nattrs; i++) if (strcmp(attrs[i].name, name) == 0) return i;
  return -1;
}
static uint32_t attr_add(const char* name, double def) {
  int i = find_attr(name);
  if (i >= 0) return (uint32_t)i + 1;  // test pre-set it
  snprintf(attrs[nattrs].name, sizeof attrs[0].name, "%s", name);
  attrs[nattrs].value = def;
  return (uint32_t)++nattrs;
}
void host_set_attr(const char* name, double v) {
  int i = find_attr(name);
  if (i < 0) { i = nattrs++; snprintf(attrs[i].name, sizeof attrs[0].name, "%s", name); }
  attrs[i].value = v;
}
void host_set_attr_str(const char* name, const char* s) {
  host_set_attr(name, 0);
  snprintf(attrs[find_attr(name)].str, sizeof attrs[0].str, "%s", s);
}
uint32_t attr_init(const char* name, uint32_t d) { return attr_add(name, (double)d); }
uint32_t attr_init_float(const char* name, float d) { return attr_add(name, d); }
uint32_t attr_read(uint32_t id) { return (uint32_t)attrs[id - 1].value; }
float attr_read_float(uint32_t id) { return (float)attrs[id - 1].value; }
string_t attr_string_init(const char* name) { int i = find_attr(name); return i < 0 ? STRING_NULL : (string_t)i + 1; }
uint32_t string_get_length(string_t s) { return (uint32_t)strlen(attrs[s - 1].str); }
uint32_t string_read(string_t s, char* buf, uint32_t n) { snprintf(buf, n, "%s", attrs[s - 1].str); return (uint32_t)strlen(buf); }

// ---- pins (unused by tests) ----
pin_t pin_init(const char* name, uint32_t mode) { (void)name; (void)mode; return 1; }
uint32_t pin_read(pin_t p) { (void)p; return 0; }
void pin_write(pin_t p, uint32_t v) { (void)p; (void)v; }

// ---- time ----
double host_nanos;
double get_sim_nanos_d(void) { return host_nanos; }
static timer_config_t timer_cfg;
static double timer_period_ns;
static double timer_next;
timer_t timer_init(const timer_config_t* c) { timer_cfg = *c; return 1; }
void timer_start(const timer_t t, uint32_t micros, bool repeat) { (void)t; (void)repeat; timer_period_ns = micros * 1000.0; timer_next = host_nanos + timer_period_ns; }
void timer_start_ns_d(const timer_t t, double ns, bool repeat) { (void)t; (void)repeat; timer_period_ns = ns; timer_next = host_nanos + ns; }
void timer_stop(const timer_t t) { (void)t; timer_period_ns = 0; }
void host_advance(double ns) {
  double end = host_nanos + ns;
  while (timer_period_ns > 0 && timer_next <= end) {
    host_nanos = timer_next;
    timer_next += timer_period_ns;
    timer_cfg.callback(timer_cfg.user_data);
    host_pump();
  }
  host_nanos = end;
}

// ---- I2C ----
static i2c_config_t i2c_cfg;
i2c_dev_t i2c_init(const i2c_config_t* c) { i2c_cfg = *c; return 1; }
bool host_i2c_write(const uint8_t* d, int n) {
  if (!i2c_cfg.connect(i2c_cfg.user_data, i2c_cfg.address, false)) return false;
  for (int i = 0; i < n; i++) i2c_cfg.write(i2c_cfg.user_data, d[i]);
  i2c_cfg.disconnect(i2c_cfg.user_data);
  return true;
}
bool host_i2c_read(uint8_t* d, int n) {
  if (!i2c_cfg.connect(i2c_cfg.user_data, i2c_cfg.address, true)) return false;
  for (int i = 0; i < n; i++) d[i] = i2c_cfg.read(i2c_cfg.user_data);
  i2c_cfg.disconnect(i2c_cfg.user_data);
  return true;
}

// ---- UART ----
static uart_config_t uart_cfg;
uint8_t host_uart_out[65536];
int host_uart_out_len;
static int pending_done;
uart_dev_t uart_init(const uart_config_t* c) { uart_cfg = *c; return 1; }
bool uart_write(uart_dev_t u, uint8_t* b, uint32_t n) {
  (void)u;
  for (uint32_t i = 0; i < n && host_uart_out_len < (int)sizeof host_uart_out; i++) host_uart_out[host_uart_out_len++] = b[i];
  pending_done++;
  return true;
}
void host_pump(void) {  // deliver write_done callbacks asynchronously, like the simulator
  while (pending_done > 0) { pending_done--; if (uart_cfg.write_done) uart_cfg.write_done(uart_cfg.user_data); }
}
void host_uart_send(const char* s, int n) {
  for (int i = 0; i < n; i++) { uart_cfg.rx_data(uart_cfg.user_data, (uint8_t)s[i]); host_pump(); }
}
void host_clear(void) { host_out_len = 0; host_out[0] = 0; host_uart_out_len = 0; }
