# EsaBusModule

This is a repository for all things around the EsaBusModule, an Arduino Nano compatible development board for the ESA 1919 and ESA 5000 electric scooters.


## Operation Principle

This board can listen to dashboard inputs and send its own commands to the motor controller without the need to make changes to the original dashboard or the motor controller PCB.
It just plugs in between the dashboard and the motor controller at the already existing 4-pin connector inside the handle bar.
Examples for commands are: Enabling the immobilizer lock mode, toggling the ECO mode or the lights and setting a new maximum speed.
It can read inputs from the dashboard such as the throttle and brake levers as well as the button on the dashboard.


## Hardware
Version 1.0:
![EsaBusModule1.0](https://github.com/ands/EsaBusModule/blob/master/hardware/EsaBusModule1.0/EsaBusModule1.0_Photo.jpg?raw=true "EsaBusModule1.0")


## Firmware

[EsaSpeedSwitch](https://github.com/ands/EsaBusModule/tree/master/firmware/EsaSpeedSwitch "EsaSpeedSwitch"): A firmware that lets you control the maximum speed and also lock your scooter.
