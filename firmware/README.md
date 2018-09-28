# Firmware

**The ESP32 and NAOS based light object firmware.**

## Topics

### `-> move up; move down; move {POSITION}`

Move up, down or to a specific position.

### `-> stop`

Immediately stop any movement.

### `-> zero`

Zero light object using end stop.

### `-> fade {RED} {GREEN} {BLUE} {WHITE} {TIME}`

Fades the color of the lights for the specified amount of milliseconds.

### `-> flash {RED} {GREEN} {BLUE} {WHITE} {TIME}`

Flashes the light in colors for the specified amount of milliseconds.

### `<- position`

The current position of the object.

### `<- motion`

If motions is currently measured.

## Parameters

### `automate (0)`

When automate is `1` the light will move according to the sensors.

### `idle-height (50)`

The idle height of the light object.

### `base-height (100)`

The base height of the light object.

### `rise-height (150)`

The rise height of the light object when motion is detected.

### `reset-height (200)`

The reset height of the light object.

### `idle-ligth (127)`

The intensity of the light in idle mode.

### `zero-switch (1)`

If the zero switch should be enabled.

### `invert-encoder`

If the encoder value should be inverted.

### `pir-sensitivity (400)`

The PIR sensitivity from 0 (high) to 400 (low).

### `pir-interval (2000)`

The interval between motion on off detection.

### `winding-length (7.5)`

The length of the cable needed for one average winding.
