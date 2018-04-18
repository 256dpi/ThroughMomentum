#include <driver/adc.h>
#include <stdlib.h>

#include "pir.h"

void pir_init() {
  // set adc width
  ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_10Bit));

  // prepare analog pin config
  ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_11db));
}

int pir_read() { return abs(590 - adc1_get_voltage(ADC1_CHANNEL_6)); }
