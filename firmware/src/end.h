#ifndef END_H
#define END_H

#include <stdbool.h>

typedef void (*end_callback_t)();

void end_init(end_callback_t);

#endif  // END_H
