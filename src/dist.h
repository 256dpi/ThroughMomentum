#ifndef DIST_H
#define DIST_H

/**
 * Initialize ultra sonic distance sensor.
 */
void dist_init();

/**
 * Distance to nearest object.
 *
 * @return Distance in cm.
 */
double dist_get();

#endif  // DIST_H
