#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

QueueHandle_t enc_rotation_queue;

static uint8_t enc_rotation_state = 0;

static void enc_rotation_handler(void *_) {
  // read GPIOs
  int p1 = gpio_get_level(GPIO_NUM_23);
  int p2 = gpio_get_level(GPIO_NUM_22);

  // calculate encoder change
  uint8_t state = (uint8_t)(enc_rotation_state & 3);
  if (p1) state |= 4;
  if (p2) state |= 8;
  enc_rotation_state = (state >> 2);

  // send relative change to queue
  switch (state) {
    case 1:
    case 7:
    case 8:
    case 14: {
      int vp = 1;
      xQueueSendFromISR(enc_rotation_queue, &vp, NULL);
      return;
    }
    case 2:
    case 4:
    case 11:
    case 13: {
      int vm = -1;
      xQueueSendFromISR(enc_rotation_queue, &vm, NULL);
      return;
    }
    case 3:
    case 12: {
      int vpp = 2;
      xQueueSendFromISR(enc_rotation_queue, &vpp, NULL);
      return;
    }
    case 6:
    case 9: {
      int vmm = -2;
      xQueueSendFromISR(enc_rotation_queue, &vmm, NULL);
      return;
    }
    default: {
      // do nothing
    }
  }
}

void enc_init() {
  // create queues
  enc_rotation_queue = xQueueCreate(64, sizeof(int));

  // configure rotation pins
  gpio_config_t rc;
  rc.pin_bit_mask = GPIO_SEL_23 | GPIO_SEL_22;
  rc.mode = GPIO_MODE_INPUT;
  rc.intr_type = GPIO_INTR_ANYEDGE;
  rc.pull_up_en = GPIO_PULLUP_ENABLE;
  rc.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_config(&rc);

  // preset state
  if (gpio_get_level(GPIO_NUM_23)) enc_rotation_state |= 1;
  if (gpio_get_level(GPIO_NUM_22)) enc_rotation_state |= 2;

  // install gpio interrupt service
  gpio_install_isr_service(0);

  // add interrupt handlers
  gpio_isr_handler_add(GPIO_NUM_23, enc_rotation_handler, NULL);
  gpio_isr_handler_add(GPIO_NUM_22, enc_rotation_handler, NULL);
}

int enc_get() {
  // prepare variables
  int total = 0;
  int next = 0;

  // get all relative changes
  while (xQueueReceive(enc_rotation_queue, &next, 0) == pdTRUE) {
    total += next;
  }

  return total;
}
