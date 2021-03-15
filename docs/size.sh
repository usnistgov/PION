#!/bin/bash
set -eo pipefail

ESP32PKG=~/AppData/Local/Arduino15/packages/esp32
SIZE=$(find $ESP32PKG -name xtensa-esp32-elf-size.exe)
NM=$(find $ESP32PKG -name xtensa-esp32-elf-nm.exe)

export SIZE_B=$($SIZE -B *.ino.elf)
export SIZE_A=$($SIZE -A *.ino.elf)
export NM_C=$($NM -C *.ino.elf)

jq -cn '{
  size: ($ENV.SIZE_B | capture("\\n\\s*(?<text>\\d+)\\t\\s*(?<data>\\d+)\\t\\s*(?<bss>\\d+)\\t")),
  sizeB: $ENV.SIZE_B,
  sizeA: $ENV.SIZE_A,
  nm: $ENV.NM_C
}'
