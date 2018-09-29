#include <art32/motion.h>
#include <art32/numbers.h>
#include <driver/ledc.h>
#include <math.h>

#include "mot.h"

static a32_motion_t mot_mp;

static void mot_set(int speed) {
  // cap speed
  speed = a32_constrain_i(speed, -1023, 1023);

  // set motor state
  if (speed == 0) {
    // disable motor (brake to GND)
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_14, 0));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_16, 0));
  } else if (speed < 0) {
    // go backwards
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_14, 0));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_16, 1));
  } else if (speed > 0) {
    // go forwards
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_14, 1));
    ESP_ERROR_CHECK(gpio_set_level(GPIO_NUM_16, 0));
  }

  // handle minus speeds
  if (speed < 0) {
    speed = speed * -1;
  }

  // set duty
  ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, (uint32_t)speed));
  ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0));
}

static void mot_move_up(double speed) {
  // cap speed
  speed = a32_constrain_d(speed, 0, 12);

  // calculate and set raw speed
  int raw = (int)floor(69.88908 * speed + 142.488);
  mot_set(raw);
}

static void mot_move_down(double speed) {
  // cap speed
  speed = a32_constrain_d(speed, 0, 12);

  // calculate and set raw speed
  int raw = (int)floor(59.54553 * speed + 65.3359);
  mot_set(-raw);
}

void mot_init() {
  // prepare in a+b config
  gpio_config_t in_ab = {.pin_bit_mask = GPIO_SEL_14 | GPIO_SEL_16,
                         .mode = GPIO_MODE_OUTPUT,
                         .pull_up_en = GPIO_PULLUP_DISABLE,
                         .pull_down_en = GPIO_PULLDOWN_ENABLE,
                         .intr_type = GPIO_INTR_DISABLE};

  // configure in a+b pins
  ESP_ERROR_CHECK(gpio_config(&in_ab));

  // prepare ledc timer config
  ledc_timer_config_t t = {.duty_resolution = LEDC_TIMER_10_BIT,
                           .freq_hz = 10000,
                           .speed_mode = LEDC_HIGH_SPEED_MODE,
                           .timer_num = LEDC_TIMER_0};

  // configure ledc timer
  ESP_ERROR_CHECK(ledc_timer_config(&t));

  // prepare ledc channel config
  ledc_channel_config_t c = {.duty = 0,
                             .intr_type = LEDC_INTR_DISABLE,
                             .speed_mode = LEDC_HIGH_SPEED_MODE,
                             .timer_sel = LEDC_TIMER_0,
                             .gpio_num = GPIO_NUM_17,
                             .channel = LEDC_CHANNEL_0};

  // configure ledc channel
  ESP_ERROR_CHECK(ledc_channel_config(&c));

  // stop motor
  mot_stop();
}

bool mot_approach(double position, double target, uint32_t time) {
  // configure motion profile
  mot_mp.max_velocity = 12.0 /* cm */ / 1000 /* s */ * 1.25;
  mot_mp.max_acceleration = 0.01 /* cm */ / 1000 /* s */;

  // provide measured position
  mot_mp.position = position;

  // update motion profile (for next ms)
  a32_motion_update(&mot_mp, target, time);

  // check if target has been reached (within 0.2cm and velocity < 2cm/s)
  if (position < target + 0.2 && position > target - 0.2 && mot_mp.velocity < 0.002) {
    // stop motor
    mot_stop();

    return true;
  }

  // move depending on position
  if (mot_mp.velocity > 0) {
    mot_move_up(mot_mp.velocity * 1000 * 0.8);
  } else {
    mot_move_down(fabs(mot_mp.velocity) * 1000 * 0.8);
  }

  return false;
}

void mot_stop() {
  // set zero speed to stop motor
  mot_set(0);

  // reset motion profile
  mot_mp = (a32_motion_t){0};
}
