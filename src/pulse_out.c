#include <driver/rmt.h>

#include "pulse_out.h"

void pulse_out_init(pulse_out_t* po) {
  // prepare config
  rmt_config_t config;
  config.rmt_mode = RMT_MODE_TX;
  config.channel = po->ch;
  config.gpio_num = po->pin;
  config.mem_block_num = 1;
  config.clk_div = 8;
  config.tx_config.loop_en = 0;
  config.tx_config.carrier_en = 0;
  config.tx_config.idle_output_en = 1;
  config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
  config.tx_config.carrier_freq_hz = 100;
  config.tx_config.carrier_level = RMT_CARRIER_LEVEL_LOW;
  config.tx_config.carrier_duty_percent = 50;

  // configure rmt controller
  ESP_ERROR_CHECK(rmt_config(&config));

  // install rmt driver
  ESP_ERROR_CHECK(rmt_driver_install(po->ch, 0, 0));
}

void pulse_out_generate(pulse_out_t* po) {
  // write high
  po->item.level0 = 1;
  po->item.duration0 = 100;  // 10us
  po->item.level1 = 0;
  po->item.duration1 = 100;  // 10us

  // show the pixels
  ESP_ERROR_CHECK(rmt_write_items(po->ch, &po->item, 1, true));  // TODO: Do not wait!
}
