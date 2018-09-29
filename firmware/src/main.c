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

#define WINDING_LENGTH 7.5

/* state */

typedef enum {
  OFFLINE,     // offline state
  INITIALIZE,  // initialize position system
  STANDBY,     // waits for external commands
  MOVE,        // move up, down to position
  AUTOMATE,    // moves according to sensors
  RESET,       // resets position
} state_t;

state_t state = OFFLINE;

/* parameters */

static bool automate = false;
static double idle_height = 0;
static double base_height = 0;
static double rise_height = 0;
static double reset_height = 0;
static int idle_light = 0;
static bool zero_switch = false;
static bool invert_encoder = false;
static int pir_low = 0;
static int pir_high = 0;
static int pir_interval = 0;

/* variables */

static bool initialized = false;
static bool motion = false;
static double distance = 0;
static double position = 0;
static double move_to = 0;

/* state machine */

const char *state_str(state_t s) {
  switch (s) {
    case OFFLINE:
      return "OFFLINE";
    case INITIALIZE:
      return "INITIALIZE";
    case STANDBY:
      return "STANDBY";
    case MOVE:
      return "MOVE";
    case AUTOMATE:
      return "AUTOMATE";
    case RESET:
      return "RESET";
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
      mot_stop();

      // turn of led
      led_fade(led_mono(0), 100);

      break;
    }

    case INITIALIZE: {
      // stop motor
      mot_stop();

      // turn of led
      led_fade(led_color(127, 0, 0, 0), 100);

      break;
    }

    case STANDBY: {
      // stop motor
      mot_stop();

      // enable idle light
      led_fade(led_mono(idle_light), 100);

      break;
    }

    case MOVE: {
      break;
    }

    case AUTOMATE: {
      break;
    }

    case RESET: {
      // stop motor
      mot_stop();

      // reset position
      position = reset_height;

      break;
    }
  }

  // set new state
  state = new_state;

  // publish new state
  naos_publish("state", state_str(state), 0, false, NAOS_LOCAL);

  // feed state machine
  state_feed();
}

static void state_feed() {
  // TODO: This is tricky to do as the sensor lags and is not accurate.

  /*// use measurement from distance sensor to adjust position
  if (!motion && distance >= idle_height && distance <= rise_height) {
    // adjust position a little towards the measured distance
    position = a32_map_d(0.001, 0, 1, position, distance);
  }*/

  // publish update if position changed more than 1cm
  static double _position = 0;
  if (position > _position + 1 || position < _position - 1) {
    naos_publish_d("position", position, 0, false, NAOS_LOCAL);
    _position = position;
  }

  // publish update if distance changed more than 2cm
  static double _distance = 0;
  if (distance > _distance + 2 || distance < _distance - 2) {
    naos_publish_d("distance", distance, 0, false, NAOS_LOCAL);
    _distance = distance;
  }

  // publish update if motion has been changed
  static bool _motion = false;
  if (motion != _motion) {
    naos_publish_b("motion", motion, 0, false, NAOS_LOCAL);
    _motion = motion;
  }

  switch (state) {
    case OFFLINE: {
      // do nothing

      break;
    }

    case INITIALIZE: {
      // TODO: Make sure the used distance has been sample over a long period?

      // use measurement from distance sensor for initial position
      if (!motion && distance >= idle_height && distance <= rise_height) {
        position = distance;
        initialized = true;
        state_transition(STANDBY);
      }

      // TODO: Use physical switch if not possible for a longer period of time?
      /*// move up to trigger reset if automate is on
      if (automate) {
        mot_approach(position, 1000, 1);
      }*/

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
        state_transition(initialized ? STANDBY : INITIALIZE);
      }

      break;
    }

    case AUTOMATE: {
      // transition back to standby if disabled
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
      // approach target, set initialized flag and transition to standby if reached
      if (mot_approach(position, reset_height - 10, 1)) {
        initialized = true;
        state_transition(STANDBY);
      }

      break;
    }
  }
}

/* naos callbacks */

static void ping() {
  // flash white
  led_flash(led_white(512), 100);
}

static void online() {
  // subscribe local topics
  naos_subscribe("move", 0, NAOS_LOCAL);
  naos_subscribe("stop", 0, NAOS_LOCAL);
  naos_subscribe("fade", 0, NAOS_LOCAL);
  naos_subscribe("flash", 0, NAOS_LOCAL);

  // transition to initialize if not yet initialized or standby otherwise
  state_transition(initialized ? STANDBY : INITIALIZE);
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
  // check for "move" command
  if (strcmp(topic, "move") == 0 && scope == NAOS_LOCAL) {
    // set target
    if (strcmp((const char *)payload, "up") == 0) {
      move_to = 1000;
    } else if (strcmp((const char *)payload, "down") == 0) {
      move_to = -1000;
    } else {
      move_to = a32_constrain_d(strtod((const char *)payload, NULL), idle_height, reset_height);
    }

    // change state if safe
    if (state != RESET) {
      state_transition(MOVE);
    }
  }

  // check for "stop" command
  else if (strcmp(topic, "stop") == 0 && scope == NAOS_LOCAL) {
    // disable automate
    naos_set_b("automate", false);

    // change state if safe
    if (state != RESET) {
      state_transition(initialized ? STANDBY : INITIALIZE);
    }
  }

  // check for "fade" command
  else if (strcmp(topic, "fade") == 0 && scope == NAOS_LOCAL) {
    // read colors and time
    int red = 0;
    int green = 0;
    int blue = 0;
    int white = 0;
    int time = 0;
    sscanf((const char *)payload, "%d %d %d %d %d", &red, &green, &blue, &white, &time);

    // fade color
    led_fade(led_color(red, green, blue, white), time);
  }

  // check for "flash" command
  else if (strcmp(topic, "flash") == 0 && scope == NAOS_LOCAL) {
    // read colors and time
    int red = 0;
    int green = 0;
    int blue = 0;
    int white = 0;
    int time = 0;
    sscanf((const char *)payload, "%d %d %d %d %d", &red, &green, &blue, &white, &time);

    // flash color
    led_flash(led_color(red, green, blue, white), time);
  }
}

static void loop() {
  // feed state machine
  state_feed();
}

/* custom callbacks */

static void pir(int m) {
  // track last motion
  static uint32_t last = 0;

  // calculate dynamic pir threshold
  int threshold = a32_safe_map_i((int)position, (int)idle_height, (int)rise_height, pir_low, pir_high);

  // update timestamp if motion detected
  if (m > threshold) {
    last = naos_millis();
  }

  // check if there was a motion in the last interval
  motion = last > naos_millis() - pir_interval;

  // feed state machine
  state_feed();
}

static void end() {
  // transition to reset if zero switch is enabled
  if (zero_switch) {
    state_transition(RESET);
  }
}

static void enc(double r) {
  // apply rotation
  position += (invert_encoder ? r * -1 : r) * WINDING_LENGTH;

  // feed state machine
  state_feed();
}

static void dst(double d) {
  // update distance
  distance = d;

  // feed state machine
  state_feed();
}

/* initialization */

static naos_param_t params[] = {
    {.name = "automate", .type = NAOS_BOOL, .default_b = false, .sync_b = &automate},
    {.name = "idle-height", .type = NAOS_DOUBLE, .default_d = 50, .sync_d = &idle_height},
    {.name = "base-height", .type = NAOS_DOUBLE, .default_d = 100, .sync_d = &base_height},
    {.name = "rise-height", .type = NAOS_DOUBLE, .default_d = 150, .sync_d = &rise_height},
    {.name = "reset-height", .type = NAOS_DOUBLE, .default_d = 200, .sync_d = &reset_height},
    {.name = "idle-light", .type = NAOS_LONG, .default_l = 127, .sync_l = &idle_light},
    {.name = "zero-switch", .type = NAOS_BOOL, .default_b = true, .sync_b = &zero_switch},
    {.name = "invert-encoder", .type = NAOS_BOOL, .default_b = true, .sync_b = &invert_encoder},
    {.name = "pir-low", .type = NAOS_LONG, .default_l = 200, .sync_l = &pir_low},
    {.name = "pir-high", .type = NAOS_LONG, .default_l = 400, .sync_l = &pir_high},
    {.name = "pir-interval", .type = NAOS_LONG, .default_l = 2000, .sync_l = &pir_interval},
};

static naos_config_t config = {.device_type = "tm-lo",
                               .firmware_version = "1.2.3",
                               .parameters = params,
                               .num_parameters = 11,
                               .ping_callback = ping,
                               .online_callback = online,
                               .offline_callback = offline,
                               .update_callback = update,
                               .message_callback = message,
                               .loop_callback = loop,
                               .loop_interval = 1,
                               .password = "tm2018"};

void app_main() {
  // install global interrupt service
  ESP_ERROR_CHECK(gpio_install_isr_service(0));

  // initialize motor
  mot_init();

  // initialize led
  led_init();

  // initialize naos
  naos_init(&config);

  // initialize motion sensor
  pir_init(pir);

  // initialize end stop
  end_init(end);

  // initialize encoder
  enc_init(enc);

  // initialize distance sensor
  dst_init(dst);
}
