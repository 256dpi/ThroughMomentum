#include <driver/ledc.h>

void rls_set(bool r1, bool r2, bool r3) {
  gpio_set_level(GPIO_NUM_33, r1 ? 1 : 0);
  gpio_set_level(GPIO_NUM_13, r2 ? 1 : 0);
  gpio_set_level(GPIO_NUM_23, r3 ? 1 : 0);
}

void rls_init() {
  // prepare config
  gpio_config_t cfg = {.pin_bit_mask = GPIO_SEL_33 | GPIO_SEL_13 | GPIO_SEL_23, .mode = GPIO_MODE_OUTPUT};

  // configure pin
  ESP_ERROR_CHECK(gpio_config(&cfg));

  // disable
  rls_set(false, false, false);
}
