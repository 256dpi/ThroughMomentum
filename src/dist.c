#include <driver/gpio.h>
#include <esp_system.h>
#include <limits.h>
#include <naos/utils.h>
#include <sdkconfig.h>

uint32_t dist_last_poll = 0;
uint32_t dist_echo_start = 0;
double dist_value = 0;

#define clockCyclesPerMicrosecond() (CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ)
#define clockCyclesToMicroseconds(a) ((a) / clockCyclesPerMicrosecond())

static void dist_handler(void *_) {
  // handle start and stop of pulse and calculate distance
  if (gpio_get_level(GPIO_NUM_27) == 1) {
    dist_echo_start = xthal_get_ccount();
  } else if (dist_echo_start > 0) {
    uint32_t echo_now = xthal_get_ccount();
    if (echo_now < dist_echo_start) {
      echo_now += UINT_MAX;
    }

    uint32_t echo_time = clockCyclesToMicroseconds(echo_now - dist_echo_start);

    dist_value = (double)echo_time / 2.0 / 29.1;
    dist_echo_start = 0;
  }
}

void dist_init() {
  // prepare trigger config
  gpio_config_t trig = {.pin_bit_mask = GPIO_SEL_14,
                        .mode = GPIO_MODE_OUTPUT,
                        .pull_up_en = GPIO_PULLUP_DISABLE,
                        .pull_down_en = GPIO_PULLDOWN_ENABLE,
                        .intr_type = GPIO_INTR_DISABLE};

  // configure trigger
  ESP_ERROR_CHECK(gpio_config(&trig));

  // prepare echo config
  gpio_config_t echo = {.pin_bit_mask = GPIO_SEL_27,
                        .mode = GPIO_MODE_INPUT,
                        .pull_up_en = GPIO_PULLUP_DISABLE,
                        .pull_down_en = GPIO_PULLDOWN_ENABLE,
                        .intr_type = GPIO_INTR_ANYEDGE};

  // configure echo
  ESP_ERROR_CHECK(gpio_config(&echo));

  // attach handler
  ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_NUM_27, dist_handler, NULL));
}

double dist_get() {
  // check trigger window
  if (dist_last_poll + 60 < naos_millis()) {
    dist_last_poll = naos_millis();

    // generate trigger pulse
    gpio_set_level(GPIO_NUM_14, 1);
    ets_delay_us(10);
    gpio_set_level(GPIO_NUM_14, 0);
  }

  // return saved value
  return dist_value;
}
