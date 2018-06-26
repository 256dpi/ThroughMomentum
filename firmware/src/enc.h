#ifndef ENC_H
#define ENC_H

#include <stdbool.h>

typedef void (*enc_callback_t)(double rot);

void enc_init(enc_callback_t);

#endif  // ENC_H
