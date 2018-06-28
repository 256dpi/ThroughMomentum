#include <naos.h>
#include <driver/gpio.h>

static bool on = false;

static void update(const char *param, const char *value) {
  // set pin
  gpio_set_level(GPIO_NUM_22, on ? 1 : 0);
}

static naos_param_t params[1] = {
    {.name = "on", .type = NAOS_BOOL, .default_b = false, .sync_b = &on},
};

static naos_config_t config = {.device_type = "supply",
                               .firmware_version = "0.1.0",
                               .parameters = params,
                               .num_parameters = 1,
                               .update_callback = update};

void app_main() {
  // prepare config
  gpio_config_t cfg = {
      .pin_bit_mask = GPIO_SEL_22,
      .mode = GPIO_MODE_OUTPUT
  };

  // configure pin
  ESP_ERROR_CHECK(gpio_config(&cfg));

  // initialize naos
  naos_init(&config);
}
