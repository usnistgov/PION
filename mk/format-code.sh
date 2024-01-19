#!/bin/bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"/..

git ls-files '**/*.[hc]pp' '**/*.ino' |
  xargs clang-format-15 -i -style=file

git ls-files -- '*.sh' | xargs \
  docker run --rm -u $(id -u):$(id -g) -v $PWD:/mnt -w /mnt mvdan/shfmt:v3 -l -w -s -i=2 -ci
