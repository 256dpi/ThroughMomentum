#include <driver/adc.h>
#include <driver/gpio.h>
#include <naos.h>

#include "pir.h"

uint64_t pir_last_trigger = 0;

static void pir_handler(void *_) {
  // save trigger time
  pir_last_trigger = naos_millis();
}

void pir_init() {
  // prepare pin config
  gpio_config_t config = {.pin_bit_mask = GPIO_SEL_19,
                          .mode = GPIO_MODE_INPUT,
                          .pull_up_en = GPIO_PULLUP_DISABLE,
                          .pull_down_en = GPIO_PULLDOWN_DISABLE,
                          .intr_type = GPIO_INTR_POSEDGE};

  // configure pin
  ESP_ERROR_CHECK(gpio_config(&config));

  // add interrupt handler
  ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_NUM_19, pir_handler, NULL));

  // prepare analog pin config
  ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_11db));
}

bool pir_get() {
  // return true when pir triggered within the last 8 seconds
  return pir_last_trigger + 8000 > naos_millis();
}

int pir_read() { return abs(590 - adc1_get_raw(ADC1_CHANNEL_6)); }
