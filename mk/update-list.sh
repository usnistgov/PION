#!/bin/bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"/..
export LC_ALL=C

(
  cd src
  echo 'pion_files = files('
  find -name '*.cpp' -printf '%P\n' | sort | sed "s|.*|'\0'|" | paste -sd,
  echo ')'
) >src/meson.build
