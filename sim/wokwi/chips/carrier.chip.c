// Carrier-board sense lines that scenarios need to control (no real part: these are the
// outputs of the power-path / charger board and the microSD socket's card-detect switch).
//   MAINS_OK   digital, HIGH while the 12 V adapter is present (opto/divider output on the PCB)
//   BATT_SENSE analog, battery voltage through the 1:2 divider (100k/100k) to an ADC pin
//   SD_CD      card-detect switch: closed to GND when a card is inserted, pulled up when removed
// Controls: mains (0/1), batteryMv (2800-4300), sdPresent (0/1).
#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  pin_t mains, batt, cd;
  uint32_t a_mains, a_mv, a_sd;
  timer_t timer;
} chip_t;

static void update(void* ud) {
  chip_t* c = ud;
  pin_write(c->mains, attr_read(c->a_mains) ? HIGH : LOW);
  pin_dac_write(c->batt, attr_read_float(c->a_mv) / 2.0f / 1000.0f);
  pin_write(c->cd, attr_read(c->a_sd) ? LOW : HIGH);
}

void chip_init(void) {
  chip_t* c = calloc(1, sizeof(chip_t));
  c->mains = pin_init("MAINS_OK", OUTPUT);
  c->batt = pin_init("BATT_SENSE", ANALOG);
  c->cd = pin_init("SD_CD", OUTPUT);
  c->a_mains = attr_init("mains", 1);
  c->a_mv = attr_init_float("batteryMv", 4100);
  c->a_sd = attr_init("sdPresent", 1);
  const timer_config_t t = {.user_data = c, .callback = update};
  c->timer = timer_init(&t);
  update(c);
  timer_start(c->timer, 10000, true);  // 10 ms
  printf("carrier: ready\n");
}
