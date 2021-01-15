#!/bin/bash
set -eo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"/..

git ls-files '**/*.[hc]pp' '**/*.ino' | \
  xargs clang-format-8 -i -style=file
