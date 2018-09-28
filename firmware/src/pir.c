#include <driver/adc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <naos.h>
#include <stdlib.h>

#include "pir.h"

static pir_callback_t pir_callback;

static void pir_task(void *p) {
  // loop forever
  for (;;) {
    // read pir
    int v = abs(590 - adc1_get_raw(ADC1_CHANNEL_6));

    // call callback
    naos_acquire();
    pir_callback(v);
    naos_release();

    // wait 100ms
    naos_delay(100);
  }
}

void pir_init(pir_callback_t cb) {
  // save callback
  pir_callback = cb;

  // set adc width
  ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_10Bit));

  // prepare analog pin config
  ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_11db));

  // run async task
  xTaskCreatePinnedToCore(&pir_task, "pir", 2048, NULL, 2, NULL, 1);
}
