#include <driver/adc.h>
#include <naos/utils.h>

#include "smooth.h"

#define DIST_SAMPLE_INTERVAL 16

static smooth_t dist_smooth = smooth_default;

static double dist_last_value = 0;
static uint32_t dist_last_sample = 0;


void dist_init() {
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_11db);
}

double dist_read() {
  if (dist_last_sample + DIST_SAMPLE_INTERVAL < naos_millis()) {
    dist_last_sample = naos_millis();

    dist_last_value = smooth_update(&dist_smooth, adc1_get_voltage(ADC1_CHANNEL_0));
  }

  return dist_last_value;
}
