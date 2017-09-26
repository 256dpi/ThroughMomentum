#include <driver/rmt.h>
#include <freertos/task.h>
#include <naos.h>

#include "pulse_in.h"

static void pulse_in_task(void * _pi) {
  // get config
  pulse_in_t * pi = _pi;

  // get buffer
  RingbufHandle_t rb = NULL;

  // get RMT RX ringbuffer
  ESP_ERROR_CHECK(rmt_get_ringbuf_handler(pi->ch, &rb));

  // start rmt receiver
  ESP_ERROR_CHECK(rmt_rx_start(pi->ch, 1));

  // receive items
  for(;;) {
    // item size
    size_t rx_size = 0;

    // receive next item from buffer
    rmt_item32_t* item = (rmt_item32_t*) xRingbufferReceive(rb, &rx_size, portMAX_DELAY);
    if(item == NULL) {
      continue;
    }

    // log data
    naos_log("rmt data (%d): %d:%d - %d:%d", rx_size, item->level0, item->duration0, item->level1, item->duration1);

    // return item to buffer
    vRingbufferReturnItem(rb, item);
  }
}

void pulse_in_init(pulse_in_t * pi) {
  // prepare config
  rmt_config_t config;
  config.rmt_mode = RMT_MODE_RX;
  config.channel = pi->ch;
  config.gpio_num = pi->pin;
  config.mem_block_num = 1;
  config.clk_div = 8;
  config.rx_config.filter_en = false;
  config.rx_config.filter_ticks_thresh = 100; // 1us
  config.rx_config.idle_threshold = 0xffff;

  // configure rmt controller
  ESP_ERROR_CHECK(rmt_config(&config));

  // install rmt driver
  ESP_ERROR_CHECK(rmt_driver_install(pi->ch, sizeof(rmt_item32_t) * 10000, 0));

  // start task
  xTaskCreatePinnedToCore(pulse_in_task, "pulse_in_task", 2048, pi, 2, NULL, 1);
}
