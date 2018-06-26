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

/* state */

typedef enum {
  OFFLINE,    // offline state
  STANDBY,    // waits for external commands
  MOVE_UP,    // moves up
  MOVE_DOWN,  // moves down
  MOVE_TO,    // moved to position
  AUTOMATE,   // moves according to sensors
  RESET,      // resets position
  REPOSITION  // reposition after a reset
} state_t;

state_t state = OFFLINE;

/* parameters */

static bool automate = false;
static double reset_height = 0;
static double winding_length = 0;
static double idle_height = 0;
static double rise_height = 0;
static int idle_light = 0;
static int flash_intensity = 0;
static int min_down_speed = 0;
static int min_up_speed = 0;
static int max_down_speed = 0;
static int max_up_speed = 0;
static int speed_map_range = 0;
static bool invert_encoder = false;
static double move_precision = 0;
static int pir_sensitivity = 0;
static int pir_interval = 0;

/* variables */

static bool motion = false;
static uint32_t last_motion = 0;
static double position = 0;
static double move_to = 0;

/* helpers */

bool approach_target(double target) {
  // check if target has been reached
  if (position < target + (move_precision / 2) && position > target - (move_precision / 2)) {
    // stop motor
    mot_set(0);

    return true;
  }

  // move up if target is below position
  if (position < target) {
    mot_set((int)a32_safe_map_d(target - position, 0, speed_map_range, min_up_speed, max_up_speed));
  }

  // move down if target is above position
  if (position > target) {
    mot_set((int)a32_safe_map_d(position - target, 0, speed_map_range, min_down_speed, max_down_speed) * -1);
  }

  return false;
}

/* state machine */

static void state_feed();

static void state_transition(state_t new_state) {
  // log state change
  naos_log("transition: %d", new_state);

  // transition state
  switch (new_state) {
    case OFFLINE: {
      // stop motor
      mot_set(0);

      // turn of led
      led_set(led_mono(0), 100);

      // set state
      state = OFFLINE;

      break;
    }

    case STANDBY: {
      // stop motor
      mot_set(0);

      // enable idle light
      led_set(led_mono(idle_light), 100);

      // set state
      state = STANDBY;

      break;
    }

    case MOVE_UP: {
      // move up
      mot_set(512);

      // set state
      state = MOVE_UP;

      break;
    }

    case MOVE_DOWN: {
      // move down
      mot_set(-512);

      // set state
      state = MOVE_DOWN;

      break;
    }

    case MOVE_TO: {
      // stop motor
      mot_set(0);

      // set state
      state = MOVE_TO;

      break;
    }

    case AUTOMATE: {
      // set state
      state = AUTOMATE;

      break;
    }

    case RESET: {
      // stop motor
      mot_set(0);

      // reset position
      position = reset_height;

      // set state
      state = RESET;

      break;
    }

    case REPOSITION: {
      // stop motor
      mot_set(0);

      // set state
      state = REPOSITION;

      break;
    }
  }

  // feed state machine
  state_feed();
}

static void state_feed() {
  switch (state) {
    case OFFLINE: {
      // do nothing
      break;
    }

    case STANDBY: {
      // do nothing
      break;
    }

    case MOVE_UP: {
      // do nothing
      break;
    }

    case MOVE_DOWN: {
      // do nothing
      break;
    }

    case MOVE_TO: {
      // approach target and transition to standby if reached
      if (approach_target(move_to)) {
        state_transition(STANDBY);
      }

      break;
    }

    case AUTOMATE: {
      // calculate target
      double target = motion ? rise_height : idle_height;

      // approach new target
      approach_target(target);

      break;
    }

    case RESET: {
      // transition to reposition state
      state_transition(REPOSITION);

      break;
    }

    case REPOSITION: {
      // approach target and transition to standby if reached
      if (approach_target(reset_height - 10)) {
        state_transition(STANDBY);
      }

      break;
    }
  }
}

/* naos callbacks */

static void ping() {
  // flash white for 100ms
  led_flash(led_white(512), 100);
}

static void online() {
  // subscribe local topics
  naos_subscribe("flash", 0, NAOS_LOCAL);
  naos_subscribe("flash-color", 0, NAOS_LOCAL);
  naos_subscribe("move", 0, NAOS_LOCAL);
  naos_subscribe("stop", 0, NAOS_LOCAL);
  naos_subscribe("disco", 0, NAOS_LOCAL);

  // transition to standby state
  state_transition(STANDBY);
}

static void offline() {
  // transition into offline state
  state_transition(OFFLINE);
}

static void message(const char *topic, uint8_t *payload, size_t len, naos_scope_t scope) {
  // perform flash
  if (strcmp(topic, "flash") == 0 && scope == NAOS_LOCAL) {
    int time = a32_str2i((const char *)payload);
    led_flash(led_mono(flash_intensity), time);
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
    led_flash(led_color(red, green, blue, white), time);
  }

  // set target
  else if (strcmp(topic, "move") == 0 && scope == NAOS_LOCAL) {
    // check for keywords
    if (strcmp((const char *)payload, "up") == 0) {
      state_transition(MOVE_UP);
    } else if (strcmp((const char *)payload, "down") == 0) {
      state_transition(MOVE_DOWN);
    } else {
      move_to = strtod((const char *)payload, NULL);
      state_transition(MOVE_TO);
    }
  }

  // stop motor
  else if (strcmp(topic, "stop") == 0 && scope == NAOS_LOCAL) {
    state_transition(STANDBY);
  }

  // perform disco
  else if (strcmp(topic, "disco") == 0 && scope == NAOS_LOCAL) {
    int r = esp_random() / 4194304;
    int g = esp_random() / 4194304;
    int b = esp_random() / 4194304;
    int w = esp_random() / 4194304;
    led_set(led_color(r, g, b, w), 100);
  }
}

static void loop() {
  // TODO: Use separate task?

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
    // update motion
    motion = new_motion;

    // publish update
    naos_publish_b("motion", motion, 0, false, NAOS_LOCAL);
  }

  // feed state machine
  state_feed();
}

/* custom callbacks */

static void end() {
  // transition in reset state
  state_transition(RESET);
}

static void enc(double rot) {
  // track last sent position
  static double sent = 0;

  // apply rotation
  position += (invert_encoder ? rot * -1 : rot) * winding_length;

  // publish update if position changed more than 1cm
  if (position > sent + 1 || position < sent - 1) {
    naos_publish_d("position", position, 0, false, NAOS_LOCAL);
    sent = position;
  }

  // feed state machine
  state_feed();
}

static naos_param_t params[] = {
    {.name = "automate", .type = NAOS_BOOL, .default_b = false, .sync_b = &automate},
    {.name = "winding-length", .type = NAOS_DOUBLE, .default_d = 7.5, .sync_d = &winding_length},
    {.name = "reset-height", .type = NAOS_DOUBLE, .default_d = 200, .sync_d = &reset_height},
    {.name = "idle-height", .type = NAOS_DOUBLE, .default_d = 100, .sync_d = &idle_height},
    {.name = "rise-height", .type = NAOS_DOUBLE, .default_d = 150, .sync_d = &rise_height},
    {.name = "idle-light", .type = NAOS_LONG, .default_l = 127, .sync_l = &idle_light},
    {.name = "flash-intensity", .type = NAOS_LONG, .default_l = 1023, .sync_l = &flash_intensity},
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
                               .num_parameters = 16,
                               .ping_callback = ping,
                               .loop_callback = loop,
                               .loop_interval = 0,
                               .online_callback = online,
                               .offline_callback = offline,
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
  enc_init(enc);

  // initialize naos
  naos_init(&config);
}
