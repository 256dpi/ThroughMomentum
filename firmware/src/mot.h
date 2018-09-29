#ifndef MOT_H
#define MOT_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Initialize motor.
 */
void mot_init();

/**
 * Approach specified target.
 *
 * @param position The current position.
 * @param target The target position.
 * @param time The interval this function is called.
 * @return
 */
bool mot_approach(double position, double target, uint32_t time);

/**
 * Stop motor.
 */
void mot_stop();

#endif  // MOT_H
