#ifndef MOT_H
#define MOT_H

/**
 * Initialize motor.
 */
void mot_init();

/**
 * Set motor state.
 *
 * @param speed Number from -1024 to 1024.
 */
void mot_set(int speed);

/**
 * Move upwards.
 *
 * @param speed Speed in cm/s (max 12cm/s).
 */
void mot_move_up(double speed);

/**
 * Move downwards.
 *
 * @param speed Speed in cm/s (max 12cm/s).
 */
void mot_move_down(double speed);

/**
 * Stop motor.
 */
void mot_hard_stop();

#endif  // MOT_H
