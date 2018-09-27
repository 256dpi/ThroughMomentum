#include <art32/motion.h>
#include <art32/numbers.h>
#include <art32/strconv.h>
#include <driver/adc.h>
#include <esp_system.h>
#include <math.h>
#include <naos.h>
#include <stdlib.h>
#include <string.h>

#include "dst.h"
#include "enc.h"
#include "end.h"
#include "led.h"
#include "mot.h"
#include "pir.h"

// TODO: Make pir async.

/* state */

typedef enum {
  OFFLINE,    // offline state
  STANDBY,    // waits for external commands
  MOVE,       // move up, down to position
  AUTOMATE,   // moves according to sensors
  RESET,      // resets position
  REPOSITION  // reposition after a reset
} state_t;

state_t state = OFFLINE;

/* parameters */

static bool automate = false;
static double idle_height = 0;
static double base_height = 0;
static double rise_height = 0;
static double reset_height = 0;
static int idle_light = 0;
static int flash_intensity = 0;
static bool zero_switch = false;
static bool invert_encoder = false;
static int pir_sensitivity = 0;
static int pir_interval = 0;
static double winding_length = 0;

/* variables */

static bool motion = false;
static double distance = 0;
static double position = 0;
static double move_to = 0;

/* state machine */

const char *state_str(state_t s) {
  switch (s) {
    case OFFLINE:
      return "OFFLINE";
    case STANDBY:
      return "STANDBY";
    case MOVE:
      return "MOVE";
    case AUTOMATE:
      return "AUTOMATE";
    case RESET:
      return "RESET";
    case REPOSITION:
      return "REPOSITION";
  }

  return "";
}

static void state_feed();

static void state_transition(state_t new_state) {
  // return if already in state
  if (new_state == state) {
    return;
  }

  // log state change
  naos_log("transition: %s", state_str(new_state));

  // transition state
  switch (new_state) {
    case OFFLINE: {
      // stop motor
      mot_hard_stop();

      // turn of led
      led_set(led_mono(0), 100);

      // set state
      state = OFFLINE;

      break;
    }

    case STANDBY: {
      // stop motor
      mot_hard_stop();

      // enable idle light
      led_set(led_mono(idle_light), 100);

      // set state
      state = STANDBY;

      break;
    }

    case MOVE: {
      // set state
      state = MOVE;

      break;
    }

    case AUTOMATE: {
      // set state
      state = AUTOMATE;

      break;
    }

    case RESET: {
      // stop motor
      mot_hard_stop();

      // reset position
      position = reset_height;

      // set state
      state = RESET;

      break;
    }

    case REPOSITION: {
      // set state
      state = REPOSITION;

      break;
    }
  }

  // publish new state
  naos_publish("state", state_str(state), 0, false, NAOS_LOCAL);

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
      // transition to automate if enabled
      if (automate) {
        state_transition(AUTOMATE);
      }

      break;
    }

    case MOVE: {
      // approach target and transition to standby if reached
      if (mot_approach(position, move_to, 1)) {
        state_transition(STANDBY);
      }

      break;
    }

    case AUTOMATE: {
      // transition to standby if disabled
      if (!automate) {
        state_transition(STANDBY);
      }

      // default target to idle height
      double target = idle_height;

      // check if we have motion or something below
      if (motion || distance < idle_height) {
        // approach object
        target = a32_constrain_d(position + (-distance + 20), base_height, rise_height);
      }

      // approach new target
      mot_approach(position, target, 1);

      break;
    }

    case RESET: {
      // transition to reposition state
      state_transition(REPOSITION);

      break;
    }

    case REPOSITION: {
      // approach target and transition to standby if reached
      if (mot_approach(position, reset_height - 5, 1)) {
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
  naos_subscribe("move", 0, NAOS_LOCAL);
  naos_subscribe("stop", 0, NAOS_LOCAL);
  naos_subscribe("flash", 0, NAOS_LOCAL);
  naos_subscribe("flash-color", 0, NAOS_LOCAL);
  naos_subscribe("disco", 0, NAOS_LOCAL);

  // transition to standby state
  state_transition(STANDBY);
}

static void offline() {
  // transition into offline state
  state_transition(OFFLINE);
}

static void update(const char *param, const char *value) {
  // feed state machine
  state_feed();
}

static void message(const char *topic, uint8_t *payload, size_t len, naos_scope_t scope) {
  // set target
  if (strcmp(topic, "move") == 0 && scope == NAOS_LOCAL) {
    // set target
    if (strcmp((const char *)payload, "up") == 0) {
      move_to = 1000;
    } else if (strcmp((const char *)payload, "down") == 0) {
      move_to = -1000;
    } else {
      move_to = a32_constrain_d(strtod((const char *)payload, NULL), idle_height, reset_height);
    }

    // change state
    state_transition(MOVE);
  }

  // stop motor
  else if (strcmp(topic, "stop") == 0 && scope == NAOS_LOCAL) {
    naos_set_b("automate", false);
    state_transition(STANDBY);
  }

  // set color
  else if (strcmp(topic, "color") == 0 && scope == NAOS_LOCAL) {
    // read colors and time
    int red = 0;
    int green = 0;
    int blue = 0;
    int white = 0;
    int time = 0;
    sscanf((const char *)payload, "%d %d %d %d %d", &red, &green, &blue, &white, &time);

    // set flash
    led_set(led_color(red, green, blue, white), time);
  }

  // perform flash
  else if (strcmp(topic, "flash") == 0 && scope == NAOS_LOCAL) {
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

  // track last motion
  static uint32_t last_motion = 0;

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

  // track last sent position
  static double sent = 0;

  // publish update if distance changed more than 2cm
  if (distance > sent + 2 || distance < sent - 2) {
    naos_publish_d("distance", distance, 0, false, NAOS_LOCAL);
    sent = distance;
  }

  // feed state machine
  state_feed();
}

/* custom callbacks */

static void end() {
  // ignore when already in reset or reposition state
  if (state == RESET || state == REPOSITION || !zero_switch) {
    return;
  }

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

static void dst(double d) {
  // update distance
  distance = d;
}

static naos_param_t params[] = {
    {.name = "automate", .type = NAOS_BOOL, .default_b = false, .sync_b = &automate},
    {.name = "idle-height", .type = NAOS_DOUBLE, .default_d = 50, .sync_d = &idle_height},
    {.name = "base-height", .type = NAOS_DOUBLE, .default_d = 100, .sync_d = &base_height},
    {.name = "rise-height", .type = NAOS_DOUBLE, .default_d = 150, .sync_d = &rise_height},
    {.name = "reset-height", .type = NAOS_DOUBLE, .default_d = 200, .sync_d = &reset_height},
    {.name = "idle-light", .type = NAOS_LONG, .default_l = 127, .sync_l = &idle_light},
    {.name = "flash-intensity", .type = NAOS_LONG, .default_l = 1023, .sync_l = &flash_intensity},
    {.name = "zero-switch", .type = NAOS_BOOL, .default_b = true, .sync_b = &zero_switch},
    {.name = "invert-encoder", .type = NAOS_BOOL, .default_b = true, .sync_b = &invert_encoder},
    {.name = "pir-sensitivity", .type = NAOS_LONG, .default_l = 300, .sync_l = &pir_sensitivity},
    {.name = "pir-interval", .type = NAOS_LONG, .default_l = 2000, .sync_l = &pir_interval},
    {.name = "winding-length", .type = NAOS_DOUBLE, .default_d = 7.5, .sync_d = &winding_length},
};

static naos_config_t config = {.device_type = "tm-lo",
                               .firmware_version = "1.1.1",
                               .parameters = params,
                               .num_parameters = 12,
                               .ping_callback = ping,
                               .loop_callback = loop,
                               .loop_interval = 1,
                               .online_callback = online,
                               .offline_callback = offline,
                               .update_callback = update,
                               .message_callback = message,
                               .password = "tm2018"};

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

  // initialize distance sensor
  dst_init(dst);
}
