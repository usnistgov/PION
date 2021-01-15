#!/bin/bash
set -eo pipefail
cd "$( dirname "${BASH_SOURCE[0]}" )"/..

mkdir -p src/vendor

sed \
  -e '0,/explicit Mpi/ s/\(\s*\)explicit Mpi/\1explicit Mpi() noexcept = default;\n\n\1explicit Mpi/' \
  -e '0,/explicit EcGroup/ s/\(\s*\)explicit EcGroup/\1explicit EcGroup() noexcept = default;\n\n\1explicit EcGroup/' \
  spake2-mbed/mbedtls-wrappers.hpp > src/vendor/mbedtls-wrappers.hpp

sed \
  -e '/TODO: move to \.cpp/,/namespace/ d' \
  -e '/#endif.*HPP/ i\} // namespace spake2' \
  spake2-mbed/spake2.hpp > src/vendor/spake2.hpp

sed \
  -e '1 i\#include "spake2.hpp"' \
  -e '1 i\#include <endian.h>' \
  -e '1 i\namespace spake2 {' \
  -e '1,/TODO: move to \.cpp/ d' \
  -e '/SPAKE2_MBED_SPAKE2_HPP/ d' \
  -e 's/\binline\b//g' \
  spake2-mbed/spake2.hpp > src/vendor/spake2.cpp

if ! [[ -f src/vendor/mbedtls-hkdf.c ]]; then
  curl -sfL https://github.com/espressif/mbedtls/raw/mbedtls-2.13.1/library/hkdf.c | \
  sed -e '1 i\#define MBEDTLS_HKDF_C' > src/vendor/mbedtls-hkdf.c
fi
