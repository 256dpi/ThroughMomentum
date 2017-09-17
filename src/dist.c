#include <driver/gpio.h>
#include <naos/utils.h>
#include <sys/time.h>
#include <rom/ets_sys.h>

uint32_t dist_last_poll = 0;
uint32_t dist_echo_start = 0;
double dist_value = 0;

uint64_t micros() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return 1000000 * tv.tv_sec + tv.tv_usec;
}

//void delay(uint32_t ms) {
//  vTaskDelay(ms / portTICK_PERIOD_MS);
//}

void naos_sleep(uint32_t us) {
  ets_delay_us(us);
}

void dist_handler(void *_) {
  // handle start and stop of pulse and calculate distance
  if(gpio_get_level(GPIO_NUM_27) == 1) {
    dist_echo_start = micros();
  } else if(dist_echo_start > 0) {
    dist_value = (micros() - dist_echo_start) / 2 / 29.1;
    dist_echo_start = 0;
  }
}

void dist_init() {
  // prepare trigger config
  gpio_config_t trig = {
      .pin_bit_mask  = GPIO_SEL_14,
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_ENABLE,
      .intr_type = GPIO_INTR_DISABLE
  };

  // configure trigger
  ESP_ERROR_CHECK(gpio_config(&trig));

  // prepare echo config
  gpio_config_t echo = {
      .pin_bit_mask  = GPIO_SEL_27,
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_ENABLE,
      .intr_type = GPIO_INTR_ANYEDGE
  };

  // configure echo
  ESP_ERROR_CHECK(gpio_config(&echo));

  // attach handler
  ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_NUM_27, dist_handler, NULL));
}

double dist_get() {
  // generate trigger pulse if poll window has arrived
  if (dist_last_poll + 50 < naos_millis()) {
    dist_last_poll = naos_millis();

    gpio_set_level(GPIO_NUM_14, 1);
    naos_sleep(10);
    gpio_set_level(GPIO_NUM_14, 0);
  }

  return dist_value;
}
