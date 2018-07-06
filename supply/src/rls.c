#include <driver/ledc.h>

void rls_set(bool on) { gpio_set_level(GPIO_NUM_33, on ? 1 : 0); }

void rls_init() {
  // prepare config
  gpio_config_t cfg = {.pin_bit_mask = GPIO_SEL_33, .mode = GPIO_MODE_OUTPUT};

  // configure pin
  ESP_ERROR_CHECK(gpio_config(&cfg));

  // disable
  rls_set(false);
}
