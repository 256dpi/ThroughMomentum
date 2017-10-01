#ifndef MOT_H
#define MOT_H

/**
 * Initialize motor.
 */
void mot_init();

/**
 * Set motor state.
 *
 * @param speed - Number from -1024 to 1024.
 */
void mot_set(int speed);

#endif  // MOT_H