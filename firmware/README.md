# Firmware

**The ESP32 and NAOS based light object firmware.**

## Topics

`> flash {TIME}`

Flashes the light for the specified amount of milliseconds.

`> move {POSITION}`

Moves the object to the specified height/position.

`> stop`

Stop immediately stops any movement. The `automate` parameter should be set to `off` beforehand.

`> reset {POSITION}`

Reset will save the specified position as the current position.

`> disco`

Disco randomly selects a color for the lights.

## Parameters

## `winding-length (7.5)`

The length of the cable needed for one average winding.

## `idle-height (100)`

The idle height of the light object.

## `rise-height (150)`

The rise height of the light object when motion is detected.

## `max-height (200)`

The maximum height of the light object when motion is detected.

## `automate (off)`

When automate is `on` the light will set its target position according to the sensor information.

## `idle-ligth (127)`

The intensity of the light in idle mode.

## `flash-intensity (1023)`

The intensity of the light flash.

## `save-threshold (2)`

The threshold to pass to trigger a position save.

## `saved-position (0)`

The last auto-saved position.

## `motor-speed (950)`

The used motor speed.
