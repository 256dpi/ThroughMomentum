#include <art32/strconv.h>
#include <driver/adc.h>
#include <esp_system.h>
#include <math.h>
#include <naos.h>
#include <string.h>

#include "dist.h"
#include "enc.h"
#include "led.h"
#include "mot.h"
#include "pir.h"

double winding_length = 0;
double idle_height = 0;
double rise_height = 0;
double max_height = 0;
bool automate = false;
int idle_light = 0;
int flash_intensity = 0;
double save_threshold = 0;
double saved_position = -9999;

uint32_t flash_end = 0;
int speed = 0;
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

  // ensure defaults
  naos_ensure("winding-length", "7.5");
  naos_ensure("idle-height", "100");
  naos_ensure("rise-height", "150");
  naos_ensure("max-height", "200");
  naos_ensure("automate", "off");
  naos_ensure("idle-light", "127");
  naos_ensure("flash-intensity", "1023");
  naos_ensure("save-threshold", "2");
  naos_ensure("saved-position", "0");

  // read settings
  winding_length = a32_str2d(naos_get("winding-length"));
  idle_height = a32_str2d(naos_get("idle-height"));
  rise_height = a32_str2d(naos_get("rise-height"));
  max_height = a32_str2d(naos_get("max-height"));
  automate = strcmp(naos_get("automate"), "on") == 0;
  idle_light = a32_str2i(naos_get("idle-light"));
  flash_intensity = a32_str2i(naos_get("flash-intensity"));

  // read position on first boot
  if (saved_position == -9999) {
    saved_position = a32_str2d(naos_get("saved-position"));
  }

  // enable idle light
  led_set(0, 0, 0, idle_light);

  // subscribe local topics
  naos_subscribe("flash", 0, NAOS_LOCAL);
  naos_subscribe("move", 0, NAOS_LOCAL);
  naos_subscribe("stop", 0, NAOS_LOCAL);
  naos_subscribe("speed", 0, NAOS_LOCAL);
  naos_subscribe("reset", 0, NAOS_LOCAL);
  naos_subscribe("disco", 0, NAOS_LOCAL);
}

static void offline() {
  // disable motor
  mot_set(0);

  // disabled led
  led_set(0, 0, 0, 0);
}

static void update(const char *param, const char *value) {
  // set winding length
  if (strcmp(param, "winding-length") == 0) {
    winding_length = a32_str2d(value);
  }

  // set idle height
  if (strcmp(param, "idle-height") == 0) {
    idle_height = a32_str2d(value);
  }

  // set rise height
  if (strcmp(param, "rise-height") == 0) {
    rise_height = a32_str2d(value);
  }

  // set max height
  if (strcmp(param, "max-height") == 0) {
    max_height = a32_str2d(value);
  }

  // set automate
  if (strcmp(param, "automate") == 0) {
    automate = strcmp(value, "on") == 0;
  }

  // set idle light
  if (strcmp(param, "idle-light") == 0) {
    idle_light = a32_str2i(value);
  }

  // set flash intensity
  if (strcmp(param, "flash-intensity") == 0) {
    flash_intensity = a32_str2i(value);
  }

  // set save threshold
  if (strcmp(param, "save-threshold") == 0) {
    save_threshold = a32_str2i(value);
  }
}

static void message(const char *topic, uint8_t *payload, size_t len, naos_scope_t scope) {
  // perform flash
  if (strcmp(topic, "flash") == 0 && scope == NAOS_LOCAL) {
    flash_end = naos_millis() + (uint32_t)strtol((const char *)payload, NULL, 10);
    led_set(0, 0, 0, flash_intensity);
  }

  // set target
  if (strcmp(topic, "move") == 0 && scope == NAOS_LOCAL) {
    target = strtod((const char *)payload, NULL);
  }

  // stop motor
  if (strcmp(topic, "stop") == 0 && scope == NAOS_LOCAL) {
    target = position;
    mot_set(0);
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

  // perform disco
  if (strcmp(topic, "disco") == 0 && scope == NAOS_LOCAL) {
    int r = esp_random() / 4194304;
    int g = esp_random() / 4194304;
    int b = esp_random() / 4194304;
    int w = esp_random() / 4194304;
    led_set(r, g, b, w);
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
    naos_publish("motion", a32_l2str(motion ? 1 : 0), 0, false, NAOS_LOCAL);
  }

  // read distance
  int new_distance = (int)round(dist_get());

  // TODO: Threshold check really needed?

  // check if distance changed
  bool distance_changed = new_distance > distance + 2 || new_distance < distance - 2;

  // check dist
  if (distance_changed) {
    distance = new_distance;

    // publish update
    naos_publish("distance", a32_l2str(distance), 0, false, NAOS_LOCAL);
  }

  // get encoder
  double rotation_change = enc_get();

  // apply rotation
  if (rotation_change != 0) {
    position += rotation_change * winding_length;

    // publish update
    naos_publish("position", a32_d2str(position), 0, false, NAOS_LOCAL);
  }

  // save position if threshold has been passed
  if (position > saved_position + save_threshold || position < saved_position - save_threshold) {
    naos_set("saved-position", a32_d2str(position));
    saved_position = position;
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
      if (new_target < rise_height) {
        new_target = rise_height;
      } else if (target > max_height) {
        new_target = max_height;
      }
    } else {
      new_target = idle_height;
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

  // finish flash
  if (flash_end > 0 && flash_end < naos_millis()) {
    led_set(0, 0, 0, idle_light);
    flash_end = 0;
  }
}

static naos_config_t config = {.device_type = "vas17",
                               .firmware_version = "0.1.0",
                               .ping_callback = ping,
                               .loop_callback = loop,
                               .loop_interval = 0,
                               .online_callback = online,
                               .offline_callback = offline,
                               .update_callback = update,
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
