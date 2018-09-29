#ifndef END_H
#define END_H

#include <stdbool.h>

/**
 * The end stop callback.
 */
typedef void (*end_callback_t)();

/**
 * Initialize the end stop system.
 *
 * @param cb The end stop callback.
 */
void end_init(end_callback_t cb);

/**
 * Read end switch.
 *
 * @return If switch is currently pressed.
 */
bool end_read();

#endif  // END_H
