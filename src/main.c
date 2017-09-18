#include <driver/adc.h>
#include <driver/gpio.h>
#include <naos.h>
#include <stdlib.h>
#include <string.h>

#include "dist.h"
#include "mot.h"
#include "pir.h"

bool last_pir = false;
double last_dist = 0;

void online() {
  // disable motor
  mot_set(0);

  // subscribe local topics
  naos_subscribe("speed", 0, NAOS_LOCAL);
}

void offline() {
  // disable motor
  mot_set(0);
}

void message(const char *topic, uint8_t *payload, size_t len, naos_scope_t scope) {
  // set motor speed
  if (strcmp(topic, "speed") == 0 && scope == NAOS_LOCAL) {
    int speed = (int)strtol((const char *)payload, NULL, 10);
    naos_log("speed: %d", speed);
    mot_set(speed);
  }
}

void loop() {
  //  // check pir state
  //  if (last_pir != pir_get()) {
  //    last_pir = pir_get();
  //
  //    if (last_pir) {
  //      naos_log("hello");
  //    } else {
  //      naos_log("bye");
  //    }
  //  }
  //
  //  // check dist
  //  if (last_dist != dist_get()) {
  //    last_dist = dist_get();
  //
  //    naos_log("dist: %lf", last_dist);
  //  }
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

  // initialize naos
  naos_init(&config);
}
