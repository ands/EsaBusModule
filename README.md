# EsaBusModule

This is a repository for all things around the EsaBusModule, an Arduino Nano compatible development board for the ESA 1919 and ESA 5000 electric scooters.


## Operation Principle

The added board can listen to dashboard inputs and send its own commands to the motor controller without the need to change the original dashboard or the motor controller PCB.
Examples for commands are: Setting a new maximum speed, enabling the immobilizer lock mode, toggling the ECO mode or the lights.
It can read inputs from the dash board such as the throttle and brake levers and the button on the dashboard.


## Hardware
Version 1.0:
![EsaBusModule1.0](https://github.com/ands/EsaBusModule/blob/master/hardware/EsaBusModule1.0/EsaBusModule1.0_Photo.jpg?raw=true "EsaBusModule1.0")


## Firmware

[EsaSpeedSwitch](https://github.com/ands/EsaBusModule/tree/master/firmware/EsaSpeedSwitch "EsaSpeedSwitch"): A firmware that lets you control the maximum speed and also lock your scooter.
