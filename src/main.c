#include <driver/adc.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <math.h>
#include <naos.h>
#include <stdlib.h>
#include <string.h>

#include "dist.h"
#include "enc.h"
#include "led.h"
#include "mot.h"
#include "pir.h"

bool automate = false;
bool motion = false;
int distance = 0;
double position = 0;
double target = 0;
bool go_up = true;

static void online() {
  // disable motor
  mot_set(0);

  // subscribe local topics
  naos_subscribe("brightness", 0, NAOS_LOCAL);
  naos_subscribe("target", 0, NAOS_LOCAL);
  naos_subscribe("reset", 0, NAOS_LOCAL);
  naos_subscribe("automate", 0, NAOS_LOCAL);
}

static void offline() {
  // disable motor
  mot_set(0);
}

static void message(const char *topic, uint8_t *payload, size_t len, naos_scope_t scope) {
  // set led brightness
  if (strcmp(topic, "brightness") == 0 && scope == NAOS_LOCAL) {
    int brightness = (int)strtol((const char *)payload, NULL, 10);
    led_set(brightness, brightness, brightness, brightness);
  }

  // set automate
  if (strcmp(topic, "automate") == 0) {
    automate = strcmp((const char *)payload, "on") == 0;
  }

  // set target
  if (strcmp(topic, "target") == 0) {
    target = strtod((const char *)payload, NULL);
  }

  // reset position
  if (strcmp(topic, "reset") == 0) {
    position = 0;
    target = 0;
  }
}

static void loop() {
  // read pir sensor
  bool new_motion = pir_get();

  // check if pir changed
  bool motion_changed = motion != new_motion;

  // check pir state
  if (motion_changed) {
    motion = new_motion;

    // publish update
    naos_publish_int("motion", motion ? 1 : 0, 0, false, NAOS_LOCAL);
  }

  // read distance
  int new_distance = (int)round(dist_get());

  // check if distance changed
  bool distance_changed = new_distance > distance + 2 || new_distance < distance - 2;

  // get dist change (+=up, -=down)
  int distance_change = (distance - new_distance) * -1;

  // check dist
  if (distance_changed) {
    distance = new_distance;

    // publish update
    naos_publish_int("distance", distance, 0, false, NAOS_LOCAL);
  }

  // get encoder
  int rotation_change = enc_get();

  // apply rotation
  if (rotation_change != 0) {
    position += (double)rotation_change / 20.0;

    // publish update
    char position_str[10];
    snprintf(position_str, 10, "%.3f", position);
    naos_publish_str("position", position_str, 0, false, NAOS_LOCAL);
  }

  // TODO: Evaluate Automation and adjust target.

  // set motor
  if (position < target + 0.1 && position > target - 0.1) {
    // break if target has been reached
    mot_set(0);
  } else if (position < target) {
    // go down
    mot_set(750);
  } else if (position > target) {
    // go up
    mot_set(-750);
  }

  //  // exit if no automated or distance and motion have not changed
  //  if (!automate || (!distance_changed && !motion_changed)) {
  //    return;
  //  }
  //
  //  // log distance change
  //  naos_log("distance change: %d", distance_change);
  //
  //  // set target distance
  //  int target = 100;
  //  if (motion) {
  //    target = 25;
  //  }
  //
  //  // log target
  //  naos_log("target: %d", target);
  //
  //  // check if target has been reached
  //  if (distance < target + 10 && distance > target - 10) {
  //    naos_log("target reached!");
  //    mot_set(0);
  //    return;
  //  }
  //
  //  // if light goes down but needs to go up
  //  if (distance_change < 0 && target > distance) {
  //    // drive up
  //    naos_log("drive up!");
  //    go_up = !go_up;
  //  }
  //
  //  // if light goes up but needs to go down
  //  if (distance_change > 0 && target < distance) {
  //    // drive down
  //    naos_log("drive down!");
  //    go_up = !go_up;
  //  }
  //
  //  // set motor speed
  //  mot_set(go_up ? 750 : -750);
}

static naos_config_t config = {.device_type = "vas17",
                               .firmware_version = "0.1.0",
                               .loop_callback = loop,
                               .loop_interval = 0,
                               .online_callback = online,
                               .offline_callback = offline,
                               .message_callback = message};

void app_main() {
  // set global adc width
  ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_10Bit));

  // install global interrupt service
  ESP_ERROR_CHECK(gpio_install_isr_service(0));

  // initialize global ledc fade service
  ESP_ERROR_CHECK(ledc_fade_func_install(0));

  // initialize motion sensor
  pir_init();

  // initialize distance sensor
  dist_init();

  // initialize motor
  mot_init();
  mot_set(0);

  // initialize led
  led_init();
  led_set(0, 0, 0, 0);

  // initialize encoder
  enc_init();

  // initialize naos
  naos_init(&config);
}
