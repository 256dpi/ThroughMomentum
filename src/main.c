#include <driver/adc.h>
#include <driver/gpio.h>
#include <math.h>
#include <naos.h>
#include <stdlib.h>
#include <string.h>

#include "dist.h"
#include "led.h"
#include "mot.h"
#include "pir.h"

bool automate = false;

bool motion = false;
int distance = 0;
bool go_up = true;

static void online() {
  // disable motor
  mot_set(0);

  // subscribe local topics
  naos_subscribe("speed", 0, NAOS_LOCAL);
  naos_subscribe("brightness", 0, NAOS_LOCAL);
  naos_subscribe("automate", 0, NAOS_LOCAL);
}

static void offline() {
  // disable motor
  mot_set(0);
}

static void message(const char *topic, uint8_t *payload, size_t len, naos_scope_t scope) {
  // set motor speed
  if (strcmp(topic, "speed") == 0 && scope == NAOS_LOCAL) {
    int speed = (int)strtol((const char *)payload, NULL, 10);
    mot_set(speed);
  }

  // set led brightness
  if (strcmp(topic, "brightness") == 0 && scope == NAOS_LOCAL) {
    int brightness = (int)strtol((const char *)payload, NULL, 10);
    led_set(brightness, brightness, brightness, brightness);
  }

  // set automate
  if(strcmp(topic, "automate") == 0) {
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
  bool distance_changed = new_distance > distance + 5 || new_distance < distance - 5;

  // get dist change (+=up, -=down)
  int distance_change = (distance - new_distance) * -1;

  // check dist
  if (distance_changed) {
    distance = new_distance;

    // publish update
    naos_publish_int("distance", distance, 0, false, NAOS_LOCAL);
  }

  // exit if no automated or distance and motion have not changed
  if(!automate || (!distance_changed && !motion_changed)) {
    return;
  }

  // log distance change
  naos_log("distance change: %d", distance_change);

  // set target distance
  int target = 100;
  if(motion) {
    target = 25;
  }

  // log target
  naos_log("target: %d", target);

  // check if target has been reached
  if (distance < target + 10 && distance > target - 10) {
    naos_log("target reached!");
    mot_set(0);
    return;
  }

  // if light goes down but needs to go up
  if (distance_change < 0 && target > distance) {
    // drive up
    naos_log("drive up!");
    go_up = !go_up;
  }

  // if light goes up but needs to go down
  if (distance_change > 0 && target < distance) {
    // drive down
    naos_log("drive down!");
    go_up = !go_up;
  }

  // set motor speed
  mot_set(go_up ? 750 : -750);
}

static naos_config_t config = {.device_type = "vas17",
                               .firmware_version = "0.1.0",
                               .loop_callback = loop,
                               .loop_interval = 100,
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
  mot_set(0);

  // initialize led
  led_init();
  led_set(0, 0, 0, 0);

  // initialize naos
  naos_init(&config);
}
