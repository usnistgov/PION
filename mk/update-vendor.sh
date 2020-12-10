#!/bin/bash
set -eo pipefail
cd "$( dirname "${BASH_SOURCE[0]}" )"/..

mkdir -p src/vendor

sed \
  -e '0,/explicit Mpi/ s/\(\s*\)explicit Mpi/\1explicit Mpi() noexcept = default;\n\n\1explicit Mpi/' \
  -e '0,/explicit EcGroup/ s/\(\s*\)explicit EcGroup/\1explicit EcGroup() noexcept = default;\n\n\1explicit EcGroup/' \
  spake2-mbed/mbedtls-wrappers.hpp > src/vendor/mbedtls-wrappers.hpp

sed \
  -e '/mbedtls-wrappers\.hpp/ i\#include <endian.h>' \
  spake2-mbed/spake2.hpp > src/vendor/spake2.hpp
