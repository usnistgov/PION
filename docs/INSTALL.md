# Installation Guide

This page describes how to install programs in this repository.

## Device

The [device](../examples/device) is an Arduino sketch targeting **ESP32** development boards with ESP32-D0WD or similar chips.
It also contains certain instrumentation features.

To install the program to a microcontroller:

1. Install Arduino IDE, [Arduino core for the ESP32](https://github.com/espressif/arduino-esp32) v1.0.5, and [esp8266ndn](https://github.com/yoursunny/esp8266ndn) library
2. Clone this repository to `$HOME/Arduino/libraries`
3. Copy `sample.config.hpp` to `config.hpp`, and modify as necessary
4. Flash the firmware as usual

## Authenticator

The [authenticator](../programs/authenticator) is a CLI program for Linux.
It can be installed on Ubuntu 20.04 with the [install-programs.sh](../extras/install-programs.sh) script.

## Certificate Authority

The [certificate authority](../extras/ca) is a Node.js program.

To install the program:

1. Install Node.js 15.x and NPM 7.x with [nvm](https://github.com/nvm-sh/nvm)
2. `npm install`
3. Modify `.env` as necessary
