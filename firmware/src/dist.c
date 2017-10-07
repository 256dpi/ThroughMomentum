#include <art32/smooth.h>
#include <driver/gpio.h>
#include <driver/rmt.h>
#include <driver/timer.h>
#include <naos.h>

#define DIST_TIMER_GROUP TIMER_GROUP_0
#define DIST_TIMER_NUM TIMER_0
#define DIST_TRIGGER_RMT_CHANNEL RMT_CHANNEL_0

static a32_smooth_t *dist_smooth;

static double dist_value = 0;

static void dist_handler(void *_) {
  // track if we are currently reading
  static bool reading = false;

  // check current pin state
  if (gpio_get_level(GPIO_NUM_22) == 1) {
    // reset and start timer
    ESP_ERROR_CHECK(timer_set_counter_value(DIST_TIMER_GROUP, DIST_TIMER_NUM, 0));
    timer_start(DIST_TIMER_GROUP, DIST_TIMER_NUM);
    reading = true;
  } else if (reading) {
    // get timer value and pause timer
    uint64_t value = 0;
    ESP_ERROR_CHECK(timer_get_counter_value(DIST_TIMER_GROUP, DIST_TIMER_NUM, &value));
    ESP_ERROR_CHECK(timer_pause(DIST_TIMER_GROUP, DIST_TIMER_NUM));

    // calculate real distance
    double real_distance = (double)value / 58.7;  // 29.3866996 us/cm

    // calculate new distance if value is greater than zero
    if (real_distance > 0 && real_distance <= 400) {
      dist_value = a32_smooth_update(dist_smooth, real_distance);
    }
  }
}

void dist_init() {
  // create smooth
  dist_smooth = a32_smooth_new(5);

  // prepare trigger rmt channel
  rmt_config_t trig;
  trig.rmt_mode = RMT_MODE_TX;
  trig.channel = DIST_TRIGGER_RMT_CHANNEL;
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
  ESP_ERROR_CHECK(rmt_driver_install(DIST_TRIGGER_RMT_CHANNEL, 0, 0));

  // prepare timer config
  timer_config_t tim = {
      .alarm_en = false,
      .counter_en = true,
      .intr_type = TIMER_INTR_LEVEL,
      .counter_dir = TIMER_COUNT_UP,
      .divider = 80  // 80Mhz: 1 count = 1us
  };

  // initialize timer
  ESP_ERROR_CHECK(timer_init(DIST_TIMER_GROUP, DIST_TIMER_NUM, &tim));

  // stop timer
  ESP_ERROR_CHECK(timer_pause(DIST_TIMER_GROUP, DIST_TIMER_NUM));

  // reset timer
  ESP_ERROR_CHECK(timer_set_counter_value(DIST_TIMER_GROUP, DIST_TIMER_NUM, 0));

  // prepare echo config
  gpio_config_t echo = {.pin_bit_mask = GPIO_SEL_22,
                        .mode = GPIO_MODE_INPUT,
                        .pull_up_en = GPIO_PULLUP_DISABLE,
                        .pull_down_en = GPIO_PULLDOWN_ENABLE,
                        .intr_type = GPIO_INTR_ANYEDGE};

  // configure echo
  ESP_ERROR_CHECK(gpio_config(&echo));

  // attach handler
  ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_NUM_22, dist_handler, NULL));
}

static void dist_trigger() {
  // create rmt waveform item
  static rmt_item32_t item = {
      .level0 = 1,
      .duration0 = 10,  // 10us
      .level1 = 0,
      .duration1 = 10  // 10us
  };

  // generate trigger signal
  ESP_ERROR_CHECK(rmt_write_items(DIST_TRIGGER_RMT_CHANNEL, &item, 1, false));
}

double dist_get() {
  // track last poll of sensor
  static uint32_t last_poll = 0;

  // generate trigger every 60ms
  if (last_poll + 100 < naos_millis()) {
    last_poll = naos_millis();
    dist_trigger();
  }

  // return saved value
  return dist_value;
}
