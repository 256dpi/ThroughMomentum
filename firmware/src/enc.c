#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <naos.h>

#include "enc.h"

// https://github.com/PaulStoffregen/Encoder/blob/master/Encoder.h

#define ENC_RESOLUTION 20

#define END_BIT (1 << 0)

static EventGroupHandle_t enc_group;

static enc_callback_t enc_callback;

static volatile uint8_t enc_state = 0;

static volatile int16_t enc_total = 0;

static void enc_rotation_handler(void *_) {
  // read GPIOs
  int p1 = gpio_get_level(GPIO_NUM_23);
  int p2 = gpio_get_level(GPIO_NUM_25);

  // calculate encoder change
  uint8_t state = (uint8_t)(enc_state & 3);
  if (p1) state |= 4;
  if (p2) state |= 8;
  enc_state = (state >> 2);

  // save relative change
  switch (state) {
    case 1:
    case 7:
    case 8:
    case 14: {
      enc_total += 1;
      break;
    }
    case 2:
    case 4:
    case 11:
    case 13: {
      enc_total -= 1;
      break;
    }
    case 3:
    case 12: {
      enc_total += 2;
      break;
    }
    case 6:
    case 9: {
      enc_total -= 2;
      break;
    }
    default: {
      // no movement
    }
  }

  // send event
  xEventGroupSetBitsFromISR(enc_group, END_BIT, NULL);
}

static double enc_get() {
  // prepare mutex
  static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

  // get saved total
  vTaskEnterCritical(&mux);
  double total = enc_total;
  enc_total = 0;
  vTaskExitCritical(&mux);

  // calculate and return real rotation
  return total / ENC_RESOLUTION;
}

static void enc_task(void *p) {
  // loop forever
  for (;;) {
    // wait for bit
    EventBits_t bits = xEventGroupWaitBits(enc_group, END_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if ((bits & END_BIT) != END_BIT) {
      continue;
    }

    // call callback
    naos_acquire();
    enc_callback(enc_get());
    naos_release();

    // wait for 1ms
    naos_delay(1);

    // clear bit
    xEventGroupClearBits(enc_group, END_BIT);
  }
}

void enc_init(enc_callback_t cb) {
  // save callback
  enc_callback = cb;

  // create mutex
  enc_group = xEventGroupCreate();

  // configure rotation pins
  gpio_config_t rc;
  rc.pin_bit_mask = GPIO_SEL_23 | GPIO_SEL_25;
  rc.mode = GPIO_MODE_INPUT;
  rc.intr_type = GPIO_INTR_ANYEDGE;
  rc.pull_up_en = GPIO_PULLUP_ENABLE;
  rc.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_config(&rc);

  // preset state
  if (gpio_get_level(GPIO_NUM_23)) enc_state |= 1;
  if (gpio_get_level(GPIO_NUM_25)) enc_state |= 2;

  // add interrupt handlers
  gpio_isr_handler_add(GPIO_NUM_23, enc_rotation_handler, NULL);
  gpio_isr_handler_add(GPIO_NUM_25, enc_rotation_handler, NULL);

  // run async task
  xTaskCreatePinnedToCore(&enc_task, "enc", 2048, NULL, 2, NULL, 1);
}
