# Firmware

**The ESP32 and NAOS based light object firmware.**

## Topics

### `> flash {TIME}`

Flashes the light for the specified amount of milliseconds.

### `> turn up/down`

Puts the motor in manual mode and drives in the requested direction.

### `> move {POSITION}`

Disables automate mode and moves the object to the specified height/position.

### `> stop`

Immediately stops any movement sets the current position as the target, cancels the manual turn mode and turns of the
automate mode.
  
### `> reset {POSITION}`

Reset will save the specified position as the current position.

### `> disco`

Disco randomly selects a color for the lights.

### `< position`

The current position of the object.

### `< motion`

The currently measured motion.

## Parameters

### `automate (off)`

When automate is `on` the light will set its target position according to the sensor information.

### `winding-length (7.5)`

The length of the cable needed for one average winding.

### `idle-height (100)`

The idle height of the light object.

### `rise-height (150)`

The rise height of the light object when motion is detected.

### `max-height (200)`

The maximum height of the light object when motion is detected.

### `idle-ligth (127)`

The intensity of the light in idle mode.

### `flash-intensity (1023)`

The intensity of the light flash.

### `save-threshold (2)`

The threshold to pass to trigger a position save.

### `saved-position (0)`

The last auto-saved position.

### `min-down-speed (350)`

The minimal downwards motor speed.

### `min-up-speed (350)`

The minimal upwards motor speed.

### `max-down-speed (500)`

The maximal downwards motor speed.

### `max-up-speed (950)`

The maximal upwards motor speed.

### `speed-map-range (20)`

The range of the target mapped speed.

### `move-precision (1)`

The precision of the movement.

### `pir-sensitivity (400)`

The PIR sensitivity from 0 (high) to 400 (low).

### `pir-interval (2000)`

The interval between motion on off detection.
