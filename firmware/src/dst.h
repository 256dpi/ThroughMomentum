#ifndef DIST_H
#define DIST_H

typedef void (*dst_callback_t)(double d);

/**
 * Initialize ultra sonic distance sensor.
 */
void dst_init(dst_callback_t);

#endif  // DIST_H
