#include <driver/rmt.h>

typedef struct {
    gpio_num_t pin;
    rmt_channel_t ch;
} pulse_in_t;

void pulse_in_init(pulse_in_t * pi);
