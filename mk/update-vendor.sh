#!/bin/bash
set -eo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"/..

mkdir -p src/vendor

sed \
  -e '0,/explicit Mpi/ s/\(\s*\)explicit Mpi/\1explicit Mpi() noexcept = default;\n\n\1explicit Mpi/' \
  spake2-mbed/mbedtls-wrappers.hpp > src/vendor/mbedtls-wrappers.hpp

cp spake2-mbed/spake2.hpp spake2-mbed/spake2.cpp src/vendor

if ! [[ -f src/vendor/mbedtls-hkdf.c ]]; then
  curl -sfL https://github.com/espressif/mbedtls/raw/mbedtls-2.13.1/library/hkdf.c | \
  sed -e '1 i\#define MBEDTLS_HKDF_C' > src/vendor/mbedtls-hkdf.c
fi
