# WATER-HEAT-REGULATOR On ESP32 under Zephyr SDK

We'll be using Zephyr RTOS environment for the development. We could have used the Arduino SDK, but our assumption is that Wifi is better managed under Zephyr.

We need to regulate the water of the hot water tanker.
We don't want to put a gauge inside the tanker.
We have everyday use of hot water, and glueing a temperature gauge on the outputpipe of the hot water, we assume, we can have a pretty good idea of the hot water temperature.

We want a screen and buttons to be able to scroll menu and set up a few things such as :

- Automatic clock setup -> Wifi -> network

- Manual clock setup

- Immediate heat / Heat duration

- Low cost period / Heat duration


We want also to be informed in case of a problem:

- Temperature too low Alarm (for shower obviously)

- Temperature too high Alarm (abnormal high temperature)




