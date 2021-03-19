#!/bin/bash
set -eo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"/..

ARDUINO=$(which arduino-cli)

ESP32PKG=~/AppData/Local/Arduino15/packages/esp32
SIZE=$(find $ESP32PKG -name xtensa-esp32-elf-size.exe)
NM=$(find $ESP32PKG -name xtensa-esp32-elf-nm.exe)

SKETCH=$(pwd)/examples/device
BUILD=$(pwd)/build.device-firmware-size

edit_config_macros() {
  sed -i "$(for V in "$@"; do
    if [[ $V =~ ^\+ ]]; then
      echo '-e' "/#define ${V#+}$/ s|^// #|#|"
    elif [[ $V =~ ^\- ]]; then
      echo '-e' "/#define ${V#-}$/ s|^#|// #|"
    else
      exit 1
    fi
  done)" ${SKETCH}/config.hpp
}

build_and_measure() {
  local OUTPUT=${BUILD}/${DIRECT}-${INFRA}-${INCLUDE_STEPS}
  if ! find $OUTPUT/*.ino.elf &>/dev/null; then
    $ARDUINO compile --fqbn esp32:esp32:esp32 --warnings more \
      --build-property 'build.partitions=noota_ffat' --build-property 'upload.maximum_size=2097152' \
      --build-path $(cygpath -w $OUTPUT) \
      ${SKETCH} >/dev/null
  fi

  SIZE_B=$($SIZE -B $OUTPUT/*.ino.elf) \
  SIZE_A=$($SIZE -A $OUTPUT/*.ino.elf) \
  NM_C=$($NM -C $OUTPUT/*.ino.elf) \
  jq -cn \
    --arg direct $DIRECT \
    --arg infra $INFRA \
    --arg include_steps $INCLUDE_STEPS \
  '{
    program: [
      "direct-" + $direct,
      "infra-" + $infra,
      "include-steps-" + $include_steps
    ],
    size: ($ENV.SIZE_B | capture("\\n\\s*(?<text>\\d+)\\t\\s*(?<data>\\d+)\\t\\s*(?<bss>\\d+)\\t")),
    sizeB: $ENV.SIZE_B,
    sizeA: $ENV.SIZE_A,
    nm: $ENV.NM_C
  }'
}

for DIRECT in wifi ble; do
  edit_config_macros -NDNOB_DIRECT_WIFI -NDNOB_DIRECT_BLE
  edit_config_macros +NDNOB_DIRECT_${DIRECT^^}
  for INFRA in udp ether; do
    edit_config_macros -NDNOB_INFRA_UDP -NDNOB_INFRA_ETHER
    edit_config_macros +NDNOB_INFRA_${INFRA^^}
    for INCLUDE_STEPS in connect pake full; do
      case $INCLUDE_STEPS in
        connect) edit_config_macros +NDNOB_SKIP_PAKE +NDNOB_SKIP_NDNCERT;;
        pake   ) edit_config_macros -NDNOB_SKIP_PAKE +NDNOB_SKIP_NDNCERT;;
        full   ) edit_config_macros -NDNOB_SKIP_PAKE -NDNOB_SKIP_NDNCERT;;
      esac
      build_and_measure
    done
  done
done > extras/firmware-sizes.ndjson
