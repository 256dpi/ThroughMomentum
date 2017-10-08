#include <art32/numbers.h>
#include <art32/strconv.h>
#include <driver/adc.h>
#include <driver/ledc.h>
#include <esp_system.h>
#include <naos.h>
#include <string.h>

#include "dist.h"
#include "enc.h"
#include "led.h"
#include "mot.h"
#include "pir.h"

bool automate = false;
double winding_length = 0;
double idle_height = 0;
double rise_height = 0;
double max_height = 0;
double target_distance = 0;
int idle_light = 0;
int flash_intensity = 0;
double save_threshold = 0;
double saved_position = -9999;
int min_down_speed = 0;
int min_up_speed = 0;
int max_down_speed = 0;
int max_up_speed = 0;
int speed_map_range = 0;
bool invert_encoder = false;

bool motion = false;
uint32_t last_distance = 0;
bool manual = false;
double position = 0;
double target = 0;
uint32_t flash_end = 0;

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
  naos_ensure("automate", "off");
  naos_ensure("winding-length", "7.5");
  naos_ensure("idle-height", "100");
  naos_ensure("rise-height", "150");
  naos_ensure("max-height", "200");
  naos_ensure("target-distance", "25");
  naos_ensure("idle-light", "127");
  naos_ensure("flash-intensity", "1023");
  naos_ensure("save-threshold", "2");
  naos_ensure("saved-position", "0");
  naos_ensure("min-down-speed", "350");
  naos_ensure("min-up-speed", "350");
  naos_ensure("max-down-speed", "500");
  naos_ensure("max-up-speed", "950");
  naos_ensure("speed-map-range", "20");
  naos_ensure("invert-encoder", "true");

  // read settings
  automate = strcmp(naos_get("automate"), "on") == 0;
  winding_length = a32_str2d(naos_get("winding-length"));
  idle_height = a32_str2d(naos_get("idle-height"));
  rise_height = a32_str2d(naos_get("rise-height"));
  max_height = a32_str2d(naos_get("max-height"));
  target_distance = a32_str2d(naos_get("target-distance"));
  idle_light = a32_str2i(naos_get("idle-light"));
  flash_intensity = a32_str2i(naos_get("flash-intensity"));
  position = a32_str2d(naos_get("saved-position"));
  min_down_speed = a32_str2i(naos_get("min-down-speed"));
  min_up_speed = a32_str2i(naos_get("min-up-speed"));
  max_down_speed = a32_str2i(naos_get("max-down-speed"));
  max_up_speed = a32_str2i(naos_get("max-up-speed"));
  speed_map_range = a32_str2i(naos_get("speed-map-range"));
  invert_encoder = strcmp(naos_get("invert-encoder"), "true") == 0;

  // set target to current position
  target = position;

  // read position on first boot
  if (saved_position == -9999) {
    saved_position = a32_str2d(naos_get("saved-position"));
  }

  // enable idle light
  led_set(idle_light, idle_light, idle_light, idle_light);

  // subscribe local topics
  naos_subscribe("flash", 0, NAOS_LOCAL);
  naos_subscribe("turn", 0, NAOS_LOCAL);
  naos_subscribe("move", 0, NAOS_LOCAL);
  naos_subscribe("stop", 0, NAOS_LOCAL);
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
  // set automate
  if (strcmp(param, "automate") == 0) {
    automate = strcmp(value, "on") == 0;
  }

  // set winding length
  else if (strcmp(param, "winding-length") == 0) {
    winding_length = a32_str2d(value);
  }

  // set idle height
  else if (strcmp(param, "idle-height") == 0) {
    idle_height = a32_str2d(value);
  }

  // set rise height
  else if (strcmp(param, "rise-height") == 0) {
    rise_height = a32_str2d(value);
  }

  // set max height
  else if (strcmp(param, "max-height") == 0) {
    max_height = a32_str2d(value);
  }

  // set target distance
  else if (strcmp(param, "target-distance") == 0) {
    target_distance = a32_str2d(value);
  }

  // set idle light
  else if (strcmp(param, "idle-light") == 0) {
    idle_light = a32_str2i(value);
  }

  // set flash intensity
  else if (strcmp(param, "flash-intensity") == 0) {
    flash_intensity = a32_str2i(value);
  }

  // set save threshold
  else if (strcmp(param, "save-threshold") == 0) {
    save_threshold = a32_str2i(value);
  }

  // set min down speed
  else if (strcmp(param, "min-down-speed") == 0) {
    min_down_speed = a32_str2i(value);
  }

  // set min up speed
  else if (strcmp(param, "min-up-speed") == 0) {
    min_up_speed = a32_str2i(value);
  }

  // set max down speed
  else if (strcmp(param, "max-down-speed") == 0) {
    max_down_speed = a32_str2i(value);
  }

  // set max up speed
  else if (strcmp(param, "max-up-speed") == 0) {
    max_up_speed = a32_str2i(value);
  }

  // set speed map range
  else if (strcmp(param, "speed-map-range") == 0) {
    speed_map_range = a32_str2i(value);
  }

  // set invert encoder
  if (strcmp(param, "invert-encoder") == 0) {
    invert_encoder = strcmp(value, "true") == 0;
  }
}

static void message(const char *topic, uint8_t *payload, size_t len, naos_scope_t scope) {
  // perform flash
  if (strcmp(topic, "flash") == 0 && scope == NAOS_LOCAL) {
    flash_end = naos_millis() + (uint32_t)strtol((const char *)payload, NULL, 10);
    led_set(flash_intensity, flash_intensity, flash_intensity, flash_intensity);
  }

  // set turn
  else if (strcmp(topic, "turn") == 0 && scope == NAOS_LOCAL) {
    if (strcmp((const char *)payload, "up") == 0) {
      manual = true;
      mot_set(512);
    } else if (strcmp((const char *)payload, "down") == 0) {
      manual = true;
      mot_set(-512);
    }
  }

  // set target
  else if (strcmp(topic, "move") == 0 && scope == NAOS_LOCAL) {
    target = strtod((const char *)payload, NULL);

    if (automate) {
      automate = false;
      naos_set("automate", "off");
    }
  }

  // stop motor
  else if (strcmp(topic, "stop") == 0 && scope == NAOS_LOCAL) {
    mot_set(0);
    manual = false;
    target = position;

    if (automate) {
      automate = false;
      naos_set("automate", "off");
    }
  }

  // reset position
  else if (strcmp(topic, "reset") == 0 && scope == NAOS_LOCAL) {
    position = strtod((const char *)payload, NULL);
    target = strtod((const char *)payload, NULL);
  }

  // perform disco
  else if (strcmp(topic, "disco") == 0 && scope == NAOS_LOCAL) {
    int r = esp_random() / 4194304;
    int g = esp_random() / 4194304;
    int b = esp_random() / 4194304;
    int w = esp_random() / 4194304;
    led_set(r, g, b, w);
  }
}

static void loop() {
//  int pir = pir_read();
//  if(pir > 10) {
//    naos_log("pir: %d", pir_read());
//  }

  // read pir sensor
  bool new_motion = pir_get();

  // check motion
  if (motion != new_motion) {
    motion = new_motion;

    // publish update
    naos_publish("motion", a32_l2str(motion ? 1 : 0), 0, false, NAOS_LOCAL);
  }

  // read distance
  double distance = dist_get();

  // publish distance every second
  if (last_distance + 1000 < naos_millis()) {
    naos_publish("distance", a32_d2str(distance), 0, false, NAOS_LOCAL);
    last_distance = naos_millis();
  }

  // get encoder
  double rotation_change = enc_get();

  // invert if requested
  if (invert_encoder) {
    rotation_change *= -1;
  }

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

  // finish flash
  if (flash_end > 0 && flash_end < naos_millis()) {
    led_set(idle_light, idle_light, idle_light, idle_light);
    flash_end = 0;
  }

  // return immediately in manual mode
  if (manual) {
    return;
  }

  // prepare new target
  double new_target = target;

  // automate positioning
  if (automate) {
    if (motion) {
      // go up and down 1cm depending on current distance
      if (distance > target_distance + 1) {
        new_target = position - (distance - target_distance);
      } else if (distance < target_distance - 1) {
        new_target = position + (target_distance - distance);
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
    naos_log("updated target: %f", target);
  }

  // apply new target
  target = new_target;

  // set motor
  if (position < target + 1 && position > target - 1) {
    // break if target has been reached
    mot_set(0);
  } else if (position < target) {
    // go up
    mot_set((int)a32_safe_map_d(target - position, 0, speed_map_range, min_up_speed, max_up_speed));
  } else if (position > target) {
    // go down
    mot_set((int)a32_safe_map_d(position - target, 0, speed_map_range, min_down_speed * -1, max_down_speed * -1));
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

  // install ledc fade service
  ESP_ERROR_CHECK(ledc_fade_func_install(0));

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
