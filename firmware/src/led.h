#ifndef LED_H
#define LED_H

/**
 * Initialize LED module.
 */
void led_init();

/**
 * Set LED brightness.
 *
 * @param r - Value from 0 to 1023.
 * @param g - Value from 0 to 1023.
 * @param b - Value from 0 to 1023.
 * @param w - Value from 0 to 1023.
 * @param t - The time for the fade.
 */
void led_set(int r, int g, int b, int w, int t);

#endif  // LED_H
