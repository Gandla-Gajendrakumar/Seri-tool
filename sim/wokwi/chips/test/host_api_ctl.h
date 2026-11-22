#pragma once
#include <stdbool.h>
#include <stdint.h>
extern char host_out[];
extern int host_out_len;
extern uint8_t host_uart_out[];
extern int host_uart_out_len;
extern double host_nanos;
void host_set_attr(const char* name, double v);
void host_set_attr_str(const char* name, const char* s);
void host_advance(double ns);
void host_pump(void);
bool host_i2c_write(const uint8_t* d, int n);
bool host_i2c_read(uint8_t* d, int n);
void host_uart_send(const char* s, int n);
void host_clear(void);
void chip_init(void);
int host_pin(const char* name);
extern float host_dac[8];
extern uint32_t host_pin_level[8];
static int fails;
#define CHECK(cond, msg) do { if (!(cond)) { fails++; fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, msg); } else { passes++; } } while (0)
static int passes;
