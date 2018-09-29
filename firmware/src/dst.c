#include <art32/smooth.h>
#include <driver/gpio.h>
#include <driver/rmt.h>
#include <driver/timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <naos.h>

#include "dst.h"

#define DST_RANGE_MIN 5
#define DST_RANGE_MAX 300
#define DST_INTERVAL 100
#define DST_TIMEOUT 2000

#define DST_TIMER_GROUP TIMER_GROUP_0
#define DST_TIMER_NUM TIMER_0
#define DST_TRIGGER_RMT_CHANNEL RMT_CHANNEL_0

static dst_callback_t dst_callback;

static QueueHandle_t dst_queue;

static a32_smooth_t *dst_smooth;

static void dst_handler(void *_) {
  // track if we are currently reading
  static bool reading = false;

  // start reading if pin is high
  if (gpio_get_level(GPIO_NUM_22) == 1) {
    // reset and start timer
    ESP_ERROR_CHECK(timer_set_counter_value(DST_TIMER_GROUP, DST_TIMER_NUM, 0));
    timer_start(DST_TIMER_GROUP, DST_TIMER_NUM);
    reading = true;
    return;
  }

  // measure pulse if reading
  if (reading) {
    // get timer value and pause timer
    uint64_t value = 0;
    ESP_ERROR_CHECK(timer_get_counter_value(DST_TIMER_GROUP, DST_TIMER_NUM, &value));
    ESP_ERROR_CHECK(timer_pause(DST_TIMER_GROUP, DST_TIMER_NUM));

    // calculate real distance
    double distance = (double)value / 58.7;  // 29.3866996 us/cm

    // send distance if value is in acceptable range
    if (distance >= DST_RANGE_MIN && distance <= DST_RANGE_MAX) {
      xQueueSendFromISR(dst_queue, &distance, NULL);
    }
  }
}

static void dst_task(void *p) {
  // loop forever
  for (;;) {
    // create rmt waveform item
    static rmt_item32_t item = {
        .level0 = 1,
        .duration0 = 10,  // 10us
        .level1 = 0,
        .duration1 = 10  // 10us
    };

    // generate trigger signal
    ESP_ERROR_CHECK(rmt_write_items(DST_TRIGGER_RMT_CHANNEL, &item, 1, false));

    // wait for distance reading
    double distance = 0;
    if (xQueueReceive(dst_queue, &distance, DST_TIMEOUT / portTICK_PERIOD_MS) == pdFALSE) {
      // try again if no reading was received after 2s
      continue;
    }

    // smooth distance
    distance = a32_smooth_update(dst_smooth, distance);

    // call callback
    naos_acquire();
    dst_callback(distance);
    naos_release();

    // wait for next reading
    naos_delay(DST_INTERVAL);
  }
}

void dst_init(dst_callback_t cb) {
  // save callback
  dst_callback = cb;

  // initialize queue
  dst_queue = xQueueCreate(16, sizeof(double));

  // create smooth
  dst_smooth = a32_smooth_new(10);

  // prepare trigger rmt channel
  rmt_config_t trig;
  trig.rmt_mode = RMT_MODE_TX;
  trig.channel = DST_TRIGGER_RMT_CHANNEL;
  trig.gpio_num = GPIO_NUM_21;
  trig.mem_block_num = 1;
  trig.clk_div = 80;  // 80Mhz: 1 count = 1us
  trig.tx_config.loop_en = 0;
  trig.tx_config.carrier_en = 0;
  trig.tx_config.idle_output_en = 1;
  trig.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
  trig.tx_config.carrier_freq_hz = 100;
  trig.tx_config.carrier_level = RMT_CARRIER_LEVEL_LOW;
  trig.tx_config.carrier_duty_percent = 50;

  // configure trigger rmt channel
  ESP_ERROR_CHECK(rmt_config(&trig));

  // install trigger rmt driver
  ESP_ERROR_CHECK(rmt_driver_install(DST_TRIGGER_RMT_CHANNEL, 0, 0));

  // prepare timer config
  timer_config_t tim = {
      .alarm_en = false,
      .counter_en = true,
      .intr_type = TIMER_INTR_LEVEL,
      .counter_dir = TIMER_COUNT_UP,
      .divider = 80  // 80Mhz: 1 count = 1us
  };

  // initialize timer
  ESP_ERROR_CHECK(timer_init(DST_TIMER_GROUP, DST_TIMER_NUM, &tim));

  // stop timer
  ESP_ERROR_CHECK(timer_pause(DST_TIMER_GROUP, DST_TIMER_NUM));

  // reset timer
  ESP_ERROR_CHECK(timer_set_counter_value(DST_TIMER_GROUP, DST_TIMER_NUM, 0));

  // prepare echo config
  gpio_config_t echo = {.pin_bit_mask = GPIO_SEL_22,
                        .mode = GPIO_MODE_INPUT,
                        .pull_up_en = GPIO_PULLUP_DISABLE,
                        .pull_down_en = GPIO_PULLDOWN_ENABLE,
                        .intr_type = GPIO_INTR_ANYEDGE};

  // configure echo
  ESP_ERROR_CHECK(gpio_config(&echo));

  // attach handler
  ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_NUM_22, dst_handler, NULL));

  // run async task
  xTaskCreatePinnedToCore(&dst_task, "dst", 2048, NULL, 2, NULL, 1);
}
