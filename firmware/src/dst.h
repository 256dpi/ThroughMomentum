#ifndef DST_H
#define DST_H

/**
 * The distance callback is called with new distance readings.
 *
 * @param d The distance.
 */
typedef void (*dst_callback_t)(double d);

/**
 * Initialize ultra sonic distance sensor.
 *
 * @param cb The distance callback.
 */
void dst_init(dst_callback_t cb);

#endif  // DST_H
