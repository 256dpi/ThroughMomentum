typedef struct {
  gpio_num_t pin;
  rmt_channel_t ch;
  rmt_item32_t item;
} pulse_out_t;

void pulse_out_init(pulse_out_t* po);

void pulse_out_generate(pulse_out_t* po);
