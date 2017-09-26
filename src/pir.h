#ifndef PIR_H
#define PIR_H

#include <stdbool.h>

/**
 * Initialize PIR sensor.
 */
void pir_init();

/**
 * Get current PIR state.
 *
 * @return If motion has been detected.
 */
bool pir_get();

#endif  // PIR_H
