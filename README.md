# EsaBusModule

This is a repository for all things around the EsaBusModule, an Arduino Nano compatible development board for the ESA 1919 and ESA 5000 electric scooters.

## How does it work?

The added board sends different commands to the motor controller. You can set a new max. speed or enable lock mode (immobilizer) without changing the original dashboard or controller.
It makes use of throttle & brake levers and the button on the dashboard.

## How to use?

There are 3 speed settings LOW (20 km/h), MEDIUM (25 km/h) and HIGH (30 km/h) and the option to enable/disable lock mode.
If you use the original hardware, the max. speed after power on is always 20 km/h, so you have to set a new speed every time.
A successful change of speed is shown by switching to eco mode for a short time (green LED lights up).
You can not change speed limit of eco mode, it will stay at 15 km/h.

NEW: there is now another, unsuspicious way of setting the default speed to 20km/h. In case of a "special situation" :wink:

- fully depress and hold throttle and brake for 3 seconds = 20 km/h
- 3x button or 5x button = 20 km/h (probably remove this, for less complexity?)
- 5x button and throttle lightly depressed = 25 km/h
- 5x button and throttle fully depressed = 30 km/h

- 4x button and brake fully depressed = enable lock mode
- 6x button and brake lightly depressed = disable lock mode