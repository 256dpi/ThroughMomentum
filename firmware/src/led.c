#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <naos.h>

#include "led.h"

#define LED_BIT (1 << 0)

static EventGroupHandle_t led_group;

static led_color_t led_fade_out_color;
static int led_fade_total_time;

static void led_task(void *p) {
  // loop forever
  for (;;) {
    // wait for bit
    EventBits_t bits = xEventGroupWaitBits(led_group, LED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
    if ((bits & LED_BIT) != LED_BIT) {
      continue;
    }

    // await half of the fade
    naos_delay(led_fade_total_time / 2);

    // fade out
    led_set(led_fade_out_color, led_fade_total_time / 2);
  }
}

void led_init() {
  // create mutex
  led_group = xEventGroupCreate();

  // install ledc fade service
  ESP_ERROR_CHECK(ledc_fade_func_install(0));

  // prepare timer config
  ledc_timer_config_t ledc_timer = {.duty_resolution = LEDC_TIMER_10_BIT,
                                    .freq_hz = 5000,
                                    .speed_mode = LEDC_HIGH_SPEED_MODE,
                                    .timer_num = LEDC_TIMER_1};

  // configure timer
  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  // prepare channel config
  ledc_channel_config_t ledc_channel = {
      .duty = 0,
      .intr_type = LEDC_INTR_FADE_END,
      .speed_mode = LEDC_HIGH_SPEED_MODE,
      .timer_sel = LEDC_TIMER_1,
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
  led_set((led_color_t){0, 0, 0, 0}, 100);

  // run async task
  xTaskCreatePinnedToCore(&led_task, "led", 2048, NULL, 2, NULL, 1);
}

void led_set(led_color_t fic, int tt) {
  // track led timeout
  static uint32_t led_timeout = 0;

  // return immediately if timeout has not been reached
  if (led_timeout > naos_millis()) {
    return;
  }

  // set red
  ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, (uint32_t)fic.r, tt));
  ESP_ERROR_CHECK(ledc_fade_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, LEDC_FADE_NO_WAIT));

  // set green
  ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, (uint32_t)fic.g, tt));
  ESP_ERROR_CHECK(ledc_fade_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, LEDC_FADE_NO_WAIT));

  // set blue
  ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_3, (uint32_t)fic.b, tt));
  ESP_ERROR_CHECK(ledc_fade_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_3, LEDC_FADE_NO_WAIT));

  // set white
  ESP_ERROR_CHECK(ledc_set_fade_with_time(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_4, (uint32_t)fic.w, tt));
  ESP_ERROR_CHECK(ledc_fade_start(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_4, LEDC_FADE_NO_WAIT));

  // set timeout
  led_timeout = naos_millis() + tt;
}

void led_flash(led_color_t fic, led_color_t foc, int tt) {
  // set variables
  led_fade_out_color = foc;
  led_fade_total_time = tt;

  // fade in
  led_set(fic, tt / 2);

  // unlock led task
  xEventGroupSetBits(led_group, LED_BIT);
}
