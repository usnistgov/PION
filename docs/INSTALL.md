# Installation Guide

This page describes how to install programs in this repository.

## Device

The [device](../examples/device) is an Arduino sketch targeting **ESP32** development boards with ESP32-D0WD or similar chips.
It also contains certain instrumentation features.

To install the program to a microcontroller:

1. Install Arduino IDE, [Arduino core for the ESP32](https://github.com/espressif/arduino-esp32) v2.0.14, [NimBLE](https://github.com/h2zero/NimBLE-Arduino) and [esp8266ndn](https://github.com/yoursunny/esp8266ndn) libraries.
2. Clone this repository to `$HOME/Arduino/libraries`.
3. Copy `sample.config.hpp` to `config.hpp`, and modify as necessary.
4. In Arduino Tools menu, select "Board: ESP32 Dev Module" and "Partition Scheme: No OTA (2MB APP/2MB FATFS)".
5. Flash the firmware as usual.

## Authenticator

The [authenticator](../programs/authenticator) is a CLI program for Linux.
It can be installed on Ubuntu 20.04 with the [programs/install.sh](../programs/install.sh) script.

## Certificate Authority

The [certificate authority](../extras/ca) is a Node.js program.

To install the program:

1. Install Node.js 20.x with [nvm](https://github.com/nvm-sh/nvm) or [NodeSource APT](https://github.com/nodesource/distributions).
2. `corepack pnpm install`.
3. Modify `.env` as necessary.

To start the program:

* `corepack pnpm -s start --nop` enables "nop" challenge, for obtaining authenticator certificate.
* `corepack pnpm -s start` enables "possession" challenge only, for normal operation.

## PCAP Parser

[pcapparse](../extras/pcapparse) is a Go program for parsing packet dump from an experiment.

To install the program:

1. Install Go 1.22.
2. `go install ./cmd/pion-pcapparse`
3. `sudo install -t /usr/local/bin ~/go/bin/pion-pcapparse`

## Experiment Script

The [experiment script](../extras/exp) is a Node.js program.

Installation procedure is same as the certificate authority.

To run the experiment:

1. Setup the environment according to [experiment system setup](expsetup.md).
2. Install the authenticator and the PCAP parser.
3. Create an authenticator certificate (see below).
4. Start the certificate authority normally.
5. `pipenv install`.
6. `corepack pnpm -s start --count N` runs the experiment N times, default is 1.

To create an authenticator certificate, start the certificate authority with "nop" challenge enabled, then:

```bash
export NDNPH_UPLINK_UDP=127.0.0.1
export NDNPH_UPLINK_UDP_PORT=6363
unset NDNPH_UPLINK_MTU
export NDNPH_KEYCHAIN=./runtime/keychain

ndnph-keychain keygen a /my-network/32=pion-authenticator/$(openssl rand -hex 4) >/dev/null
ndnph-ndncertclient -P ../ca/runtime/profile.data -i a | ndnph-keychain certimport a
ndnph-keychain certinfo a
```
