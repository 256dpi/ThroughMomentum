#include <driver/gpio.h>
#include <naos.h>

#include "led.h"
#include "rls.h"

static bool r1 = false;
static bool r2 = false;
static bool r3 = false;

static naos_status_t st;

static void update(const char *param, const char *value) {
  rls_set(r1, r2, r3);
}

static void status(naos_status_t status) {
  // save status
  st = status;

  switch (status) {
    case NAOS_DISCONNECTED:
      led_set(false, false);
      break;
    case NAOS_CONNECTED:
      led_set(true, false);
      break;
    case NAOS_NETWORKED:
      led_set(false, true);
      break;
  }
}

static void ping() {
  led_set(true, true);
  naos_delay(300);
  status(st);
}

static naos_param_t params[3] = {
    {.name = "relay-1", .type = NAOS_BOOL, .default_b = false, .sync_b = &r1},
    {.name = "relay-2", .type = NAOS_BOOL, .default_b = false, .sync_b = &r2},
    {.name = "relay-3", .type = NAOS_BOOL, .default_b = false, .sync_b = &r3},
};

static naos_config_t config = {.device_type = "tm-ps",
                               .firmware_version = "0.3.0",
                               .parameters = params,
                               .num_parameters = 3,
                               .ping_callback = ping,
                               .update_callback = update,
                               .status_callback = status,
                               .password = "tm2018"};

void app_main() {
  // init led
  led_init();

  // init relays
  rls_init();

  // initialize naos
  naos_init(&config);

  // set relays
  rls_set(r1, r2, r3);
}
