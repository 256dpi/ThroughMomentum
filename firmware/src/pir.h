#ifndef PIR_H
#define PIR_H

#include <stdbool.h>

/**
 * Initialize PIR sensor.
 */
void pir_init();

/**
 * Read the analog PIR value.
 *
 * @return The motion from 0 to ~400.
 */
int pir_read();

#endif  // PIR_H
