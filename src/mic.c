#include <driver/adc.h>
#include <math.h>
#include <naos/utils.h>

#include "smooth.h"

#define MIC_SAMPLE_INTERVAL 16
#define MIC_NUM_SAMPLES 20

static smooth_t mic_smooth = smooth_default;
static double mic_samples[MIC_NUM_SAMPLES] = {0};
static int mic_sample_index = 0;
static double mic_last_value = 0;
static uint32_t mic_last_sample = 0;

void mic_init() {
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_11db);
}

double mic_read() {
  if (mic_last_sample + MIC_SAMPLE_INTERVAL < naos_millis()) {
    mic_last_sample = naos_millis();

    mic_last_value =  adc1_get_voltage(ADC1_CHANNEL_6);
//    mic_last_value = fabs(smooth_update(&mic_smooth, adc1_get_voltage(ADC1_CHANNEL_6) - 512.0));
  }

  return mic_last_value;
}
