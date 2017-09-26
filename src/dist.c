#include <driver/gpio.h>
#include <driver/timer.h>
#include <esp_system.h>
#include <naos/utils.h>

#define DIST_TIMER_GROUP TIMER_GROUP_0
#define DIST_TIMER_NUM TIMER_0

static double dist_value = 0;

static void dist_handler(void *_) {
  // check current pin state
  if (gpio_get_level(GPIO_NUM_27) == 1) {
    // reset and start timer
    ESP_ERROR_CHECK(timer_set_counter_value(DIST_TIMER_GROUP, DIST_TIMER_NUM, 0));
    timer_start(DIST_TIMER_GROUP, DIST_TIMER_NUM);
  } else {
    // get timer value and pause timer
    uint64_t value = 0;
    ESP_ERROR_CHECK(timer_get_counter_value(DIST_TIMER_GROUP, DIST_TIMER_NUM, &value));
    ESP_ERROR_CHECK(timer_pause(DIST_TIMER_GROUP, DIST_TIMER_NUM));

    // calculate new distance if value is greater than zero
    if (value > 0) {
      dist_value = value / 2.0 / 29.1;
    }
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

  // prepare timer config
  timer_config_t tim = {
      .alarm_en = false,
      .counter_en = true,
      .intr_type = TIMER_INTR_LEVEL,
      .counter_dir = TIMER_COUNT_UP,
      .divider = 240  // 1 count = 1us
  };

  // initialize timer
  ESP_ERROR_CHECK(timer_init(DIST_TIMER_GROUP, DIST_TIMER_NUM, &tim));

  // stop timer
  ESP_ERROR_CHECK(timer_pause(DIST_TIMER_GROUP, DIST_TIMER_NUM));

  // reset timer
  ESP_ERROR_CHECK(timer_set_counter_value(DIST_TIMER_GROUP, DIST_TIMER_NUM, 0));

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
  // track last poll of sensor
  static uint32_t last_poll = 0;

  // check trigger window
  if (last_poll + 100 < naos_millis()) {
    last_poll = naos_millis();

    // TODO: Use RMT module to generate outward pulse.

    // generate trigger pulse
    gpio_set_level(GPIO_NUM_14, 1);
    ets_delay_us(10);
    gpio_set_level(GPIO_NUM_14, 0);
  }

  // return saved value
  return dist_value;
}
