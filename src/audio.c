#include <driver/uart.h>
#include <driver/gpio.h>

void audio_init() {
  // prepare uart config
  uart_config_t uart_config = {
      .baud_rate = 115200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .rx_flow_ctrl_thresh = 122,
  };

  // configure uart
  ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));

  // set uart pins
  ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, GPIO_NUM_19, -1, -1, -1));

  // install uart driver
  ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, UART_FIFO_LEN + 1, 0, 0, NULL, 0));
}

void audio_play(int n) {
  if (n == 1) {
    uart_write_bytes(UART_NUM_1, "1", 1);
  } else if(n == 2) {
    uart_write_bytes(UART_NUM_1, "2", 1);
  } else if(n == 3) {
    uart_write_bytes(UART_NUM_1, "3", 1);
  }
}

void audio_stop() {
  uart_write_bytes(UART_NUM_1, "s", 1);
}
