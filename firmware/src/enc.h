#ifndef ENC_H
#define ENC_H

#include <stdbool.h>

/**
 * Callback is execute with rotational change up to a frequency of 1ms.
 *
 * @param rot Rotations.
 */
typedef void (*enc_callback_t)(double rot);

/**
 * Initialize the encoder sub system.
 */
void enc_init(enc_callback_t);

#endif  // ENC_H
