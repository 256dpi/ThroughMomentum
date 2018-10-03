# Firmware

**The ESP32 and NAOS based light object firmware.**

## Topics

### `-> move up; move down; move {POSITION}`

Move up, down or to a specific position.

### `-> stop`

Immediately stop any movement.

### `-> fade {RED} {GREEN} {BLUE} {WHITE} {TIME}`

Fades the color of the lights for the specified amount of milliseconds.

### `-> flash {RED} {GREEN} {BLUE} {WHITE} {TIME}`

Flashes the light in colors for the specified amount of milliseconds.

### `-> calibrate`

Trigger a new calibration.

### `<- position`

The current position of the object.

### `<- distance`

The current distance measured by the sensor.

### `<- motion`

If motions is currently measured.

## Parameters

### `debug (false)`

Enable debug lighting.

### `automate (false)`

When automate is `1` the light will move according to the sensors.

### `approach-range (40)`

The distance considered a valid approach range.

### `approach-target (20)`

The distance to approach from the detected object.

### `idle-height (50)`

The idle height of the light object.

### `base-height (100)`

The base height of the light object.

### `rise-height (150)`

The rise height of the light object when motion is detected.

### `reset-height (200)`

The reset height of the light object.

### `idle-light (127)`

The intensity of the light in idle mode.

### `zero-switch (true)`

If the zero switch should be enabled.

### `invert-encoder (true)`

If the encoder value should be inverted.

### `pir-low (200)`

The PIR low sensitivity from 0 (high) to 400 (low).

### `pir-high (400)`

The PIR high sensitivity from 0 (high) to 400 (low).

### `pir-interval (2000)`

The interval between motion on off detection.

### `calib-interval (200)`

The amount of way the object moves before another calibration is triggered.
