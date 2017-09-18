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
 */
void led_set(int r, int g, int b, int w);
