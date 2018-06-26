# Firmware

**The ESP32 and NAOS based light object firmware.**

## Topics

### `> flash {TIME}`

Flashes the light for the specified amount of milliseconds.

### `> flash-color {RED GREEN BLUE WHITE TIME}`

Flashes the light in colors for the specified amount of milliseconds.

### `> move up; move down; move {POSITION}`

Move up, down or to a specific position.

### `> stop`

Immediately stops any movement sets the current position as the target, cancels the manual turn mode and turns of the
automate mode.

### `> disco`

Disco randomly selects a color for the lights.

### `< position`

The current position of the object.

### `< motion`

The currently measured motion.

## Parameters

### `automate (off)`

When automate is `on` the light will move according to the sensors.

### `winding-length (7.5)`

The length of the cable needed for one average winding.

### `base-height (50)`

The base height of the light object.

### `idle-height (100)`

The idle height of the light object.

### `rise-height (150)`

The rise height of the light object when motion is detected.

### `reset-height (200)`

The reset height of the light object.

### `idle-ligth (127)`

The intensity of the light in idle mode.

### `flash-intensity (1023)`

The intensity of the light flash.

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
