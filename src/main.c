#include <naos.h>
#include <driver/adc.h>
#include <string.h>
#include <stdlib.h>

#include "dist.h"
#include "mic.h"
#include "audio.h"
#include "led.h"

double last_dist = 0;
double last_mic = 0;

int red = 0;
int green = 0;
int blue = 0;

void online() {
  naos_subscribe("play/1", 0, NAOS_GLOBAL);
  naos_subscribe("play/2", 0, NAOS_GLOBAL);
  naos_subscribe("play/3", 0, NAOS_GLOBAL);
  naos_subscribe("stop", 0, NAOS_GLOBAL);
  naos_subscribe("red", 0, NAOS_GLOBAL);
  naos_subscribe("blue", 0, NAOS_GLOBAL);
  naos_subscribe("green", 0, NAOS_GLOBAL);
}

void message(const char *topic, const char *payload, unsigned int len, naos_scope_t scope) {
  if (strcmp(topic, "play/1") == 0) {
    audio_play(1);
  } else if (strcmp(topic, "play/2") == 0) {
    audio_play(2);
  } else if (strcmp(topic, "play/3") == 0) {
    audio_play(3);
  } else if (strcmp(topic, "stop") == 0) {
    audio_stop();
  } else if (strcmp(topic, "red") == 0) {
    red = atoi(payload);
    led_set(red, green, blue);
  } else if (strcmp(topic, "green") == 0) {
    green = atoi(payload);
    led_set(red, green, blue);
  } else if (strcmp(topic, "blue") == 0) {
    blue = atoi(payload);
    led_set(red, green, blue);
  }
}

void loop() {
  // check for distance change
  double d = dist_read();
  //    naos_log("dist: %f", d);
  if (d > last_dist + 10 || d < last_dist - 10) {
    // send update
    char buf[64];
    sprintf(buf, "%f", d);
    naos_publish_str("dist", buf, 0, false, NAOS_GLOBAL);

    // save value
    last_dist = d;
  }

//  // check for microphone change
//  double m = mic_read();
//
//  if (m > last_mic + 5 || m < last_mic - 5) {
//    // send update
//    char buf[64];
//    sprintf(buf, "%f", m);
//    naos_publish_str("mic", buf, 0, false, NAOS_GLOBAL);
//
//    // save value
//    last_mic = m;
//  }
}

static naos_config_t config = {
    .device_type = "vas17", .firmware_version = "0.0.1", .loop_callback = loop, .loop_interval = 0, .online_callback = online, .message_callback = message};

void app_main() {
  // set general adc width
  adc1_config_width(ADC_WIDTH_10Bit);

  // initialize naos
  naos_init(&config);

  // initialize distance sensor
  dist_init();

  // initialize microphone
//  mic_init();

  // initialize audio
//  audio_init();

  // initialize led
  led_init();
}
