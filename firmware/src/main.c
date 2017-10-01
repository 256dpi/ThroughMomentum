#include <driver/adc.h>
#include <driver/gpio.h>
#include <math.h>
#include <naos.h>
#include <stdlib.h>
#include <string.h>

#include "dist.h"
#include "enc.h"
#include "led.h"
#include "mot.h"
#include "pir.h"

int speed = 0;
bool automate = false;
bool motion = false;
int distance = 0;
double position = 0;
double target = 0;

static void ping() {
  // flash white for 200ms
  led_set(0, 0, 0, 512);
  naos_delay(200);
  led_set(0, 0, 0, 0);
}

static void online() {
  // disable motor
  mot_set(0);

  // subscribe local topics
  naos_subscribe("brightness", 0, NAOS_LOCAL);
  naos_subscribe("move", 0, NAOS_LOCAL);
  naos_subscribe("speed", 0, NAOS_LOCAL);
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

  // set target
  if (strcmp(topic, "move") == 0 && scope == NAOS_LOCAL) {
    target = strtod((const char *)payload, NULL);
  }

  // set speed
  if (strcmp(topic, "speed") == 0 && scope == NAOS_LOCAL) {
    speed = (int)strtol((const char *)payload, NULL, 10);
  }

  // reset position
  if (strcmp(topic, "reset") == 0 && scope == NAOS_LOCAL) {
    position = strtod((const char *)payload, NULL);
    target = strtod((const char *)payload, NULL);
  }

  // set automate
  if (strcmp(topic, "automate") == 0 && scope == NAOS_LOCAL) {
    automate = strcmp((const char *)payload, "on") == 0;
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
    position += (double)rotation_change / 20.0 * 7.5;  // TODO: Calculate average length of wire per revolution.

    // publish update
    char position_str[10];
    snprintf(position_str, 10, "%.3f", position);
    naos_publish_str("position", position_str, 0, false, NAOS_LOCAL);
  }

  // prepare new target
  double new_target = target;

  // automate positioning
  if (automate) {
    if (motion) {
      // go up and down 1cm depending on current distance
      if (distance > 25 + 1) {
        new_target = position - 1;
      } else if (distance < 25 - 1) {
        new_target = position + 1;
      }

      // constrain movement to 150cm to 250cm
      if (new_target < 150) {
        new_target = 150;
      } else if (target > 250) {
        new_target = 250;
      }
    } else {
      new_target = 100;
    }
  }

  // log target if changed
  if (new_target != target) {
    naos_log("updated target: %.3f", target);
  }

  // apply new target
  target = new_target;

  // set motor
  if (position < target + 1 && position > target - 1) {
    // break if target has been reached
    mot_set(0);
  } else if (position < target) {
    // go down
    mot_set(speed);
  } else if (position > target) {
    // go up
    mot_set(speed * -1);
  }
}

static naos_config_t config = {.device_type = "vas17",
                               .firmware_version = "0.1.0",
                               .ping_callback = ping,
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

  // initialize motion sensor
  pir_init();

  // initialize distance sensor
  dist_init();

  // initialize motor
  mot_init();

  // initialize led
  led_init();

  // initialize encoder
  enc_init();

  // initialize naos
  naos_init(&config);
}
