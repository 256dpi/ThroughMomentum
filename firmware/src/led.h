#ifndef LED_H
#define LED_H

typedef struct {
  /**
   * The individual channels in values from 0 to 1023.
   */
  int r, g, b, w;
} led_color_t;

/**
 * Initialize LED module.
 */
void led_init();

/**
 * Set LED brightness.
 * @param c The color.
 * @param t The total time of the fade.
 */
void led_set(led_color_t c, int t);

/**
 * Perform LED flash.
 *
 * @param c The fade in color.
 * @param c The fade out color.
 * @param tt The total time of the fade.
 */
void led_flash(led_color_t fic, led_color_t foc, int tt);

#endif  // LED_H
