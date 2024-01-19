#!/bin/bash
set -eo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"/..

ARDUINO=$(command -v arduino-cli)

ESP32PKG=~/AppData/Local/Arduino15/packages/esp32
SIZE=$(find $ESP32PKG -name xtensa-esp32-elf-size.exe)
NM=$(find $ESP32PKG -name xtensa-esp32-elf-nm.exe)

SKETCH_BASELINE=$(pwd)/examples/baseline
SKETCH_DEVICE=$(pwd)/examples/device
BUILD=$(pwd)/build.device-firmware-size

edit_config_macros() {
  local SKETCH=$1
  shift 1
  if ! [[ -f ${SKETCH}/config.hpp ]]; then
    cp ${SKETCH}/sample.config.hpp ${SKETCH}/config.hpp
  fi
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
  local SKETCH=$1
  local OUTPUT=$2
  local PROGRAM="$3"
  shift 3
  if ! find $OUTPUT/*.ino.elf &>/dev/null; then
    $ARDUINO compile --fqbn esp32:esp32:esp32 --warnings more \
      --build-property 'build.partitions=noota_ffat' \
      --build-property 'upload.maximum_size=2097152' \
      --build-path $(cygpath -w $OUTPUT) \
      ${SKETCH} >/dev/null
  fi

  SIZE_B=$($SIZE -B $OUTPUT/*.ino.elf) \
  SIZE_A=$($SIZE -A $OUTPUT/*.ino.elf) \
  NM_C=$($NM -C $OUTPUT/*.ino.elf) \
    jq -cn --argjson program "$PROGRAM" '{
      program: $program,
      size: ($ENV.SIZE_B | capture("\\n\\s*(?<text>\\d+)\\t\\s*(?<data>\\d+)\\t\\s*(?<bss>\\d+)\\t")),
      sizeB: $ENV.SIZE_B,
      sizeA: $ENV.SIZE_A,
      nm: $ENV.NM_C
    }'
}

edit_config_macros $SKETCH_BASELINE \
  -BASELINE_WANT_SERIAL \
  -BASELINE_WANT_BLE_DEVICE \
  -BASELINE_WANT_WIFI_AP \
  -BASELINE_WANT_WIFI_STA
build_and_measure $SKETCH_BASELINE ${BUILD}/baseline-blank '["baseline-blank"]'

edit_config_macros $SKETCH_BASELINE \
  +BASELINE_WANT_SERIAL \
  -BASELINE_WANT_BLE_DEVICE \
  -BASELINE_WANT_WIFI_AP \
  -BASELINE_WANT_WIFI_STA
build_and_measure $SKETCH_BASELINE ${BUILD}/baseline-hello '["baseline-hello"]'

edit_config_macros $SKETCH_BASELINE \
  +BASELINE_WANT_SERIAL \
  -BASELINE_WANT_BLE_DEVICE \
  +BASELINE_WANT_WIFI_AP \
  +BASELINE_WANT_WIFI_STA
build_and_measure $SKETCH_BASELINE ${BUILD}/baseline-wifi '["baseline-wifi"]'

edit_config_macros $SKETCH_BASELINE \
  +BASELINE_WANT_SERIAL \
  +BASELINE_WANT_BLE_DEVICE \
  -BASELINE_WANT_WIFI_AP \
  +BASELINE_WANT_WIFI_STA
build_and_measure $SKETCH_BASELINE ${BUILD}/baseline-ble-wifi '["baseline-ble-wifi"]'

for DIRECT in wifi ble; do
  edit_config_macros $SKETCH_DEVICE -PION_DIRECT_WIFI -PION_DIRECT_BLE
  edit_config_macros $SKETCH_DEVICE +PION_DIRECT_${DIRECT^^}
  for INFRA in udp ether; do
    edit_config_macros $SKETCH_DEVICE -PION_INFRA_UDP -PION_INFRA_ETHER
    edit_config_macros $SKETCH_DEVICE +PION_INFRA_${INFRA^^}
    for INCLUDE_STEPS in connect pake full; do
      case $INCLUDE_STEPS in
        connect) edit_config_macros $SKETCH_DEVICE +PION_SKIP_PAKE +PION_SKIP_NDNCERT ;;
        pake) edit_config_macros $SKETCH_DEVICE -PION_SKIP_PAKE +PION_SKIP_NDNCERT ;;
        full) edit_config_macros $SKETCH_DEVICE -PION_SKIP_PAKE -PION_SKIP_NDNCERT ;;
      esac
      build_and_measure $SKETCH_DEVICE \
        ${BUILD}/${DIRECT}-${INFRA}-${INCLUDE_STEPS} \
        "$(jq -cn --arg direct $DIRECT --arg infra $INFRA --arg include_steps $INCLUDE_STEPS \
          '["direct-" + $direct, "infra-" + $infra, "include-steps-" + $include_steps]')"
    done
  done
done
