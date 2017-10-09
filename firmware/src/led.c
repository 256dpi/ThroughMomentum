#include <driver/ledc.h>
#include <naos.h>

#include "led.h"

static uint32_t led_timeout = 0;

void led_init() {
  // prepare timer config
  ledc_timer_config_t ledc_timer = {
      .bit_num = LEDC_TIMER_10_BIT, .freq_hz = 5000, .speed_mode = LEDC_HIGH_SPEED_MODE, .timer_num = LEDC_TIMER_1};

  // configure timer
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  // prepare channel config
  ledc_channel_config_t ledc_channel = {
      .duty = 0, .intr_type = LEDC_INTR_FADE_END, .speed_mode = LEDC_HIGH_SPEED_MODE, .timer_sel = LEDC_TIMER_1,
  };

  // configure red channel
  ledc_channel.channel = LEDC_CHANNEL_1;
  ledc_channel.gpio_num = GPIO_NUM_27;
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  // configure green channel
  ledc_channel.channel = LEDC_CHANNEL_2;
  ledc_channel.gpio_num = GPIO_NUM_26;
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  // configure blue channel
  ledc_channel.channel = LEDC_CHANNEL_3;
  ledc_channel.gpio_num = GPIO_NUM_32;
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  // configure white channel
  ledc_channel.channel = LEDC_CHANNEL_4;
  ledc_channel.gpio_num = GPIO_NUM_33;
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  // reset led
  led_set(0, 0, 0, 0);
}

void led_set(int r, int g, int b, int w) {
  // return immediately if timeout has not been reached
  if (led_timeout > naos_millis()) {
    return;
  }

  // set red
  ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, (uint32_t)r, 100));
  ESP_ERROR_CHECK(ledc_fade_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, LEDC_FADE_NO_WAIT));

  // set green
  ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, (uint32_t)g, 100));
  ESP_ERROR_CHECK(ledc_fade_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, LEDC_FADE_NO_WAIT));

  // set blue
  ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_3, (uint32_t)b, 100));
  ESP_ERROR_CHECK(ledc_fade_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_3, LEDC_FADE_NO_WAIT));

  // set white
  ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_4, (uint32_t)w, 100));
  ESP_ERROR_CHECK(ledc_fade_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_4, LEDC_FADE_NO_WAIT));

  // set timeout
  led_timeout = naos_millis() + 100;
}
