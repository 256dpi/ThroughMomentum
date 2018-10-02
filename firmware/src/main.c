#include <art32/motion.h>
#include <art32/numbers.h>
#include <art32/smooth.h>
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

#define CALIBRATION_SAMPLES 20
#define CALIBRATION_TIMEOUT 1000 * 120

/* state */

typedef enum {
  OFFLINE,    // offline state
  CALIBRATE,  // initialize position system
  STANDBY,    // waits for external commands
  MOVE,       // move up, down to position
  AUTOMATE,   // moves according to sensors
  RESET,      // resets position
} state_t;

state_t state = -1;

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
static int calib_interval = 0;

/* variables */

static bool motion = false;
static double distance = 0;
static double position = 0;
static double usage = 0;

static double move_to = 0;

static bool calibrated = false;
static a32_smooth_t *calibration_data = NULL;
static uint32_t calibration_timeout = 0;

/* state machine */

const char *state_str(state_t s) {
  switch (s) {
    case OFFLINE:
      return "OFFLINE";
    case CALIBRATE:
      return "CALIBRATE";
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

      // set led to red
      led_fade(led_color(127, 0, 0, 0), 100);

      break;
    }

    case CALIBRATE: {
      // set flag
      calibrated = false;

      // stop motor
      mot_stop();

      // turn led to blue
      led_fade(led_color(0, 0, 127, 0), 100);

      // free existing calibration
      if (calibration_data != NULL) {
        a32_smooth_free(calibration_data);
      }

      // create new calibration
      calibration_data = a32_smooth_new(CALIBRATION_SAMPLES);

      // save current time
      calibration_timeout = naos_millis() + CALIBRATION_TIMEOUT;

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
      // turn led to green
      led_fade(led_color(0, 127, 0, 0), 100);

      break;
    }

    case AUTOMATE: {
      // enable idle light
      led_fade(led_mono(idle_light), 100);

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

    case CALIBRATE: {
      // perform physical calibration if automate is on and timeout has been reached
      if (automate && calibration_timeout < naos_millis()) {
        mot_approach(position, 1000, 1);
        break;
      }

      // calibrate if we have all samples and error is within 2cm
      if (calibration_data->count == CALIBRATION_SAMPLES && calibration_data->max - calibration_data->min < 2) {
        position = calibration_data->total / calibration_data->num;
        calibrated = true;
        state_transition(STANDBY);
        break;
      }

      break;
    }

    case STANDBY: {
      // transition to standby if not calibrated
      if (!calibrated) {
        state_transition(CALIBRATE);
        break;
      }

      // transition to automate if enabled
      if (automate) {
        state_transition(AUTOMATE);
        break;
      }

      break;
    }

    case MOVE: {
      // approach target and transition to standby if reached
      if (mot_approach(position, move_to, 1)) {
        state_transition(STANDBY);
        break;
      }

      break;
    }

    case AUTOMATE: {
      // transition back to standby if disabled
      if (!automate) {
        state_transition(STANDBY);
        break;
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
      // approach target, set flag and transition to standby if reached
      if (mot_approach(position, reset_height - 10, 1)) {
        calibrated = true;
        state_transition(STANDBY);
        break;
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
  naos_subscribe("calibrate", 0, NAOS_LOCAL);

  // transition to standby
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
      return;
    }
  }

  // check for "stop" command
  else if (strcmp(topic, "stop") == 0 && scope == NAOS_LOCAL) {
    // disable automate
    naos_set_b("automate", false);

    // change state if safe
    if (state != RESET) {
      state_transition(STANDBY);
      return;
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

  // check for "calibrate" command
  else if (strcmp(topic, "calibrate") == 0 && scope == NAOS_LOCAL) {
    state_transition(CALIBRATE);
    return;
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
  // movement
  double movement = (invert_encoder ? r * -1 : r) * WINDING_LENGTH;

  // apply rotation
  position += movement;

  // apply movement
  usage += fabs(movement);

  // re-calibrate if usage is high and no reset is being performed
  if (state != RESET && usage > calib_interval) {
    usage = 0;
    state_transition(CALIBRATE);
  }

  // feed state machine
  state_feed();
}

static void dst(double d) {
  // update distance
  distance = d;

  // update calibration data
  if (calibration_data != NULL && d >= idle_height && d <= rise_height) {
    a32_smooth_update(calibration_data, d);
  }

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
    {.name = "calib-interval", .type = NAOS_LONG, .default_l = 200, .sync_l = &calib_interval},
};

static naos_config_t config = {.device_type = "tm-lo",
                               .firmware_version = "1.3.0",
                               .parameters = params,
                               .num_parameters = 12,
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

  // disable automate mode if end switch is pressed
  if (end_read()) {
    naos_set_b("automate", false);
  }

  // activate first state
  state_transition(OFFLINE);
}
