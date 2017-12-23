[![Build Status](https://travis-ci.org/kylegordon/DrivewayLights.png?branch=master)](https://travis-ci.org/kylegordon/DrivewayLights)

Driveway Lights Project
=======================

This uses JeeNode ID2, and a Dimmer plug. The dimmer plug is connected to a series of 2N7000 MOSFETS, and directly controls 3 12V LED lights on the house wall.

Port 1 - Unused (but potentially used for digital input 4 as the rear light switch or something else)
Port 2 - Defective due to burnt out digital pin 5
Port 3 - Dimmer plug - overhangs defective Port 2
Port 4 - Used for digital input 7 for the front light switch

2N7000 MOSFETS - one per channel. Common positive on the bottom screw terminal

WIRING
======

Front Cable
-----------
Blue + Blue/White = Front LED
Green + Green/White = Front Switch

Rear Cable
----------
Blue + Blue/White = Middle LED
Green + Green/White = Rear LED


Rev 2 - Quad MOSFET Board & CC3000 WiFi
=======================================

1 - Unused
2 - CC3000 IRQ
3 - Digital PWM Channel 1
4 - CC3000 VBEN
5 - Digital PWM Channel 2
6 - Digital PWM Channel 3
7 - Digital input, pulled low
8 - Digital input, pulled low
9 - Digital PWM Channel 4
10 - CC3000 CS
11 - CC3000 MOSI
12 - CC3000 MISO
13 - CC3000 CLK
RST - Reset button
Vcc - 5v
GNS - GND
