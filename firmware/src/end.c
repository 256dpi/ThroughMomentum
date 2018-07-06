#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <naos.h>

#include "end.h"

#define END_BIT (1 << 0)

static EventGroupHandle_t end_group;

static end_callback_t end_callback;

static void end_handler(void *args) {
  // send event
  xEventGroupSetBitsFromISR(end_group, END_BIT, NULL);
}

static void end_task(void *p) {
  // loop forever
  for (;;) {
    // wait for bit
    EventBits_t bits = xEventGroupWaitBits(end_group, END_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if ((bits & END_BIT) != END_BIT) {
      continue;
    }

    // call callback
    naos_acquire();
    end_callback();
    naos_release();

    // wait for 50ms
    naos_delay(50);

    // clear bit
    xEventGroupClearBits(end_group, END_BIT);
  }
}

void end_init(end_callback_t cb) {
  // save callback
  end_callback = cb;

  // create mutex
  end_group = xEventGroupCreate();

  // prepare in a+b config
  gpio_config_t end = {.pin_bit_mask = GPIO_SEL_13,
                       .mode = GPIO_MODE_INPUT,
                       .pull_up_en = GPIO_PULLUP_DISABLE,
                       .pull_down_en = GPIO_PULLDOWN_DISABLE,
                       .intr_type = GPIO_INTR_POSEDGE};

  // configure in a+b pins
  ESP_ERROR_CHECK(gpio_config(&end));

  // register interrupt handler
  gpio_isr_handler_add(GPIO_NUM_13, &end_handler, NULL);

  // run async task
  xTaskCreatePinnedToCore(&end_task, "end", 2048, NULL, 2, NULL, 1);
}
