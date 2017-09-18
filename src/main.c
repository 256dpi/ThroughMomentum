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

bool last_pir = false;
int last_dist = 0;

static void online() {
  // disable motor
  mot_set(0);

  // subscribe local topics
  naos_subscribe("speed", 0, NAOS_LOCAL);
  naos_subscribe("brightness", 0, NAOS_LOCAL);
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
}

static void loop() {
  // read pir sensor
  bool new_pir = pir_get();

  // check pir state
  if (last_pir != new_pir) {
    last_pir = new_pir;

    // publish update
    if (new_pir) {
      naos_publish_int("motion", 1, 0, false, NAOS_LOCAL);
    } else {
      naos_publish_int("motion", 0, 0, false, NAOS_LOCAL);
    }
  }

  // read distance
  int new_dist = (int)round(dist_get());

  // check dist
  if (last_dist != new_dist) {
    last_dist = new_dist;

    // publish update
    naos_publish_int("distance", new_dist, 0, false, NAOS_LOCAL);
  }
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
