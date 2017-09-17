#include <driver/adc.h>
#include <naos.h>
#include <driver/gpio.h>

#include "pir.h"
#include "dist.h"

bool last_pir = false;
double last_dist = 0;

void online() {}

void message(const char *topic, uint8_t *payload, size_t len, naos_scope_t scope) {}

void loop() {
  // check pir state
  if(last_pir != pir_get()) {
    last_pir = pir_get();

    if(last_pir) {
      naos_log("hello");
    } else {
      naos_log("bye");
    }
  }

  // check dist
  if(last_dist != dist_get()) {
    last_dist = dist_get();

    naos_log("dist: %lf", last_dist);
  }
}

static naos_config_t config = {.device_type = "vas17",
                               .firmware_version = "0.1.0",
                               .loop_callback = loop,
                               .loop_interval = 0,
                               .online_callback = online,
                               .message_callback = message};

void app_main() {
  // set global adc width
  adc1_config_width(ADC_WIDTH_10Bit);

  // install global interrupt service
  gpio_install_isr_service(0);

  // initialize naos
  naos_init(&config);

  // initialize motion sensor
  pir_init();

  // initialize distance sensor
  dist_init();
}
