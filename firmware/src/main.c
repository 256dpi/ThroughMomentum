#include <art32/numbers.h>
#include <art32/strconv.h>
#include <driver/adc.h>
#include <esp_system.h>
#include <naos.h>
#include <stdlib.h>
#include <string.h>

#include "enc.h"
#include "end.h"
#include "led.h"
#include "mot.h"
#include "pir.h"

bool automate = false;
double winding_length = 0;
double idle_height = 0;
double rise_height = 0;
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
double move_precision = 0;
int pir_sensitivity = 0;
int pir_interval = 0;

bool motion = false;
uint32_t last_motion = 0;
bool manual = false;
double position = 0;
double sent_position = 0;
double target = 0;
int flash_time = 0;
uint32_t flash_end = 0;

static void ping() {
  // flash white for at least 100ms
  led_set(0, 0, 0, 512, 100);
  naos_delay(100);
  led_set(0, 0, 0, 0, 100);
}

static void online() {
  // disable motor
  mot_set(0);

  // set target to current position
  target = position;

  // enable idle light
  led_set(idle_light, idle_light, idle_light, idle_light, 100);

  // subscribe local topics
  naos_subscribe("flash", 0, NAOS_LOCAL);
  naos_subscribe("flash-color", 0, NAOS_LOCAL);
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
  led_set(0, 0, 0, 0, 100);
}

static void update(const char *param, const char *value) {}

static void message(const char *topic, uint8_t *payload, size_t len, naos_scope_t scope) {
  // perform flash
  if (strcmp(topic, "flash") == 0 && scope == NAOS_LOCAL) {
    flash_time = (uint32_t)a32_str2l((const char *)payload);
    flash_end = naos_millis() + flash_time / 2;
    led_set(flash_intensity, flash_intensity, flash_intensity, flash_intensity, flash_time / 2);
  }

  // perform flash
  else if (strcmp(topic, "flash-color") == 0 && scope == NAOS_LOCAL) {
    // read colors and time
    int red = 0;
    int green = 0;
    int blue = 0;
    int white = 0;
    int time = 0;
    sscanf((const char *)payload, "%d %d %d %d %d", &red, &green, &blue, &white, &time);

    // set flash
    flash_time = (uint32_t)time;
    flash_end = naos_millis() + flash_time / 2;
    led_set(red, green, blue, white, flash_time / 2);
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
      naos_set_b("automate", false);
    }
  }

  // stop motor
  else if (strcmp(topic, "stop") == 0 && scope == NAOS_LOCAL) {
    mot_set(0);
    manual = false;
    target = position;

    if (automate) {
      naos_set_b("automate", false);
    }
  }

  // reset position
  else if (strcmp(topic, "reset") == 0 && scope == NAOS_LOCAL) {
    position = a32_str2d((const char *)payload);
    naos_set_d("saved-position", position);
    target = position;
  }

  // perform disco
  else if (strcmp(topic, "disco") == 0 && scope == NAOS_LOCAL) {
    int r = esp_random() / 4194304;
    int g = esp_random() / 4194304;
    int b = esp_random() / 4194304;
    int w = esp_random() / 4194304;
    led_set(r, g, b, w, 100);
  }
}

static void loop() {
  // calculate dynamic pir threshold
  int threshold = a32_safe_map_i((int)position, 0, (int)rise_height, 0, pir_sensitivity);

  // update timestamp if motion detected
  if (pir_read() > threshold) {
    last_motion = naos_millis();
  }

  // check if there was a motion in the last 8sec
  bool new_motion = last_motion > naos_millis() - pir_interval;

  // check motion
  if (motion != new_motion) {
    motion = new_motion;

    // publish update
    naos_publish_b("motion", motion, 0, false, NAOS_LOCAL);
  }

  // get encoder
  double rotation_change = enc_get();

  // invert if requested
  if (invert_encoder) {
    rotation_change *= -1;
  }

  // apply rotation
  position += rotation_change * winding_length;

  // publish update if position changed
  if (position > sent_position + 1 || position < sent_position - 1) {
    naos_publish_d("position", position, 0, false, NAOS_LOCAL);
    sent_position = position;
  }

  // save position if threshold has been passed
  if (position > saved_position + save_threshold || position < saved_position - save_threshold) {
    naos_set_d("saved-position", position);
  }

  // finish flash
  if (flash_end > 0 && flash_end < naos_millis()) {
    led_set(idle_light, idle_light, idle_light, idle_light, flash_time / 2);
    flash_time = 0;
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
      // move to rise height on motion
      new_target = rise_height;
    } else {
      // move to idle height if no motion
      new_target = idle_height;
    }
  }

  // apply new target
  target = new_target;

  // set motor
  if (position < target + (move_precision / 2) && position > target - (move_precision / 2)) {
    // break if target has been reached
    mot_set(0);
  } else if (position < target) {
    // go up
    mot_set((int)a32_safe_map_d(target - position, 0, speed_map_range, min_up_speed, max_up_speed));
  } else if (position > target) {
    // go down
    mot_set((int)a32_safe_map_d(position - target, 0, speed_map_range, min_down_speed, max_down_speed) * -1);
  }
}

static void end() { naos_log("end: triggered"); }

static naos_param_t params[] = {
    {.name = "automate", .type = NAOS_BOOL, .default_b = false, .sync_b = &automate},
    {.name = "winding-length", .type = NAOS_DOUBLE, .default_d = 7.5, .sync_d = &winding_length},
    {.name = "idle-height", .type = NAOS_DOUBLE, .default_d = 100, .sync_d = &idle_height},
    {.name = "rise-height", .type = NAOS_DOUBLE, .default_d = 150, .sync_d = &rise_height},
    {.name = "idle-light", .type = NAOS_LONG, .default_l = 127, .sync_l = &idle_light},
    {.name = "flash-intensity", .type = NAOS_LONG, .default_l = 1023, .sync_l = &flash_intensity},
    {.name = "save-threshold", .type = NAOS_DOUBLE, .default_d = 2, .sync_d = &save_threshold},
    {.name = "saved-position", .type = NAOS_DOUBLE, .default_d = 0, .sync_d = &saved_position},
    {.name = "min-down-speed", .type = NAOS_LONG, .default_l = 350, .sync_l = &min_down_speed},
    {.name = "min-up-speed", .type = NAOS_LONG, .default_l = 350, .sync_l = &min_up_speed},
    {.name = "max-down-speed", .type = NAOS_LONG, .default_l = 500, .sync_l = &max_down_speed},
    {.name = "max-up-speed", .type = NAOS_LONG, .default_l = 950, .sync_l = &max_up_speed},
    {.name = "speed-map-range", .type = NAOS_LONG, .default_l = 20, .sync_l = &speed_map_range},
    {.name = "invert-encoder", .type = NAOS_BOOL, .default_b = true, .sync_b = &invert_encoder},
    {.name = "move-precision", .type = NAOS_DOUBLE, .default_d = 1, .sync_d = &move_precision},
    {.name = "pir-sensitivity", .type = NAOS_LONG, .default_l = 300, .sync_l = &pir_sensitivity},
    {.name = "pir-interval", .type = NAOS_LONG, .default_l = 2000, .sync_l = &pir_interval},
};

static naos_config_t config = {.device_type = "vas17",
                               .firmware_version = "0.7.0",
                               .parameters = params,
                               .num_parameters = 17,
                               .ping_callback = ping,
                               .loop_callback = loop,
                               .loop_interval = 0,
                               .online_callback = online,
                               .offline_callback = offline,
                               .update_callback = update,
                               .message_callback = message};

void app_main() {
  // install global interrupt service
  ESP_ERROR_CHECK(gpio_install_isr_service(0));

  // initialize end stop
  end_init(&end);

  // initialize motion sensor
  pir_init();

  // initialize motor
  mot_init();

  // initialize led
  led_init();

  // initialize encoder
  enc_init();

  // initialize naos
  naos_init(&config);

  // set position
  position = saved_position;
}
