#include <driver/ledc.h>

// TODO: Use fade module to drive ramps?

void mot_init() {
  // prepare in a+b config
  gpio_config_t inAB = {.pin_bit_mask = GPIO_SEL_32 | GPIO_SEL_33,
                        .mode = GPIO_MODE_OUTPUT,
                        .pull_up_en = GPIO_PULLUP_DISABLE,
                        .pull_down_en = GPIO_PULLDOWN_ENABLE,
                        .intr_type = GPIO_INTR_DISABLE};

  // configure in a+b pins
  ESP_ERROR_CHECK(gpio_config(&inAB));

  // prepare ledc timer config
  ledc_timer_config_t t = {
      .bit_num = LEDC_TIMER_10_BIT, .freq_hz = 10000, .speed_mode = LEDC_HIGH_SPEED_MODE, .timer_num = LEDC_TIMER_0};

  // configure ledc timer
  ESP_ERROR_CHECK(ledc_timer_config(&t));

  // prepare ledc channel config
  ledc_channel_config_t c = {.duty = 0,
                             .intr_type = LEDC_INTR_DISABLE,
                             .speed_mode = LEDC_HIGH_SPEED_MODE,
                             .timer_sel = LEDC_TIMER_0,
                             .gpio_num = GPIO_NUM_25,
                             .channel = LEDC_CHANNEL_0};

  // configure ledc channel
  ESP_ERROR_CHECK(ledc_channel_config(&c));
}

void mot_set(int speed) {
  if (speed == 0) {
    // disable motor (brake to GND)
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_32, 0));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_33, 0));
  } else if (speed < 0) {
    // go backwards
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_32, 0));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_33, 1));
  } else if (speed > 0) {
    // go forwards
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_32, 1));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_33, 0));
  }

  // handle minus speeds
  if (speed < 0) {
    speed = speed * -1;
  }

  // set duty
  ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, (uint32_t)speed));

  // update channel
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0));
}